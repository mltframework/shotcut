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

#ifndef SHOTCUT_MCP_MCPSERVER_H
#define SHOTCUT_MCP_MCPSERVER_H

#include "mcp/httpserver.h"
#include "mcp/toolregistry.h"

#include <QObject>
#include <QTcpServer>

namespace Shotcut {
namespace Mcp {

class McpServer : public QObject
{
    Q_OBJECT
public:
    explicit McpServer(QObject *parent = nullptr);
    explicit McpServer(const ToolRegistry &registry, QObject *parent = nullptr);
    ~McpServer() override;

    bool listen(quint16 port = 0);
    void close();
    quint16 boundPort() const;

    const ToolRegistry &registry() const;

signals:
    void errorMessage(const QString &message);

private:
    void handleHttpRequest(const HttpServer::Request &request, HttpServer::Responder responder);
    void handleForumSearchRequest(HttpServer::Responder responder,
                                  const QJsonValue &id,
                                  const QString &toolName,
                                  const QJsonObject &arguments);
    QJsonObject handleRequest(const QJsonObject &request, QString *error) const;

    QTcpServer *m_server = nullptr;
    HttpServer m_httpServer;
    ToolRegistry m_registry;
};

} // namespace Mcp
} // namespace Shotcut

#endif // SHOTCUT_MCP_MCPSERVER_H
