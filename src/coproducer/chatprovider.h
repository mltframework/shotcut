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

#ifndef CHATPROVIDER_H
#define CHATPROVIDER_H

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>

namespace Coproducer {

// A function call requested by the model; arguments is a JSON object string.
struct ToolCall
{
    QString id;
    QString name;
    QString arguments;
};

// A function the model may call; parameters is a JSON Schema object.
struct ToolDefinition
{
    QString name;
    QString description;
    QJsonObject parameters;
};

// One turn in the conversation sent to/from an AI provider.
struct ChatMessage
{
    enum Role { System, User, Assistant, Tool };

    Role role = User;
    QString content;
    QList<ToolCall> toolCalls; // Assistant messages that requested tools
    QString toolCallId;        // Tool messages: the call this result answers
};

// Token accounting for a single request, when the provider reports it.
struct Usage
{
    int promptTokens = 0;
    int completionTokens = 0;
};

// Vendor-neutral interface so the Coproducer panel can target OpenAI,
// Anthropic, or other chat completion providers behind one API.
class ChatProvider : public QObject
{
    Q_OBJECT

public:
    explicit ChatProvider(QObject *parent = nullptr)
        : QObject(parent)
    {}
    ~ChatProvider() override = default;

    virtual QString name() const = 0;

    // Sends the full conversation history (including the system prompt) and
    // requests a single reply. Results arrive via the signals below.
    virtual void sendMessage(const QList<ChatMessage> &history) = 0;
    virtual void cancel() {}

    // Tools offered on subsequent requests. When allowCalls is false the tools
    // are still declared (history may reference them) but the model must answer.
    void setTools(const QList<ToolDefinition> &tools, bool allowCalls = true)
    {
        m_tools = tools;
        m_toolCallsAllowed = allowCalls;
    }

signals:
    void responseStarted();
    // General-purpose progress text (e.g. "Loading model..."), before
    // responseStarted(). Not all providers emit this; it's a no-op if unused.
    void statusMessage(const QString &text);
    // Reports progress evaluating the prompt itself, before generation
    // starts. Not all providers can report this; it's a no-op if unused.
    void promptProgress(int processedTokens, int totalTokens);
    void tokenReceived(const QString &token);
    void responseReceived(const QString &text, const Coproducer::Usage &usage);
    // Emitted instead of responseReceived() when the model wants tools run.
    // The caller should append the results and call sendMessage() again.
    void toolCallsRequested(const QList<Coproducer::ToolCall> &calls, const QString &content);
    void errorOccurred(const QString &message);

protected:
    QList<ToolDefinition> m_tools;
    bool m_toolCallsAllowed = true;
};

} // namespace Coproducer

#endif // CHATPROVIDER_H
