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

#ifndef SHOTCUT_API_APICATALOG_H
#define SHOTCUT_API_APICATALOG_H

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <QVariantMap>

struct QMetaObject;
class QMetaMethod;

namespace Shotcut {
namespace Api {

// API documentation is declared with Q_CLASSINFO next to each member:
//   "doc"                  object description
//   "doc.<member>"         property or method description
//   "mcp.<method>"         "read", "edit", or "action". Reads and actions become MCP tools;
//                          edits and actions run through shotcut_edit. Actions (seek, play,
//                          select) do not create undo entries or change the project revision.
//   "arg.<method>.<name>"  "[type[!]|]description" for a parameter or, when the method's
//                          last parameter is a QVariantMap named "options", an option key.
//                          type is a JSON schema type: integer, number, boolean, string, object,
//                          array. "!" marks a required option key.

struct ParamInfo
{
    QString name;
    QString jsonType;
    QString itemType;
    QString description;
    bool required = false;
    bool isOption = false;
};

struct MemberInfo
{
    QString path;
    QString name;
    bool isProperty = false;
    bool isObject = false;
    bool takesOptions = false;
    bool optionsRequired = false;
    QString kind;
    QString description;
    QString type;
    QList<ParamInfo> params;
};

struct ObjectInfo
{
    QString path;
    QString description;
    QList<MemberInfo> members;
};

// Walks the API from the root meta-object; does not need instances.
QList<ObjectInfo> apiCatalog();
const MemberInfo *findMember(const QList<ObjectInfo> &catalog, const QString &path);

QList<ParamInfo> optionParams(const QMetaObject *metaObject, const QByteArray &method);
bool hasOptionsParam(const QMetaMethod &method);
QString jsonType(QMetaType type);
bool matchesJsonType(const QVariant &value, const QString &jsonType);

// e.g. "timeline.clipAt" -> "timeline_clip_at"; root members get a "shotcut_" prefix.
QString toolName(const QString &path);

QVariantMap toVariant(const MemberInfo &member);
QVariantMap toVariant(const ObjectInfo &object);

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APICATALOG_H
