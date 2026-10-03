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
#include "coproducer/forumsearch.h"

#include <QDockWidget>
#include <QElapsedTimer>
#include <QList>
#include <QTextCursor>

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTextEdit;
class QTimer;
class QToolButton;

class CoproducerDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit CoproducerDock(QWidget *parent = nullptr);

private slots:
    void onNewChatClicked();
    void onSendOrCancelClicked();
    void onModeChanged(int index);
    void onSearchResults(const QList<Coproducer::ForumSource> &sources, const QString &error);
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
    // Conversation: chat with the model, which may call the forum search
    // tool itself. Help Search: forum search only, top results shown
    // directly, no model involved.
    enum class Mode { Conversation, HelpSearch };

    void ensureProvider();
    void unloadProvider();
    void appendTranscript(TranscriptRole role, const QString &text);
    void rebuildTranscript();
    void setBusy(bool busy);
    void sendPrompt();
    void cancelRequest();
    void runNextToolCall();
    void finishToolCall(const QString &result);
    void onForumToolResults(const QList<Coproducer::ForumSource> &sources, const QString &error);
    void showHelpSearchResults(const QList<Coproducer::ForumSource> &sources, const QString &error);
    void setStatusBase(const QString &text);
    void updateStatusLabel();
    static QString formatElapsedTime(int seconds);
    void insertMarkdownFragment(QTextCursor &cursor, const QString &text);
    void updateAssistantStream(const QString &partialText);
    void finishAssistantStream(const QString &fullText);
    QString applyCitations(const QString &text) const;

    QTextEdit *m_transcript;
    QPlainTextEdit *m_promptEdit;
    QPushButton *m_sendButton;
    QToolButton *m_newChatButton;
    QComboBox *m_modeCombo;
    QToolButton *m_settingsButton;
    QLabel *m_statusLabel;
    QTimer *m_thinkingTimer;
    QList<Coproducer::ChatMessage> m_history;
    QString m_pendingResponse;
    QString m_statusBase;
    int m_promptGroupCounter = 0;
    int m_thinkingSeconds = 0;
    bool m_busy = false;
    Mode m_mode = Mode::Conversation;
    int m_streamStartPosition = -1;
    QElapsedTimer m_promptTimer;
    Coproducer::ChatProvider *m_provider = nullptr;
    Coproducer::ForumSearch *m_forumSearch = nullptr;
    QList<Coproducer::ForumSource> m_lastSources;
    // Tool search: the conversation for the current prompt, including tool
    // calls and results, which is resent after each round of tool calls.
    QList<Coproducer::ChatMessage> m_toolPayload;
    QList<Coproducer::ToolCall> m_queuedToolCalls;
    Coproducer::ToolCall m_activeToolCall;
    bool m_toolSearchActive = false;
    int m_toolRounds = 0;
};

#endif // COPRODUCERDOCK_H
