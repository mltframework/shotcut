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

#include "localllamaserverprovider.h"

#include "Logger.h"
#include "openaicompatibleprovider.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTcpServer>
#include <QUrl>

namespace Coproducer {

LocalLlamaServerProvider::LocalLlamaServerProvider(const QString &modelPath, QObject *parent)
    : ChatProvider(parent)
    , m_modelPath(modelPath)
    , m_server(new QProcess(this))
    , m_healthNetwork(new QNetworkAccessManager(this))
    , m_healthTimer(new QTimer(this))
    , m_startupTimeout(new QTimer(this))
{
    m_server->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_server, &QProcess::started, this, [this] {
        m_healthTimer->start();
        checkServerHealth();
    });
    connect(m_server, &QProcess::readyReadStandardOutput, this, [this] {
        m_serverOutput += QString::fromLocal8Bit(m_server->readAllStandardOutput());
        if (m_serverOutput.size() > 8192)
            m_serverOutput = m_serverOutput.right(8192);
    });
    connect(m_server,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (m_stopping || (!m_starting && !m_serverReady))
                    return;
                const auto detail = m_serverOutput.trimmed();
                const auto reason = detail.isEmpty()
                                        ? tr("llama-server exited with code %1.").arg(exitCode)
                                        : detail;
                LOG_WARNING() << "Coproducer: llama-server exited unexpectedly:" << reason;
                m_serverReady = false;
                m_starting = false;
                m_healthTimer->stop();
                m_startupTimeout->stop();
                if (m_chatProvider) {
                    m_chatProvider->cancel();
                    m_chatProvider->deleteLater();
                    m_chatProvider = nullptr;
                }
                Q_UNUSED(exitStatus)
                emit errorOccurred(reason);
            });
    connect(m_server, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && m_starting)
            failStartup(
                tr("Unable to start llama-server. Check that it is installed with Shotcut."));
    });

    m_healthTimer->setInterval(1000);
    connect(m_healthTimer, &QTimer::timeout, this, &LocalLlamaServerProvider::checkServerHealth);
    m_startupTimeout->setSingleShot(true);
    m_startupTimeout->setInterval(30 * 60 * 1000);
    connect(m_startupTimeout, &QTimer::timeout, this, [this] {
        failStartup(tr("Timed out while loading the local model."));
    });
}

LocalLlamaServerProvider::~LocalLlamaServerProvider()
{
    if (m_chatProvider)
        m_chatProvider->cancel();
    stopServer();
}

QString LocalLlamaServerProvider::findServerExecutable() const
{
    QString executableName = QStringLiteral("llama-server");
#ifdef Q_OS_WIN
    executableName += QStringLiteral(".exe");
#endif
    const auto appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates
        = {QDir(appDir).filePath(executableName),
           QDir(appDir).filePath(QStringLiteral("../bin/") + executableName),
           QDir(appDir).filePath(QStringLiteral("../MacOS/") + executableName)};
    for (const auto &candidate : candidates) {
        if (QFileInfo::exists(candidate) && QFileInfo(candidate).isExecutable())
            return QFileInfo(candidate).absoluteFilePath();
    }
    return QStandardPaths::findExecutable(executableName);
}

void LocalLlamaServerProvider::sendMessage(const QList<ChatMessage> &history)
{
    if (m_chatProvider) {
        m_chatProvider->setTools(m_tools, m_toolCallsAllowed);
        m_chatProvider->sendMessage(history);
        return;
    }
    if (m_starting) {
        emit errorOccurred(tr("The local model is still loading."));
        return;
    }
    if (m_modelPath.isEmpty()) {
        emit errorOccurred(tr("No local model has been selected."));
        return;
    }
    if (!QFileInfo::exists(m_modelPath)) {
        emit errorOccurred(tr("Local model not found: %1").arg(m_modelPath));
        return;
    }

    startServer(history);
}

void LocalLlamaServerProvider::startServer(const QList<ChatMessage> &history)
{
    const auto executable = findServerExecutable();
    if (executable.isEmpty()) {
        emit errorOccurred(
            tr("llama-server was not found. Install it with Shotcut or add it to PATH."));
        return;
    }

    QTcpServer portProbe;
    if (!portProbe.listen(QHostAddress::LocalHost, 0)) {
        emit errorOccurred(tr("Unable to reserve a localhost port for llama-server."));
        return;
    }
    const auto port = portProbe.serverPort();
    portProbe.close();

    m_apiKey = QStringLiteral("shotcut-%1").arg(QRandomGenerator::system()->generate64(), 0, 16);
    m_baseUrl = QStringLiteral("http://127.0.0.1:%1/v1").arg(port);
    m_pendingHistory = history;
    m_serverOutput.clear();
    m_starting = true;
    m_serverReady = false;
    m_stopping = false;

    const QStringList arguments = {QStringLiteral("--model"),
                                   m_modelPath,
                                   QStringLiteral("--host"),
                                   QStringLiteral("127.0.0.1"),
                                   QStringLiteral("--port"),
                                   QString::number(port),
                                   QStringLiteral("--api-key"),
                                   m_apiKey,
                                   QStringLiteral("--alias"),
                                   QStringLiteral("shotcut-local"),
                                   QStringLiteral("--no-webui"),
                                   QStringLiteral("--jinja"),
                                   QStringLiteral("--n-gpu-layers"),
                                   QStringLiteral("0")};
    m_server->setProgram(executable);
    m_server->setArguments(arguments);
    LOG_INFO() << "Coproducer: loading local model" << QFileInfo(m_modelPath).fileName();
    emit statusMessage(tr("Loading model %1...").arg(QFileInfo(m_modelPath).fileName()));
    m_startupTimeout->start();
    m_server->start();
}

void LocalLlamaServerProvider::checkServerHealth()
{
    if (!m_starting || m_healthReply || m_server->state() != QProcess::Running)
        return;

    QNetworkRequest request{QUrl(m_baseUrl + QStringLiteral("/health"))};
    request.setTransferTimeout(3000);
    m_healthReply = m_healthNetwork->get(request);
    connect(m_healthReply, &QNetworkReply::finished, this, [this, reply = m_healthReply] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool healthy = reply->error() == QNetworkReply::NoError && status == 200;
        reply->deleteLater();
        if (m_healthReply == reply)
            m_healthReply = nullptr;
        if (m_starting && healthy)
            onServerReady();
    });
}

void LocalLlamaServerProvider::onServerReady()
{
    m_starting = false;
    m_serverReady = true;
    m_healthTimer->stop();
    m_startupTimeout->stop();
    LOG_INFO() << "Coproducer: local model loaded" << QFileInfo(m_modelPath).fileName();

    m_chatProvider = new OpenAiCompatibleProvider(m_apiKey, QStringLiteral("shotcut-local"), m_baseUrl, this);
    connect(m_chatProvider, &ChatProvider::responseStarted, this, &ChatProvider::responseStarted);
    connect(m_chatProvider, &ChatProvider::tokenReceived, this, &ChatProvider::tokenReceived);
    connect(m_chatProvider, &ChatProvider::responseReceived, this, &ChatProvider::responseReceived);
    connect(m_chatProvider, &ChatProvider::toolCallsRequested, this, &ChatProvider::toolCallsRequested);
    connect(m_chatProvider, &ChatProvider::errorOccurred, this, &ChatProvider::errorOccurred);
    emit statusMessage(tr("Thinking..."));
    m_chatProvider->setTools(m_tools, m_toolCallsAllowed);
    m_chatProvider->sendMessage(m_pendingHistory);
    m_pendingHistory.clear();
}

void LocalLlamaServerProvider::failStartup(const QString &message)
{
    if (!m_starting)
        return;
    LOG_WARNING() << "Coproducer: local model failed to load:" << message;
    m_starting = false;
    m_healthTimer->stop();
    m_startupTimeout->stop();
    if (m_healthReply) {
        m_healthReply->abort();
        m_healthReply->deleteLater();
        m_healthReply = nullptr;
    }
    const auto detail = m_serverOutput.trimmed();
    stopServer();
    emit errorOccurred(detail.isEmpty() ? message : QStringLiteral("%1\n%2").arg(message, detail));
}

void LocalLlamaServerProvider::stopServer()
{
    if (m_serverReady)
        LOG_INFO() << "Coproducer: unloading local model" << QFileInfo(m_modelPath).fileName();
    m_stopping = true;
    m_starting = false;
    m_serverReady = false;
    m_healthTimer->stop();
    m_startupTimeout->stop();
    if (m_healthReply) {
        m_healthReply->abort();
        m_healthReply->deleteLater();
        m_healthReply = nullptr;
    }
    if (m_server->state() != QProcess::NotRunning) {
        m_server->kill();
        m_server->waitForFinished(1000);
    }
}

void LocalLlamaServerProvider::cancel()
{
    if (m_starting) {
        m_pendingHistory.clear();
        stopServer();
        return;
    }
    if (m_chatProvider)
        m_chatProvider->cancel();
}

} // namespace Coproducer