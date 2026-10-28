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

#include "openaicompatibleprovider.h"

#include "Logger.h"
#include "util.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>

namespace {

QString roleToString(Coproducer::ChatMessage::Role role)
{
    switch (role) {
    case Coproducer::ChatMessage::System:
        return QStringLiteral("system");
    case Coproducer::ChatMessage::Assistant:
        return QStringLiteral("assistant");
    case Coproducer::ChatMessage::Tool:
        return QStringLiteral("tool");
    case Coproducer::ChatMessage::User:
    default:
        return QStringLiteral("user");
    }
}

} // namespace

namespace Coproducer {

OpenAiCompatibleProvider::OpenAiCompatibleProvider(const QString &apiKey,
                                                   const QString &model,
                                                   const QString &baseUrl,
                                                   QObject *parent)
    : ChatProvider(parent)
    , m_apiKey(apiKey)
    , m_model(model)
    , m_baseUrl(baseUrl.endsWith('/') ? baseUrl.chopped(1) : baseUrl)
{
    connect(&m_network,
            &QNetworkAccessManager::finished,
            this,
            &OpenAiCompatibleProvider::onFinished);
}

void OpenAiCompatibleProvider::sendMessage(const QList<ChatMessage> &history)
{
    if (m_apiKey.isEmpty()) {
        emit errorOccurred(
            tr("No OpenAI API key is configured. Open the Coproducer menu to add one."));
        return;
    }
    if (m_model.isEmpty()) {
        emit errorOccurred(tr("No model is configured. Open the Coproducer menu to choose one."));
        return;
    }
    if (!Util::isAllowedApiBaseUrl(m_baseUrl)) {
        emit errorOccurred(tr("The configured API base URL is not allowed. Use https, or http for "
                              "localhost only."));
        return;
    }

    QJsonArray messages;
    for (const auto &message : history) {
        QJsonObject obj;
        obj["role"] = roleToString(message.role);
        if (message.content.isEmpty() && !message.toolCalls.isEmpty())
            obj["content"] = QJsonValue::Null;
        else
            obj["content"] = message.content;
        if (!message.toolCalls.isEmpty()) {
            QJsonArray calls;
            for (const auto &call : message.toolCalls) {
                calls.append(
                    QJsonObject{{"id", call.id},
                                {"type", "function"},
                                {"function",
                                 QJsonObject{{"name", call.name}, {"arguments", call.arguments}}}});
            }
            obj["tool_calls"] = calls;
        }
        if (message.role == ChatMessage::Tool)
            obj["tool_call_id"] = message.toolCallId;
        messages.append(obj);
    }

    QJsonObject body;
    body["model"] = m_model;
    body["messages"] = messages;
    body["stream"] = true;
    // Disable OpenAI reasoning to allow function tools through Chat Completions.
    if (QUrl(m_baseUrl).host().compare(QStringLiteral("api.openai.com"), Qt::CaseInsensitive) == 0)
        body["reasoning_effort"] = QStringLiteral("none");
    if (!m_tools.isEmpty()) {
        QJsonArray tools;
        for (const auto &tool : m_tools) {
            tools.append(QJsonObject{{"type", "function"},
                                     {"function",
                                      QJsonObject{{"name", tool.name},
                                                  {"description", tool.description},
                                                  {"parameters", tool.parameters}}}});
        }
        body["tools"] = tools;
        body["tool_choice"] = m_toolCallsAllowed ? QStringLiteral("auto") : QStringLiteral("none");
    }

    QNetworkRequest request{QUrl(m_baseUrl + QStringLiteral("/chat/completions"))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());

    const auto payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    LOG_DEBUG() << "Coproducer: sending" << history.size() << "message(s) to" << m_baseUrl
                << "model" << m_model << "payload bytes=" << payload.size();
    m_rawBuffer.clear();
    m_streamText.clear();
    m_streamToolCalls.clear();
    m_finishReason.clear();
    m_reply = m_network.post(request, payload);
    connect(m_reply, &QNetworkReply::readyRead, this, &OpenAiCompatibleProvider::onReadyRead);
    emit responseStarted();
}

void OpenAiCompatibleProvider::cancel()
{
    if (m_reply)
        m_reply->abort();
}

// OpenAI streams the response as Server-Sent Events: lines of the form
// "data: {json chunk}", ending with "data: [DONE]". An error response (e.g.
// a bad API key) instead arrives as a single plain JSON body, so leave the
// buffer untouched for onFinished() to parse in that case.
void OpenAiCompatibleProvider::onReadyRead()
{
    m_rawBuffer += m_reply->readAll();
    if (m_reply->error() != QNetworkReply::NoError
        || m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() >= 400)
        return;

    int newlineIndex;
    while ((newlineIndex = m_rawBuffer.indexOf('\n')) >= 0) {
        const auto line = m_rawBuffer.left(newlineIndex).trimmed();
        m_rawBuffer.remove(0, newlineIndex + 1);
        if (!line.startsWith("data:"))
            continue;
        const auto payload = line.mid(5).trimmed();
        if (payload.isEmpty() || payload == "[DONE]")
            continue;
        const auto choices = QJsonDocument::fromJson(payload).object().value("choices").toArray();
        if (choices.isEmpty())
            continue;
        const auto choice = choices.at(0).toObject();
        const auto finishReason = choice.value("finish_reason").toString();
        if (!finishReason.isEmpty())
            m_finishReason = finishReason;
        const auto delta = choice.value("delta").toObject();
        const auto content = delta.value("content").toString();
        if (!content.isEmpty()) {
            m_streamText += content;
            emit tokenReceived(content);
        }
        // Tool call names/ids arrive once; arguments arrive as string fragments.
        for (const auto &value : delta.value("tool_calls").toArray()) {
            const auto callDelta = value.toObject();
            const int index = callDelta.value("index").toInt(m_streamToolCalls.size());
            if (index < 0 || index > 64)
                continue;
            while (m_streamToolCalls.size() <= index)
                m_streamToolCalls.append(ToolCall());
            auto &call = m_streamToolCalls[index];
            const auto id = callDelta.value("id").toString();
            if (!id.isEmpty())
                call.id = id;
            const auto function = callDelta.value("function").toObject();
            const auto name = function.value("name").toString();
            if (!name.isEmpty())
                call.name = name;
            call.arguments += function.value("arguments").toString();
        }
    }
}

void OpenAiCompatibleProvider::onFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        if (reply == m_reply)
            m_reply = nullptr;
        m_rawBuffer.clear();
        m_streamText.clear();
        m_streamToolCalls.clear();
        return; // CoproducerDock already reset its own state when the user canceled.
    }

    if (reply->error() != QNetworkReply::NoError) {
        m_rawBuffer += reply->readAll();
        const auto root = QJsonDocument::fromJson(m_rawBuffer).object();
        auto message = reply->errorString();
        const auto errorObj = root.value("error").toObject();
        if (!errorObj.value("message").toString().isEmpty())
            message = errorObj.value("message").toString();
        QStringList details;
        details << tr("Model: %1").arg(m_model);
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status)
            details << tr("HTTP status: %1").arg(status);
        for (const auto &key : {"type", "code", "param"}) {
            const auto value = errorObj.value(QLatin1String(key));
            if (value.isString() && !value.toString().isEmpty())
                details << QStringLiteral("%1: %2").arg(QLatin1String(key), value.toString());
        }
        const auto requestId = reply->rawHeader("x-request-id");
        if (!requestId.isEmpty())
            details << tr("Request ID: %1").arg(QString::fromUtf8(requestId));
        message += QStringLiteral("\n\n") + details.join(QLatin1Char('\n'));
        LOG_ERROR() << "Coproducer: OpenAI request failed:" << message;
        emit errorOccurred(message);
        if (reply == m_reply)
            m_reply = nullptr;
        m_rawBuffer.clear();
        m_streamText.clear();
        m_streamToolCalls.clear();
        return;
    }

    onReadyRead(); // Flush any trailing chunk that arrived without a final readyRead().
    if (reply == m_reply)
        m_reply = nullptr;

    QList<ToolCall> toolCalls;
    for (int i = 0; i < m_streamToolCalls.size(); ++i) {
        auto call = m_streamToolCalls.at(i);
        if (call.name.isEmpty())
            continue;
        if (call.id.isEmpty())
            call.id = QStringLiteral("call_%1").arg(i);
        toolCalls.append(call);
    }
    m_streamToolCalls.clear();
    if (!toolCalls.isEmpty()) {
        LOG_DEBUG() << "Coproducer: model requested" << toolCalls.size() << "tool call(s)";
        const auto content = m_streamText;
        m_streamText.clear();
        emit toolCallsRequested(toolCalls, content);
        return;
    }

    if (m_streamText.isEmpty()) {
        auto message = tr("OpenAI returned an empty response.");
        message += QStringLiteral("\n\n") + tr("Model: %1").arg(m_model);
        if (!m_finishReason.isEmpty())
            message += QLatin1Char('\n') + tr("Finish reason: %1").arg(m_finishReason);
        LOG_ERROR() << "Coproducer:" << message;
        emit errorOccurred(message);
        return;
    }

    Usage usage; // Not reported by the streaming Chat Completions API.
    emit responseReceived(m_streamText, usage);
    m_streamText.clear();
}

} // namespace Coproducer
