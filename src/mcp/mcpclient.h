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

#ifndef SHOTCUT_MCP_MCPCLIENT_H
#define SHOTCUT_MCP_MCPCLIENT_H

#include <QJsonObject>
#include <QObject>
#include <QSet>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

namespace Shotcut {
namespace Mcp {

class McpClient : public QObject
{
    Q_OBJECT
public:
    explicit McpClient(quint16 serverPort, QObject *parent = nullptr);

    quint64 initialize();
    quint64 toolsList();
    quint64 callTool(const QString &name, const QJsonObject &arguments);
    void cancelAll();

signals:
    void requestCompleted(quint64 requestId, const QJsonObject &response, const QString &error);

private:
    quint64 sendRequest(const QString &method,
                        const QJsonObject &params = QJsonObject(),
                        int timeoutMs = 5000);
    void post(const QJsonObject &message,
              const QString &protocolVersion,
              std::function<void(int, const QByteArray &, const QString &)> callback,
              int timeoutMs = 5000);

    QNetworkAccessManager *m_network = nullptr;
    QSet<QNetworkReply *> m_pendingReplies;
    quint64 m_nextRequestId = 0;
    quint16 m_serverPort = 0;
    QString m_protocolVersion;
};

} // namespace Mcp
} // namespace Shotcut

#endif // SHOTCUT_MCP_MCPCLIENT_H
