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

#ifndef SHOTCUT_MCP_HTTPSERVER_H
#define SHOTCUT_MCP_HTTPSERVER_H

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QUrl>

#include <functional>
#include <memory>

class QTcpServer;
class QTcpSocket;

namespace Shotcut {
namespace Mcp {

// A deliberately small QHttpServer-shaped façade for Qt builds without QtHttpServer.
// Keep its API and behavior limited to the subset Shotcut uses so it can be replaced
// by an adapter backed by QHttpServer when that module is available. Do not grow it
// into a general-purpose HTTP server.
class HttpServer : public QObject
{
public:
    enum class Method { Get, Post, Delete, Any };

    struct Request
    {
        QByteArray method;
        QUrl url;
        QHash<QByteArray, QByteArray> headers;
        QByteArray body;

        QByteArray header(const QByteArray &name) const;
    };

    struct Response
    {
        int statusCode = 200;
        QByteArray contentType = QByteArrayLiteral("application/json");
        QByteArray body;
        QHash<QByteArray, QByteArray> headers;
    };

    class Responder
    {
    public:
        void write(const Response &response) const;

    private:
        friend class HttpServer;
        explicit Responder(std::function<void(const Response &)> writer,
                           std::shared_ptr<bool> completed);

        std::function<void(const Response &)> m_writer;
        std::shared_ptr<bool> m_completed;
    };

    using Handler = std::function<void(const Request &, Responder)>;

    explicit HttpServer(QObject *parent = nullptr);

    void route(const QString &path, Method method, const QObject *context, Handler handler);
    bool bind(QTcpServer *server);

private:
    struct Route
    {
        QString path;
        Method method = Method::Any;
        QPointer<const QObject> context;
        Handler handler;
    };

    void onNewConnection();
    void onReadyRead(QTcpSocket *socket);
    void dispatch(QTcpSocket *socket, const Request &request);
    void writeResponse(QTcpSocket *socket, const Response &response);
    void reject(QTcpSocket *socket, int statusCode, const QByteArray &message);

    QPointer<QTcpServer> m_server;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QList<Route> m_routes;
};

} // namespace Mcp
} // namespace Shotcut

#endif // SHOTCUT_MCP_HTTPSERVER_H