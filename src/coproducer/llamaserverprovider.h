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

#ifndef LLAMASERVERPROVIDER_H
#define LLAMASERVERPROVIDER_H

#include "aiprovider.h"

#include <QProcess>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

namespace Coproducer {

class OpenAiCompatibleClient;

class LlamaServerProvider : public AiProvider
{
    Q_OBJECT

public:
    explicit LlamaServerProvider(const QString &modelPath, QObject *parent = nullptr);
    ~LlamaServerProvider() override;

    QString name() const override { return QStringLiteral("Local model"); }
    void sendMessage(const QList<ChatMessage> &history) override;
    void cancel() override;

private:
    QString findServerExecutable() const;
    void startServer(const QList<ChatMessage> &history);
    void checkServerHealth();
    void onServerReady();
    void failStartup(const QString &message);
    void stopServer();

    QString m_modelPath;
    QString m_apiKey;
    QString m_baseUrl;
    QString m_serverOutput;
    QList<ChatMessage> m_pendingHistory;
    QProcess *m_server = nullptr;
    QNetworkAccessManager *m_healthNetwork = nullptr;
    QNetworkReply *m_healthReply = nullptr;
    QTimer *m_healthTimer = nullptr;
    QTimer *m_startupTimeout = nullptr;
    OpenAiCompatibleClient *m_chatProvider = nullptr;
    bool m_starting = false;
    bool m_serverReady = false;
    bool m_stopping = false;
};

} // namespace Coproducer

#endif // LLAMASERVERPROVIDER_H