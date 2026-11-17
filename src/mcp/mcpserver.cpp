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

#include "mcp/mcpserver.h"

#include "mcp/forumsearch.h"
#include "mcp/shotcuttools.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QPointer>
#include <QSet>

namespace Shotcut {
namespace Mcp {

namespace {
const QString kCurrentProtocolVersion = QStringLiteral("2025-11-25");
const QSet<QString> kSupportedProtocolVersions = {QStringLiteral("2025-11-25"),
                                                  QStringLiteral("2025-06-18"),
                                                  QStringLiteral("2025-03-26")};

bool isLoopbackHost(const QString &host)
{
    if (host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0)
        return true;
    QHostAddress address;
    return address.setAddress(host) && address.isLoopback();
}

HttpServer::Response httpResponse(int statusCode, const QJsonObject &body)
{
    HttpServer::Response response;
    response.statusCode = statusCode;
    response.contentType = QByteArrayLiteral("application/json");
    response.body = QJsonDocument(body).toJson(QJsonDocument::Compact);
    return response;
}

QJsonObject jsonRpcErrorObject(const QJsonValue &id, int code, const QString &message)
{
    QJsonObject error;
    error.insert("code", code);
    error.insert("message", message);
    QJsonObject response;
    response.insert("jsonrpc", QStringLiteral("2.0"));
    response.insert("id", id);
    response.insert("error", error);
    return response;
}

HttpServer::Response jsonRpcError(const QJsonValue &id, int code, const QString &message)
{
    return httpResponse(200, jsonRpcErrorObject(id, code, message));
}

QJsonObject jsonRpcResult(const QJsonValue &id, const QJsonObject &result)
{
    QJsonObject response;
    response.insert("jsonrpc", QStringLiteral("2.0"));
    response.insert("id", id);
    response.insert("result", result);
    return response;
}

QJsonObject toolResult(const QJsonObject &result, bool isError = false)
{
    QJsonArray content;
    QJsonObject text;
    text.insert("type", QStringLiteral("text"));
    text.insert("text", QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)));
    content.append(text);
    QJsonObject toolResponse;
    toolResponse.insert("content", content);
    toolResponse.insert("structuredContent", result);
    if (isError)
        toolResponse.insert("isError", true);
    return toolResponse;
}
} // namespace

McpServer::McpServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    m_httpServer.route(QStringLiteral("/mcp"),
                       HttpServer::Method::Any,
                       this,
                       [this](const HttpServer::Request &request, HttpServer::Responder responder) {
                           handleHttpRequest(request, responder);
                       });
    m_httpServer.bind(m_server);
#ifndef SHOTCUT_MCP_NO_DEFAULT_TOOLS
    m_registry = createShotcutToolRegistry();
#endif
}

McpServer::McpServer(const ToolRegistry &registry, QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_registry(registry)
{
    m_httpServer.route(QStringLiteral("/mcp"),
                       HttpServer::Method::Any,
                       this,
                       [this](const HttpServer::Request &request, HttpServer::Responder responder) {
                           handleHttpRequest(request, responder);
                       });
    m_httpServer.bind(m_server);
}

McpServer::~McpServer()
{
    close();
}

bool McpServer::listen(quint16 port)
{
    if (m_server->isListening())
        m_server->close();

    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        emit errorMessage(QStringLiteral("Failed to listen for MCP HTTP on localhost: %1")
                              .arg(m_server->errorString()));
        return false;
    }

    return true;
}

quint16 McpServer::boundPort() const
{
    return m_server->serverPort();
}

void McpServer::close()
{
    if (m_server && m_server->isListening())
        m_server->close();
}

const ToolRegistry &McpServer::registry() const
{
    return m_registry;
}

void McpServer::handleHttpRequest(const HttpServer::Request &request,
                                  HttpServer::Responder responder)
{
    const QByteArray hostHeader = request.header("host");
    const QUrl hostUrl(QStringLiteral("http://") + QString::fromLatin1(hostHeader));
    if (hostHeader.isEmpty() || !hostUrl.isValid() || !isLoopbackHost(hostUrl.host())) {
        HttpServer::Response response;
        response.statusCode = 403;
        response.contentType = QByteArrayLiteral("text/plain; charset=utf-8");
        response.body = QByteArrayLiteral("Only loopback Host values are allowed.");
        responder.write(response);
        return;
    }

    const QByteArray originHeader = request.header("origin");
    if (!originHeader.isEmpty()) {
        const QUrl origin = QUrl::fromEncoded(originHeader);
        if (!origin.isValid()
            || (origin.scheme() != QStringLiteral("http")
                && origin.scheme() != QStringLiteral("https"))
            || !origin.userInfo().isEmpty() || !isLoopbackHost(origin.host())) {
            HttpServer::Response response;
            response.statusCode = 403;
            response.contentType = QByteArrayLiteral("text/plain; charset=utf-8");
            response.body = QByteArrayLiteral("Origin is not allowed.");
            responder.write(response);
            return;
        }
    }

    if (request.method == "GET") {
        HttpServer::Response response;
        response.statusCode = 405;
        response.contentType.clear();
        response.headers.insert("Allow", QByteArrayLiteral("POST"));
        responder.write(response);
        return;
    }
    if (request.method != "POST") {
        HttpServer::Response response;
        response.statusCode = 405;
        response.contentType.clear();
        response.headers.insert("Allow", QByteArrayLiteral("POST"));
        responder.write(response);
        return;
    }

    const QByteArray contentType
        = request.header("content-type").split(';').first().trimmed().toLower();
    if (contentType != "application/json") {
        HttpServer::Response response;
        response.statusCode = 415;
        response.contentType.clear();
        responder.write(response);
        return;
    }

    QSet<QByteArray> acceptedTypes;
    const QList<QByteArray> acceptValues = request.header("accept").split(',');
    for (QByteArray value : acceptValues) {
        value = value.split(';').first().trimmed().toLower();
        acceptedTypes.insert(value);
    }
    if (!acceptedTypes.contains("application/json")
        || !acceptedTypes.contains("text/event-stream")) {
        HttpServer::Response response;
        response.statusCode = 406;
        response.contentType.clear();
        responder.write(response);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(request.body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        responder.write(jsonRpcError(QJsonValue(), -32700, QStringLiteral("Parse error.")));
        return;
    }

    const QJsonObject message = document.object();
    if (message.value("jsonrpc").toString() != QStringLiteral("2.0")
        || !message.value("method").isString()) {
        responder.write(
            jsonRpcError(message.value("id"), -32600, QStringLiteral("Invalid Request.")));
        return;
    }

    const QString method = message.value("method").toString();
    const bool hasId = message.contains("id");
    const QJsonValue id = message.value("id");
    if (!hasId) {
        HttpServer::Response accepted;
        accepted.statusCode = 202;
        accepted.contentType.clear();
        responder.write(accepted);
        return;
    }

    if (id.isNull() || id.isUndefined()) {
        responder.write(jsonRpcError(QJsonValue(), -32600, QStringLiteral("Invalid Request.")));
        return;
    }

    if (method == QStringLiteral("initialize")) {
        const QJsonObject params = message.value("params").toObject();
        const QString requestedVersion = params.value("protocolVersion").toString();
        if (requestedVersion.isEmpty()) {
            responder.write(jsonRpcError(id, -32602, QStringLiteral("Missing protocolVersion.")));
            return;
        }
        const QString protocolVersion = kSupportedProtocolVersions.contains(requestedVersion)
                                            ? requestedVersion
                                            : kCurrentProtocolVersion;
        QJsonObject toolsCapability;
        toolsCapability.insert("listChanged", false);
        QJsonObject capabilities;
        capabilities.insert("tools", toolsCapability);
        QJsonObject serverInfo;
        serverInfo.insert("name", QStringLiteral("Shotcut"));
        serverInfo.insert("version", QStringLiteral("1.0.0"));
        QJsonObject result;
        result.insert("protocolVersion", protocolVersion);
        result.insert("capabilities", capabilities);
        result.insert("serverInfo", serverInfo);
        responder.write(httpResponse(200, jsonRpcResult(id, result)));
        return;
    }

    const QByteArray protocolHeader = request.header("mcp-protocol-version");
    const QString protocolVersion = protocolHeader.isEmpty() ? QStringLiteral("2025-03-26")
                                                             : QString::fromLatin1(protocolHeader);
    if (!kSupportedProtocolVersions.contains(protocolVersion)) {
        HttpServer::Response response;
        response.statusCode = 400;
        response.contentType = QByteArrayLiteral("application/json");
        response.body = QJsonDocument(
                            jsonRpcErrorObject(id,
                                               -32600,
                                               QStringLiteral("Unsupported MCP protocol version.")))
                            .toJson(QJsonDocument::Compact);
        responder.write(response);
        return;
    }

    if (method == QStringLiteral("ping")) {
        responder.write(httpResponse(200, jsonRpcResult(id, QJsonObject())));
        return;
    }

    if (method == QStringLiteral("tools/call")) {
        const QJsonObject params = message.value("params").toObject();
        const auto toolName = params.value("name").toString();
        if (toolName == QStringLiteral("shotcut_forum_search")
            || toolName == QStringLiteral("shotcut_documentation_search")) {
            handleForumSearchRequest(responder, id, toolName, params.value("arguments").toObject());
            return;
        }
    }

    QString error;
    const QJsonObject result = handleRequest(message, &error);
    if (!error.isEmpty()) {
        const int code = method == QStringLiteral("tools/call") ? -32602 : -32601;
        responder.write(jsonRpcError(id, code, error));
        return;
    }
    responder.write(httpResponse(200, jsonRpcResult(id, result)));
}

void McpServer::handleForumSearchRequest(HttpServer::Responder responder,
                                         const QJsonValue &id,
                                         const QString &toolName,
                                         const QJsonObject &arguments)
{
    const QString query = arguments.value("query").toString().trimmed().left(200);
    if (query.isEmpty()) {
        responder.write(httpResponse(
            200,
            jsonRpcResult(id,
                          toolResult(QJsonObject{{"error",
                                                  QStringLiteral(
                                                      "The query argument is required.")}},
                                     true))));
        return;
    }

    auto *search = new ForumSearch(this);
    connect(search,
            &ForumSearch::resultsReady,
            this,
            [responder, id, query, search, toolName](const QList<ForumSource> &sources,
                                                     const QString &error) {
                QJsonObject result;
                result.insert("tool", toolName);
                result.insert("status",
                              sources.isEmpty() ? QStringLiteral("no_results")
                                                : QStringLiteral("ok"));
                result.insert("query", query);
                QJsonArray sourceArray;
                for (const ForumSource &source : sources) {
                    QJsonObject sourceObject;
                    sourceObject.insert("title", source.title);
                    sourceObject.insert("url", source.url);
                    sourceObject.insert("excerpt", source.excerpt);
                    sourceArray.append(sourceObject);
                }
                result.insert("sources", sourceArray);
                if (!error.isEmpty())
                    result.insert("error", error);

                responder.write(httpResponse(200, jsonRpcResult(id, toolResult(result))));
                search->deleteLater();
            });
    search->search(query,
                   toolName == QStringLiteral("shotcut_documentation_search")
                       ? ForumSearch::Scope::Documentation
                       : ForumSearch::Scope::Discussion);
}

QJsonObject McpServer::handleRequest(const QJsonObject &request, QString *error) const
{
    const QString method = request.value("method").toString();
    const QJsonValue id = request.value("id");
    Q_UNUSED(id)

    if (method == QStringLiteral("tools/list")) {
        QJsonArray tools;
        for (const ToolDefinition &definition : m_registry.tools()) {
            QJsonObject tool;
            tool["name"] = definition.name;
            tool["description"] = definition.description;
            tool["inputSchema"] = definition.parameters;
            tools.append(tool);
        }
        QJsonObject result;
        result["tools"] = tools;
        return result;
    }

    if (method == QStringLiteral("tools/call")) {
        const QJsonValue rawArguments = request.value("params").toObject().value("arguments");
        const QString toolName = request.value("params").toObject().value("name").toString();
        if (toolName.isEmpty()) {
            if (error)
                *error = QStringLiteral("Missing tool name.");
            return QJsonObject();
        }

        const QJsonObject arguments = rawArguments.isObject() ? rawArguments.toObject()
                                                              : QJsonObject();
        QString callError;
        const QJsonObject result = m_registry.call(toolName, arguments, &callError);
        if (!callError.isEmpty()) {
            if (error)
                *error = callError;
            return QJsonObject();
        }

        QJsonObject object;
        QJsonArray contentArray;
        QJsonObject textContent;
        textContent.insert("type", QStringLiteral("text"));
        textContent.insert("text",
                           QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)));
        contentArray.append(textContent);
        object.insert("content", contentArray);
        object.insert("structuredContent", result);
        return object;
    }

    if (error)
        *error = QStringLiteral("Unsupported MCP method: %1").arg(method);
    return QJsonObject();
}

} // namespace Mcp
} // namespace Shotcut
