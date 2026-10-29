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

#ifndef OPENAICOMPATIBLEPROVIDER_H
#define OPENAICOMPATIBLEPROVIDER_H

#include "chatprovider.h"

#include <QNetworkAccessManager>

class QNetworkReply;

namespace Coproducer {

// A chat-completions client for the OpenAI wire protocol: streams tokens as
// they arrive via Server-Sent Events. baseUrl may point at OpenAI itself or
// any compatible endpoint (OpenRouter, Groq, a locally run llama-server,
// etc). Used directly for the "OpenAI API" provider, and also by
// LocalLlamaServerProvider to talk to its own spawned llama-server process,
// so neither provider implementation depends on the other.
class OpenAiCompatibleProvider : public ChatProvider
{
    Q_OBJECT

public:
    explicit OpenAiCompatibleProvider(const QString &apiKey,
                                      const QString &model,
                                      const QString &baseUrl,
                                      QObject *parent = nullptr);

    QString name() const override { return QStringLiteral("OpenAI-compatible"); }
    void sendMessage(const QList<ChatMessage> &history) override;
    void cancel() override;

private slots:
    void onReadyRead();
    void onFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager m_network;
    QString m_apiKey;
    QString m_model;
    QString m_baseUrl;
    QNetworkReply *m_reply = nullptr;
    QByteArray m_rawBuffer;
    QString m_streamText;
    QString m_finishReason;
    QList<ToolCall> m_streamToolCalls; // Indexed by the stream's tool_calls[].index
};

} // namespace Coproducer

#endif // OPENAICOMPATIBLEPROVIDER_H
