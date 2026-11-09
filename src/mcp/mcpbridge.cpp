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

#include "mcp/mcpbridge.h"

#include "api/apicatalog.h"
#include "api/apiinvoker.h"
#include "api/apiobject.h"
#include "api/shotcutapi.h"
#include "mcp/mcpedittool.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>

namespace Shotcut {
namespace Mcp {

namespace {

using Api::apiCatalog;
using Api::findMember;
using Api::HostCallScope;
using Api::invokePath;
using Api::InvokeResult;
using Api::MemberInfo;
using Api::ParamInfo;
using Api::ShotcutApi;
using Api::toolName;
using Api::toVariant;
namespace ErrorCode = Api::ErrorCode;

// Keeps a single result within a model's context budget (roughly 25k tokens).
constexpr qsizetype kMaxResultBytes = 100000;

} // namespace

QJsonObject limitSize(const QJsonObject &response)
{
    const auto size = QJsonDocument(response).toJson(QJsonDocument::Compact).size();
    if (size <= kMaxResultBytes)
        return response;
    return QJsonObject{{"status", "error"},
                       {"code", ErrorCode::ResultTooLarge},
                       {"message",
                        QStringLiteral("The result is %1 KB, over the %2 KB limit. "
                                       "Use the tool's options to request less.")
                            .arg(size / 1000)
                            .arg(kMaxResultBytes / 1000)},
                       {"revision", ShotcutApi::singleton().revision()}};
}

namespace {

QJsonObject respond(const InvokeResult &invoked)
{
    QJsonObject response;
    if (invoked.ok) {
        response["status"] = QStringLiteral("ok");
        response["result"] = QJsonValue::fromVariant(invoked.value);
    } else {
        response["status"] = QStringLiteral("error");
        response["code"] = invoked.errorCode;
        response["message"] = invoked.errorMessage;
    }
    response["revision"] = ShotcutApi::singleton().revision();
    return limitSize(response);
}

QVariantMap state(QObject *object)
{
    QVariantMap result;
    const QMetaObject *mo = object->metaObject();
    for (int i = QObject::staticMetaObject.propertyCount(); i < mo->propertyCount(); ++i) {
        const QMetaProperty property = mo->property(i);
        const QVariant value = property.read(object);
        if (value.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
            if (auto child = value.value<QObject *>())
                result[property.name()] = state(child);
        } else {
            result[property.name()] = value;
        }
    }
    return result;
}

QJsonObject inputSchema(const QList<ParamInfo> &params)
{
    QJsonObject properties;
    QJsonArray required;
    for (const auto &param : params) {
        QJsonObject schema;
        if (!param.jsonType.isEmpty())
            schema["type"] = param.jsonType;
        // OpenAI rejects array schemas without "items".
        if (param.jsonType == QLatin1String("array"))
            schema["items"] = param.itemType.isEmpty() ? QJsonObject()
                                                       : QJsonObject{{"type", param.itemType}};
        if (!param.description.isEmpty())
            schema["description"] = param.description;
        properties[param.name] = schema;
        if (param.required)
            required.append(param.name);
    }
    return QJsonObject{{"type", "object"}, {"properties", properties}, {"required", required}};
}

ToolDefinition makeStateTool()
{
    ToolDefinition tool;
    tool.name = QStringLiteral("shotcut_state");
    tool.description = QStringLiteral(
        "Returns the live state of Shotcut: the project file and video mode, the player, and "
        "summaries of the timeline, playlist, and markers. Call this first for questions about "
        "the open project, then use the timeline_, playlist_, and markers_ tools for details. "
        "All times are frames at project.profile.fps.");
    tool.parameters = inputSchema({});
    tool.handler = [](const QJsonObject &) {
        InvokeResult result;
        result.ok = true;
        HostCallScope hostCall;
        result.value = state(&ShotcutApi::singleton());
        return respond(result);
    };
    return tool;
}

ToolDefinition makeCapabilitiesTool()
{
    ToolDefinition tool;
    tool.name = QStringLiteral("shotcut_capabilities");
    tool.description = QStringLiteral(
        "Describes the Shotcut scripting API: objects, properties, and methods with their "
        "parameters, and the MCP tool for each method. Omit path for everything, or pass an "
        "object path such as \"timeline\" or a member path such as \"timeline.clips\".");
    ParamInfo path;
    path.name = QStringLiteral("path");
    path.jsonType = QStringLiteral("string");
    path.description = QStringLiteral("Object or member path; omit for the whole API.");
    tool.parameters = inputSchema({path});
    tool.handler = [](const QJsonObject &arguments) {
        const QString path = arguments.value("path").toString();
        const auto catalog = apiCatalog();
        InvokeResult result;
        if (path.isEmpty()) {
            QVariantList objects;
            for (const auto &object : catalog)
                objects.append(toVariant(object));
            result.ok = true;
            result.value = objects;
            return respond(result);
        }
        for (const auto &object : catalog) {
            if (object.path == path) {
                result.ok = true;
                result.value = toVariant(object);
                return respond(result);
            }
        }
        if (auto member = findMember(catalog, path)) {
            result.ok = true;
            result.value = toVariant(*member);
            return respond(result);
        }
        result.errorCode = ErrorCode::NotFound;
        result.errorMessage = QStringLiteral("\"%1\" is not part of the API.").arg(path);
        return respond(result);
    };
    return tool;
}

ToolDefinition makeMethodTool(const MemberInfo &member)
{
    ToolDefinition tool;
    tool.name = toolName(member.path);
    tool.description = member.description + QStringLiteral(" (API: %1)").arg(member.path);
    tool.parameters = inputSchema(member.params);
    const QString path = member.path;
    tool.handler = [path](const QJsonObject &arguments) {
        return respond(invokePath(&ShotcutApi::singleton(), path, arguments.toVariantMap()));
    };
    return tool;
}

} // namespace

QList<ToolDefinition> makeApiTools()
{
    QList<ToolDefinition> tools{makeStateTool(), makeCapabilitiesTool(), makeEditTool()};
    for (const auto &object : apiCatalog()) {
        for (const auto &member : object.members) {
            if (member.kind == QLatin1String("read") || member.kind == QLatin1String("action"))
                tools.append(makeMethodTool(member));
        }
    }
    return tools;
}

} // namespace Mcp
} // namespace Shotcut
