/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mcp/httpserver.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

namespace Shotcut {
namespace Mcp {

namespace {
constexpr qsizetype kMaxHeaderBytes = 16 * 1024;
constexpr qint64 kMaxBodyBytes = 1024 * 1024;

QByteArray reasonPhrase(int statusCode)
{
    switch (statusCode) {
    case 200:
        return QByteArrayLiteral("OK");
    case 202:
        return QByteArrayLiteral("Accepted");
    case 400:
        return QByteArrayLiteral("Bad Request");
    case 403:
        return QByteArrayLiteral("Forbidden");
    case 404:
        return QByteArrayLiteral("Not Found");
    case 405:
        return QByteArrayLiteral("Method Not Allowed");
    case 408:
        return QByteArrayLiteral("Request Timeout");
    case 413:
        return QByteArrayLiteral("Content Too Large");
    case 415:
        return QByteArrayLiteral("Unsupported Media Type");
    case 417:
        return QByteArrayLiteral("Expectation Failed");
    case 431:
        return QByteArrayLiteral("Request Header Fields Too Large");
    case 500:
        return QByteArrayLiteral("Internal Server Error");
    case 501:
        return QByteArrayLiteral("Not Implemented");
    default:
        return QByteArrayLiteral("Error");
    }
}

HttpServer::Method methodFromBytes(const QByteArray &method)
{
    if (method == "GET")
        return HttpServer::Method::Get;
    if (method == "POST")
        return HttpServer::Method::Post;
    if (method == "DELETE")
        return HttpServer::Method::Delete;
    return HttpServer::Method::Any;
}
} // namespace

QByteArray HttpServer::Request::header(const QByteArray &name) const
{
    return headers.value(name.toLower());
}

HttpServer::Responder::Responder(std::function<void(const Response &)> writer,
                                 std::shared_ptr<bool> completed)
    : m_writer(std::move(writer))
    , m_completed(std::move(completed))
{}

void HttpServer::Responder::write(const Response &response) const
{
    if (!m_writer || !m_completed || *m_completed)
        return;
    *m_completed = true;
    m_writer(response);
}

HttpServer::HttpServer(QObject *parent)
    : QObject(parent)
{}

void HttpServer::route(const QString &path, Method method, const QObject *context, Handler handler)
{
    m_routes.append({path, method, context, std::move(handler)});
}

bool HttpServer::bind(QTcpServer *server)
{
    if (!server || m_server)
        return false;
    m_server = server;
    connect(server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
    return true;
}

void HttpServer::onNewConnection()
{
    if (!m_server)
        return;

    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        if (!socket)
            continue;
        m_buffers.insert(socket, QByteArray());
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] { onReadyRead(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
            m_buffers.remove(socket);
            socket->deleteLater();
        });
        QTimer::singleShot(10000, socket, [this, socket] {
            if (m_buffers.contains(socket))
                reject(socket, 408, QByteArrayLiteral("Request timed out."));
        });
    }
}

void HttpServer::onReadyRead(QTcpSocket *socket)
{
    auto buffer = m_buffers.find(socket);
    if (buffer == m_buffers.end())
        return;

    buffer->append(socket->readAll());
    const qsizetype headerEnd = buffer->indexOf(QByteArrayLiteral("\r\n\r\n"));
    if (headerEnd < 0) {
        if (buffer->size() > kMaxHeaderBytes)
            reject(socket, 431, QByteArrayLiteral("Request headers are too large."));
        return;
    }
    if (headerEnd > kMaxHeaderBytes) {
        reject(socket, 431, QByteArrayLiteral("Request headers are too large."));
        return;
    }

    const QByteArray headerData = buffer->left(headerEnd);
    const QList<QByteArray> lines = headerData.split('\n');
    if (lines.isEmpty()) {
        reject(socket, 400, QByteArrayLiteral("Invalid request."));
        return;
    }

    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    if (requestLine.size() != 3
        || (requestLine.at(2) != "HTTP/1.1" && requestLine.at(2) != "HTTP/1.0")) {
        reject(socket, 400, QByteArrayLiteral("Invalid request line."));
        return;
    }

    Request request;
    request.method = requestLine.at(0);
    request.url = QUrl::fromEncoded(requestLine.at(1));
    if (!request.url.isValid() || !request.url.path().startsWith('/')) {
        reject(socket, 400, QByteArrayLiteral("Invalid request target."));
        return;
    }

    bool contentLengthFound = false;
    qint64 contentLength = 0;
    for (qsizetype index = 1; index < lines.size(); ++index) {
        const QByteArray line = lines.at(index).trimmed();
        const qsizetype colon = line.indexOf(':');
        if (colon <= 0) {
            reject(socket, 400, QByteArrayLiteral("Invalid request header."));
            return;
        }
        const QByteArray name = line.left(colon).trimmed().toLower();
        const QByteArray value = line.mid(colon + 1).trimmed();
        if (name.isEmpty() || name.contains(' ') || name.contains('\t')) {
            reject(socket, 400, QByteArrayLiteral("Invalid request header."));
            return;
        }
        if (name == "content-length") {
            bool ok = false;
            const qint64 parsed = value.toLongLong(&ok);
            if (!ok || parsed < 0 || (contentLengthFound && parsed != contentLength)) {
                reject(socket, 400, QByteArrayLiteral("Invalid Content-Length."));
                return;
            }
            contentLengthFound = true;
            contentLength = parsed;
        }
        if (name == "transfer-encoding") {
            reject(socket, 501, QByteArrayLiteral("Transfer-Encoding is not supported."));
            return;
        }
        request.headers.insert(name, value);
    }

    if (requestLine.at(2) == "HTTP/1.1" && request.header("host").isEmpty()) {
        reject(socket, 400, QByteArrayLiteral("Host header is required."));
        return;
    }
    if (contentLength > kMaxBodyBytes) {
        reject(socket, 413, QByteArrayLiteral("Request body is too large."));
        return;
    }

    const qsizetype bodyStart = headerEnd + 4;
    if (buffer->size() - bodyStart < contentLength)
        return;
    request.body = buffer->mid(bodyStart, contentLength);
    m_buffers.erase(buffer);
    dispatch(socket, request);
}

void HttpServer::dispatch(QTcpSocket *socket, const Request &request)
{
    const Method method = methodFromBytes(request.method);
    for (const Route &route : std::as_const(m_routes)) {
        if (route.context && route.path == request.url.path()
            && (route.method == Method::Any || route.method == method)) {
            const QPointer<QTcpSocket> guardedSocket(socket);
            auto completed = std::make_shared<bool>(false);
            Responder responder(
                [this, guardedSocket](const Response &response) {
                    if (guardedSocket)
                        writeResponse(guardedSocket, response);
                },
                completed);
            route.handler(request, responder);
            return;
        }
    }
    reject(socket, 404, QByteArrayLiteral("Not found."));
}

void HttpServer::writeResponse(QTcpSocket *socket, const Response &response)
{
    if (!socket || socket->state() == QAbstractSocket::UnconnectedState)
        return;

    QByteArray payload = "HTTP/1.1 " + QByteArray::number(response.statusCode) + " ";
    payload += reasonPhrase(response.statusCode) + "\r\n";
    if (!response.contentType.isEmpty())
        payload += "Content-Type: " + response.contentType + "\r\n";
    payload += "Content-Length: " + QByteArray::number(response.body.size()) + "\r\n";
    payload += "Connection: close\r\n";
    for (auto it = response.headers.cbegin(); it != response.headers.cend(); ++it)
        payload += it.key() + ": " + it.value() + "\r\n";
    payload += "\r\n";
    payload += response.body;
    socket->write(payload);
    socket->disconnectFromHost();
}

void HttpServer::reject(QTcpSocket *socket, int statusCode, const QByteArray &message)
{
    Response response;
    response.statusCode = statusCode;
    response.contentType = QByteArrayLiteral("text/plain; charset=utf-8");
    response.body = message;
    writeResponse(socket, response);
}

} // namespace Mcp
} // namespace Shotcut