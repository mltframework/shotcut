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

#include "mcp/mcpclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace Shotcut {
namespace Mcp {

McpClient::McpClient(quint16 serverPort, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_serverPort(serverPort)
{}

quint64 McpClient::initialize()
{
    const quint64 requestId = ++m_nextRequestId;
    QJsonObject params;
    params.insert("protocolVersion", QStringLiteral("2025-11-25"));
    params.insert("capabilities", QJsonObject());
    QJsonObject clientInfo;
    clientInfo.insert("name", QStringLiteral("Shotcut Coproducer"));
    clientInfo.insert("version", QStringLiteral("1.0.0"));
    params.insert("clientInfo", clientInfo);

    QJsonObject message;
    message.insert("jsonrpc", QStringLiteral("2.0"));
    message.insert("id", static_cast<qint64>(requestId));
    message.insert("method", QStringLiteral("initialize"));
    message.insert("params", params);
    post(message,
         QString(),
         [this, requestId](int status, const QByteArray &body, const QString &error) {
             if (!error.isEmpty() || status != 200) {
                 emit requestCompleted(requestId,
                                       QJsonObject(),
                                       error.isEmpty()
                                           ? QStringLiteral("MCP initialize failed with HTTP %1.")
                                                 .arg(status)
                                           : error);
                 return;
             }

             QJsonParseError parseError;
             const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
             if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                 emit requestCompleted(requestId,
                                       QJsonObject(),
                                       QStringLiteral("MCP initialize returned invalid JSON."));
                 return;
             }
             const QJsonObject response = document.object();
             if (response.contains("error")) {
                 emit requestCompleted(requestId,
                                       response,
                                       response.value("error").toObject().value("message").toString());
                 return;
             }
             m_protocolVersion
                 = response.value("result").toObject().value("protocolVersion").toString();
             if (m_protocolVersion.isEmpty()) {
                 emit requestCompleted(requestId,
                                       response,
                                       QStringLiteral("MCP initialize omitted protocolVersion."));
                 return;
             }

             QJsonObject notification;
             notification.insert("jsonrpc", QStringLiteral("2.0"));
             notification.insert("method", QStringLiteral("notifications/initialized"));
             post(notification,
                  m_protocolVersion,
                  [this, requestId, response](int notificationStatus,
                                              const QByteArray &,
                                              const QString &notificationError) {
                      if (!notificationError.isEmpty() || notificationStatus != 202) {
                          emit requestCompleted(
                              requestId,
                              response,
                              notificationError.isEmpty()
                                  ? QStringLiteral(
                                        "MCP initialized notification failed with HTTP %1.")
                                        .arg(notificationStatus)
                                  : notificationError);
                          return;
                      }
                      emit requestCompleted(requestId, response, QString());
                  });
         });
    return requestId;
}

quint64 McpClient::toolsList()
{
    return sendRequest(QStringLiteral("tools/list"));
}

quint64 McpClient::callTool(const QString &name, const QJsonObject &arguments)
{
    QJsonObject params;
    params.insert("name", name);
    params.insert("arguments", arguments);
    const int timeoutMs = name == QStringLiteral("shotcut_forum_search")
                                  || name == QStringLiteral("shotcut_documentation_search")
                              ? 35000
                              : 5000;
    return sendRequest(QStringLiteral("tools/call"), params, timeoutMs);
}

void McpClient::cancelAll()
{
    for (QNetworkReply *reply : std::as_const(m_pendingReplies))
        reply->abort();
}

quint64 McpClient::sendRequest(const QString &method, const QJsonObject &params, int timeoutMs)
{
    const quint64 requestId = ++m_nextRequestId;
    QJsonObject request;
    request.insert("jsonrpc", QStringLiteral("2.0"));
    request.insert("id", static_cast<qint64>(requestId));
    request.insert("method", method);
    if (!params.isEmpty())
        request.insert("params", params);

    post(
        request,
        m_protocolVersion,
        [this, requestId](int status, const QByteArray &body, const QString &error) {
            if (!error.isEmpty() || status != 200) {
                emit requestCompleted(requestId,
                                      QJsonObject(),
                                      error.isEmpty()
                                          ? QStringLiteral("MCP request failed with HTTP %1.")
                                                .arg(status)
                                          : error);
                return;
            }

            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                emit requestCompleted(requestId,
                                      QJsonObject(),
                                      QStringLiteral("MCP server returned invalid JSON."));
                return;
            }
            const QJsonObject response = document.object();
            const QString responseError
                = response.value("error").toObject().value("message").toString();
            emit requestCompleted(requestId, response, responseError);
        },
        timeoutMs);
    return requestId;
}

void McpClient::post(const QJsonObject &message,
                     const QString &protocolVersion,
                     std::function<void(int, const QByteArray &, const QString &)> callback,
                     int timeoutMs)
{
    const QUrl url(QStringLiteral("http://127.0.0.1:%1/mcp").arg(m_serverPort));
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json, text/event-stream");
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!protocolVersion.isEmpty())
        request.setRawHeader("MCP-Protocol-Version", protocolVersion.toUtf8());

    QNetworkReply *reply = m_network->post(request,
                                           QJsonDocument(message).toJson(QJsonDocument::Compact));
    m_pendingReplies.insert(reply);
    QTimer::singleShot(timeoutMs, reply, [reply] {
        if (reply->isRunning())
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback = std::move(callback)] {
        m_pendingReplies.remove(reply);
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        const QString error = reply->error() == QNetworkReply::NoError ? QString()
                                                                       : reply->errorString();
        reply->deleteLater();
        callback(status, body, error);
    });
}

} // namespace Mcp
} // namespace Shotcut
