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

#ifndef COPRODUCERDOCK_H
#define COPRODUCERDOCK_H

#include "coproducer/chatprovider.h"
#include "mcp/forumsearch.h"
#include "mcp/mcpclient.h"

#include <QDockWidget>
#include <QElapsedTimer>
#include <QList>
#include <QQmlContext>
#include <QQuickWidget>

class QTimer;

class CoproducerDock : public QDockWidget
{
    Q_OBJECT
    Q_PROPERTY(QVariantList transcript READ transcript NOTIFY transcriptChanged)
    Q_PROPERTY(QString promptText READ promptText WRITE setPromptText NOTIFY promptTextChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)

signals:
    void transcriptChanged();
    void promptTextChanged();
    void statusTextChanged();
    void busyChanged();
    void modeChanged();

public:
    explicit CoproducerDock(quint16 mcpServerPort, QWidget *parent = nullptr);

    QVariantList transcript() const;
    QString promptText() const;
    void setPromptText(const QString &text);
    QString statusText() const;
    bool isBusy() const;
    int mode() const;
    void setMode(int value);

public slots:
    void submitPrompt();
    void newChat();
    void openSettings();

private slots:
    void onNewChatClicked();
    void onSendOrCancelClicked();
    void onModeChanged(int index);
    void onMcpRequestCompleted(quint64 requestId, const QJsonObject &response, const QString &error);
    void onResponseStarted();
    void onPromptProgress(int processedTokens, int totalTokens);
    void onTokenReceived(const QString &token);
    void onResponseReceived(const QString &text, const Coproducer::Usage &usage);
    void onToolCallsRequested(const QList<Coproducer::ToolCall> &calls, const QString &content);
    void onErrorOccurred(const QString &message);
    void onSettingsTriggered();
    void onThinkingTick();

private:
    enum class TranscriptRole { User, Assistant, Error, Diagnostic };
    // AI Chat: chat with the model, which may call the user-guide search
    // tool itself. Search User Guide: user-guide search only, top results shown
    // directly, no model involved.
    enum class Mode { AiChat, SearchUserGuide };

    void ensureProvider();
    void unloadProvider();
    void appendTranscript(TranscriptRole role, const QString &text);
    void rebuildTranscript();
    void setBusy(bool busy);
    void sendPrompt();
    void cancelRequest();
    void runNextToolCall();
    void finishToolCall(const QString &result);
    void onForumToolResults(const QList<Shotcut::Mcp::ForumSource> &sources, const QString &error);
    void onUserGuideMcpResults(const QJsonObject &structuredContent);
    void showUserGuideSearchResults(const QList<Shotcut::Mcp::ForumSource> &sources,
                                    const QString &error);
    void setStatusBase(const QString &text);
    void updateStatusLabel();
    static QString formatElapsedTime(int seconds);
    void updateAssistantStream(const QString &partialText);
    void finishAssistantStream(const QString &fullText);
    QString applyCitations(const QString &text) const;

    QQuickWidget *m_qview = nullptr;
    QVariantList m_transcriptData;
    QString m_promptText;
    QString m_statusText;
    QTimer *m_thinkingTimer;
    QList<Coproducer::ChatMessage> m_history;
    QString m_pendingResponse;
    QString m_statusBase;
    int m_thinkingSeconds = 0;
    bool m_busy = false;
    Mode m_mode = Mode::AiChat;
    QElapsedTimer m_promptTimer;
    Coproducer::ChatProvider *m_provider = nullptr;
    Shotcut::Mcp::McpClient *m_mcpClient = nullptr;
    QList<Coproducer::ToolDefinition> m_mcpTools;
    quint64 m_initializeRequestId = 0;
    quint64 m_toolListRequestId = 0;
    quint64 m_activeMcpRequestId = 0;
    QString m_pendingSearchQuery;
    QList<Shotcut::Mcp::ForumSource> m_lastSources;
    // Tool search: the conversation for the current prompt, including tool
    // calls and results, which is resent after each round of tool calls.
    QList<Coproducer::ChatMessage> m_toolPayload;
    QList<Coproducer::ToolCall> m_queuedToolCalls;
    Coproducer::ToolCall m_activeToolCall;
    bool m_toolSearchActive = false;
    int m_toolRounds = 0;
};

#endif // COPRODUCERDOCK_H
