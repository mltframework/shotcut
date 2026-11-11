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

#include "mcp/mcpedittool.h"

#include "api/apicatalog.h"
#include "api/apiinvoker.h"
#include "api/apiobject.h"
#include "api/shotcutapi.h"
#include "api/transaction.h"
#include "mainwindow.h"
#include "mcp/mcpbridge.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QUndoStack>

namespace Shotcut {
namespace Mcp {

namespace {

using Api::apiCatalog;
using Api::findMember;
using Api::invokePath;
using Api::InvokeResult;
using Api::MemberInfo;
using Api::ObjectInfo;
using Api::ShotcutApi;
using Api::Transaction;
using Api::TransactionScope;
namespace ErrorCode = Api::ErrorCode;

constexpr int kMaxOps = 500;

struct Op
{
    QString path;
    QVariantMap args;
    QString as;
};

QJsonObject errorResponse(const QString &code, const QString &message, int opIndex = -1)
{
    QJsonObject response{{"status", "error"}, {"code", code}, {"message", message}};
    if (opIndex >= 0)
        response["opIndex"] = opIndex;
    response["revision"] = ShotcutApi::singleton().revision();
    return response;
}

// Returns the alias name referenced by a "$name..." string, or an empty string.
QString aliasName(const QString &text)
{
    if (!text.startsWith('$') || text.startsWith(QLatin1String("$$")))
        return QString();
    return text.mid(1).section('.', 0, 0);
}

bool findUndefinedAlias(const QVariant &value, const QSet<QString> &defined, QString *missing)
{
    switch (value.typeId()) {
    case QMetaType::QString: {
        const QString name = aliasName(value.toString());
        if (!name.isEmpty() && !defined.contains(name)) {
            *missing = name;
            return true;
        }
        return false;
    }
    case QMetaType::QVariantList:
        for (const auto &item : value.toList()) {
            if (findUndefinedAlias(item, defined, missing))
                return true;
        }
        return false;
    case QMetaType::QVariantMap: {
        const auto map = value.toMap();
        for (const auto &item : map) {
            if (findUndefinedAlias(item, defined, missing))
                return true;
        }
        return false;
    }
    default:
        return false;
    }
}

bool resolveReference(const QString &text,
                      const QVariantMap &aliases,
                      QVariant *resolved,
                      QString *message)
{
    const QStringList parts = text.mid(1).split('.');
    QVariant value = aliases.value(parts.first());
    for (int i = 1; i < parts.size(); ++i) {
        const QString &part = parts.at(i);
        if (value.typeId() == QMetaType::QVariantMap && value.toMap().contains(part)) {
            value = value.toMap().value(part);
            continue;
        }
        bool isIndex = false;
        const int index = part.toInt(&isIndex);
        if ((value.typeId() == QMetaType::QVariantList || value.typeId() == QMetaType::QStringList)
            && isIndex && index >= 0 && index < value.toList().size()) {
            value = value.toList().at(index);
            continue;
        }
        *message = QStringLiteral("\"%1\" does not resolve: \"%2\" not found.").arg(text, part);
        return false;
    }
    *resolved = value;
    return true;
}

bool resolve(const QVariant &value, const QVariantMap &aliases, QVariant *resolved, QString *message)
{
    switch (value.typeId()) {
    case QMetaType::QString: {
        const QString text = value.toString();
        if (text.startsWith(QLatin1String("$$"))) {
            *resolved = text.mid(1);
            return true;
        }
        if (text.startsWith('$'))
            return resolveReference(text, aliases, resolved, message);
        *resolved = value;
        return true;
    }
    case QMetaType::QVariantList: {
        QVariantList list;
        for (const auto &item : value.toList()) {
            QVariant itemResolved;
            if (!resolve(item, aliases, &itemResolved, message))
                return false;
            list.append(itemResolved);
        }
        *resolved = list;
        return true;
    }
    case QMetaType::QVariantMap: {
        QVariantMap map = value.toMap();
        for (auto it = map.begin(); it != map.end(); ++it) {
            QVariant itemResolved;
            if (!resolve(it.value(), aliases, &itemResolved, message))
                return false;
            it.value() = itemResolved;
        }
        *resolved = map;
        return true;
    }
    default:
        *resolved = value;
        return true;
    }
}

QString signature(const MemberInfo &member)
{
    QStringList params;
    for (const auto &param : member.params)
        params.append(param.required ? param.name : param.name + '?');
    return QStringLiteral("%1(%2)").arg(member.path, params.join(", "));
}

QString editDescription(const QList<ObjectInfo> &catalog)
{
    QStringList edits;
    QStringList actions;
    for (const auto &object : catalog) {
        for (const auto &member : object.members) {
            if (member.kind == QLatin1String("edit"))
                edits.append(signature(member));
            else if (member.kind == QLatin1String("action"))
                actions.append(member.path);
        }
    }
    return QStringLiteral(
               "Changes the project. Runs a list of API method calls (ops) in order as one "
               "undoable step; if any op fails, all of them are undone. Each op is "
               "{\"path\": \"markers.add\", \"args\": {...}, \"as\": \"name\"}. Pass arguments by "
               "name. Ops may also be read methods. A later op can use an earlier result named "
               "with \"as\": \"$name\" is the whole result and \"$name.field\" or \"$name.0\" "
               "reaches inside it; write \"$$\" for a literal \"$\". Use shotcut_capabilities for "
               "parameter details. Pass expectedRevision (from any earlier result) to fail with "
               "stale_revision if the project changed since. Edit methods: %1. Ops may also be "
               "actions, which are not undone or rolled back (e.g. to select or seek to a result "
               "at the end): %2.")
        .arg(edits.join(", "), actions.join(", "));
}

QJsonObject runOps(const QList<Op> &ops, const QString &label)
{
    TransactionScope scope(label);
    QVariantMap aliases;
    QJsonArray results;
    for (int i = 0; i < ops.size(); ++i) {
        const Op &op = ops.at(i);
        QVariant args;
        QString message;
        InvokeResult invoked;
        if (!resolve(op.args, aliases, &args, &message)) {
            invoked.errorCode = ErrorCode::InvalidArgument;
            invoked.errorMessage = message;
        } else {
            invoked = invokePath(&ShotcutApi::singleton(), op.path, args.toMap());
        }
        if (!invoked.ok) {
            scope.fail(invoked.errorMessage);
            scope.end();
            auto response = errorResponse(invoked.errorCode, invoked.errorMessage, i);
            response["rolledBack"] = true;
            return response;
        }
        if (!op.as.isEmpty())
            aliases[op.as] = invoked.value;
        results.append(
            QJsonObject{{"path", op.path}, {"result", QJsonValue::fromVariant(invoked.value)}});
    }
    const auto result = scope.end();
    QJsonObject response{{"status", "ok"}, {"results", results}};
    if (result == Transaction::Result::Committed)
        response["undoText"] = MAIN.undoStack()->undoText();
    response["revision"] = ShotcutApi::singleton().revision();
    return response;
}

} // namespace

ToolDefinition makeEditTool()
{
    const auto catalog = apiCatalog();
    ToolDefinition tool;
    tool.name = QStringLiteral("shotcut_edit");
    tool.description = editDescription(catalog);
    QJsonObject opSchema{
        {"type", "object"},
        {"properties",
         QJsonObject{{"path",
                      QJsonObject{{"type", "string"},
                                  {"description", "API method path, e.g. \"markers.add\"."}}},
                     {"args",
                      QJsonObject{{"type", "object"},
                                  {"description", "Named arguments; may contain $references."}}},
                     {"as",
                      QJsonObject{{"type", "string"},
                                  {"description", "Name for this op's result."}}}}},
        {"required", QJsonArray{"path"}}};
    tool.parameters = QJsonObject{
        {"type", "object"},
        {"properties",
         QJsonObject{{"ops",
                      QJsonObject{{"type", "array"},
                                  {"items", opSchema},
                                  {"description", "Method calls to run in order."}}},
                     {"label",
                      QJsonObject{{"type", "string"},
                                  {"description", "Undo history text; defaults to a summary."}}},
                     {"expectedRevision",
                      QJsonObject{{"type", "integer"},
                                  {"description", "Fail if the project revision differs."}}}}},
        {"required", QJsonArray{"ops"}}};

    tool.handler = [](const QJsonObject &arguments) {
        const auto opsValue = arguments.value("ops");
        if (!opsValue.isArray() || opsValue.toArray().isEmpty())
            return errorResponse(ErrorCode::InvalidArgument,
                                 QStringLiteral("\"ops\" must be a non-empty array."));
        const QJsonArray opsArray = opsValue.toArray();
        if (opsArray.size() > kMaxOps)
            return errorResponse(ErrorCode::InvalidArgument,
                                 QStringLiteral("At most %1 ops are allowed per call.").arg(kMaxOps));

        if (arguments.contains("expectedRevision")) {
            const int expected = arguments.value("expectedRevision").toInt(-1);
            const int current = ShotcutApi::singleton().revision();
            if (expected != current)
                return errorResponse(ErrorCode::StaleRevision,
                                     QStringLiteral("The project changed (revision %1, expected "
                                                    "%2). Read it again before editing.")
                                         .arg(current)
                                         .arg(expected));
        }
        if (Transaction::singleton().isOpen())
            return errorResponse(ErrorCode::Busy,
                                 QStringLiteral("A script is changing the project. Try again."));

        const auto catalog = apiCatalog();
        QList<Op> ops;
        QSet<QString> defined;
        for (int i = 0; i < opsArray.size(); ++i) {
            const QJsonObject opObject = opsArray.at(i).toObject();
            Op op;
            op.path = opObject.value("path").toString();
            op.as = opObject.value("as").toString();
            const auto argsValue = opObject.value("args");
            if (!argsValue.isUndefined() && !argsValue.isObject())
                return errorResponse(ErrorCode::InvalidArgument,
                                     QStringLiteral("\"args\" must be an object."),
                                     i);
            op.args = argsValue.toObject().toVariantMap();

            const MemberInfo *member = findMember(catalog, op.path);
            if (!member || member->isProperty
                || (member->kind != QLatin1String("edit") && member->kind != QLatin1String("read")
                    && member->kind != QLatin1String("action")))
                return errorResponse(ErrorCode::NotFound,
                                     QStringLiteral("\"%1\" is not an API method available to "
                                                    "shotcut_edit.")
                                         .arg(op.path),
                                     i);
            QString missing;
            if (findUndefinedAlias(op.args, defined, &missing))
                return errorResponse(ErrorCode::InvalidArgument,
                                     QStringLiteral("\"$%1\" is not defined by an earlier op.")
                                         .arg(missing),
                                     i);
            if (!op.as.isEmpty()) {
                if (op.as.contains('.') || op.as.startsWith('$') || defined.contains(op.as))
                    return errorResponse(ErrorCode::InvalidArgument,
                                         QStringLiteral("\"as\": \"%1\" must be a unique name "
                                                        "without \".\" or \"$\".")
                                             .arg(op.as),
                                         i);
                defined.insert(op.as);
            }
            ops.append(op);
        }
        auto response = runOps(ops, arguments.value("label").toString());
        if (limitSize(response).value("status") != response.value("status")) {
            response.remove("results");
            response["message"] = QStringLiteral("The edit succeeded, but its results were too "
                                                 "large to return. Read them separately.");
        }
        return response;
    };
    return tool;
}

} // namespace Mcp
} // namespace Shotcut
