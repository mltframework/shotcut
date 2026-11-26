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

#include "api/apicatalog.h"

#include "api/shotcutapi.h"

#include <QHash>
#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaProperty>

#include <cmath>

namespace Shotcut {
namespace Api {

namespace {

QString classInfo(const QMetaObject *metaObject, const QByteArray &key)
{
    const int index = metaObject->indexOfClassInfo(key.constData());
    return index >= 0 ? QString::fromUtf8(metaObject->classInfo(index).value()) : QString();
}

void splitTyped(const QString &value, QString *type, QString *description, bool *required = nullptr)
{
    static const QStringList kTypes{"integer", "number", "boolean", "string", "object", "array"};
    const int bar = value.indexOf('|');
    QString token = bar > 0 ? value.left(bar) : QString();
    const bool bang = token.endsWith('!');
    if (bang)
        token.chop(1);
    if (kTypes.contains(token)) {
        *type = token;
        *description = value.mid(bar + 1);
        if (required && bang)
            *required = true;
    } else {
        *description = value;
    }
}

QString joinPath(const QString &parent, const QString &name)
{
    return parent.isEmpty() ? name : parent + '.' + name;
}

bool isApiMethod(const QMetaMethod &method)
{
    return method.methodType() == QMetaMethod::Method && method.access() == QMetaMethod::Public;
}

void addObject(QList<ObjectInfo> &catalog, const QMetaObject *metaObject, const QString &path)
{
    ObjectInfo info;
    info.path = path;
    info.description = classInfo(metaObject, "doc");
    QList<QPair<const QMetaObject *, QString>> children;

    for (int i = QObject::staticMetaObject.propertyCount(); i < metaObject->propertyCount(); ++i) {
        const QMetaProperty property = metaObject->property(i);
        MemberInfo member;
        member.name = QString::fromLatin1(property.name());
        member.path = joinPath(path, member.name);
        member.isProperty = true;
        member.description = classInfo(metaObject, "doc." + QByteArray(property.name()));
        const QMetaType type = property.metaType();
        if (type.flags().testFlag(QMetaType::PointerToQObject) && type.metaObject()) {
            member.isObject = true;
            member.type = QStringLiteral("object");
            children.append({type.metaObject(), member.path});
        } else {
            member.type = jsonType(type);
        }
        info.members.append(member);
    }

    QStringList names;
    QHash<QString, QMetaMethod> fullMethods;
    QHash<QString, int> minCounts;
    for (int i = QObject::staticMetaObject.methodCount(); i < metaObject->methodCount(); ++i) {
        const QMetaMethod method = metaObject->method(i);
        if (!isApiMethod(method))
            continue;
        const QString name = QString::fromLatin1(method.name());
        if (!fullMethods.contains(name)) {
            names.append(name);
            fullMethods.insert(name, method);
            minCounts.insert(name, method.parameterCount());
            continue;
        }
        if (method.parameterCount() > fullMethods.value(name).parameterCount())
            fullMethods.insert(name, method);
        minCounts[name] = qMin(minCounts.value(name), method.parameterCount());
    }

    for (const QString &name : names) {
        const QMetaMethod method = fullMethods.value(name);
        const QByteArray key = name.toLatin1();
        MemberInfo member;
        member.name = name;
        member.path = joinPath(path, name);
        member.kind = classInfo(metaObject, "mcp." + key);
        member.description = classInfo(metaObject, "doc." + key);
        member.type = jsonType(method.returnMetaType());
        if (member.type.isEmpty() && method.returnMetaType().id() != QMetaType::Void)
            member.type = QStringLiteral("any");
        const auto parameterNames = method.parameterNames();
        member.takesOptions = hasOptionsParam(method);
        const int positionalCount = method.parameterCount() - (member.takesOptions ? 1 : 0);
        member.optionsRequired = member.takesOptions && minCounts.value(name) > positionalCount;
        for (int p = 0; p < positionalCount; ++p) {
            ParamInfo param;
            param.name = QString::fromLatin1(parameterNames.value(p));
            param.jsonType = jsonType(method.parameterMetaType(p));
            if (method.parameterMetaType(p).id() == QMetaType::QStringList)
                param.itemType = QStringLiteral("string");
            splitTyped(classInfo(metaObject, "arg." + key + '.' + parameterNames.value(p)),
                       &param.jsonType,
                       &param.description);
            param.required = p < minCounts.value(name);
            member.params.append(param);
        }
        if (member.takesOptions)
            member.params.append(optionParams(metaObject, key));
        info.members.append(member);
    }

    catalog.append(info);
    for (const auto &child : children)
        addObject(catalog, child.first, child.second);
}

QVariantList paramsToVariant(const QList<ParamInfo> &params, bool options)
{
    QVariantList result;
    for (const auto &param : params) {
        if (param.isOption != options)
            continue;
        QVariantMap map{{"name", param.name}, {"required", param.required}};
        if (!param.jsonType.isEmpty())
            map["type"] = param.jsonType;
        if (!param.itemType.isEmpty())
            map["itemType"] = param.itemType;
        if (!param.description.isEmpty())
            map["description"] = param.description;
        result.append(map);
    }
    return result;
}

} // namespace

QList<ObjectInfo> apiCatalog()
{
    QList<ObjectInfo> catalog;
    addObject(catalog, &ShotcutApi::staticMetaObject, QString());
    return catalog;
}

const MemberInfo *findMember(const QList<ObjectInfo> &catalog, const QString &path)
{
    for (const auto &object : catalog) {
        for (const auto &member : object.members) {
            if (member.path == path)
                return &member;
        }
    }
    return nullptr;
}

QList<ParamInfo> optionParams(const QMetaObject *metaObject, const QByteArray &method)
{
    QList<QByteArray> positional;
    for (int i = QObject::staticMetaObject.methodCount(); i < metaObject->methodCount(); ++i) {
        const QMetaMethod candidate = metaObject->method(i);
        if (candidate.name() == method)
            positional.append(candidate.parameterNames());
    }
    QList<ParamInfo> result;
    const QByteArray prefix = "arg." + method + '.';
    for (int i = 0; i < metaObject->classInfoCount(); ++i) {
        const QByteArray name = metaObject->classInfo(i).name();
        if (!name.startsWith(prefix) || positional.contains(name.mid(prefix.size())))
            continue;
        ParamInfo param;
        param.name = QString::fromLatin1(name.mid(prefix.size()));
        param.isOption = true;
        splitTyped(QString::fromUtf8(metaObject->classInfo(i).value()),
                   &param.jsonType,
                   &param.description,
                   &param.required);
        result.append(param);
    }
    return result;
}

bool hasOptionsParam(const QMetaMethod &method)
{
    const int last = method.parameterCount() - 1;
    return last >= 0 && method.parameterNames().value(last) == "options"
           && method.parameterMetaType(last).id() == QMetaType::QVariantMap;
}

QString jsonType(QMetaType type)
{
    switch (type.id()) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return QStringLiteral("integer");
    case QMetaType::Double:
    case QMetaType::Float:
        return QStringLiteral("number");
    case QMetaType::Bool:
        return QStringLiteral("boolean");
    case QMetaType::QString:
        return QStringLiteral("string");
    case QMetaType::QVariantMap:
        return QStringLiteral("object");
    case QMetaType::QVariantList:
    case QMetaType::QStringList:
        return QStringLiteral("array");
    default:
        return QString();
    }
}

bool matchesJsonType(const QVariant &value, const QString &jsonType)
{
    switch (value.typeId()) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return jsonType == "integer" || jsonType == "number" || jsonType.isEmpty();
    case QMetaType::Double:
    case QMetaType::Float:
        return jsonType == "number" || jsonType.isEmpty()
               || (jsonType == "integer" && std::trunc(value.toDouble()) == value.toDouble());
    case QMetaType::Bool:
        return jsonType == "boolean" || jsonType.isEmpty();
    case QMetaType::QString:
        return jsonType == "string" || jsonType.isEmpty();
    case QMetaType::QVariantMap:
        return jsonType == "object" || jsonType.isEmpty();
    case QMetaType::QVariantList:
    case QMetaType::QStringList:
        return jsonType == "array" || jsonType.isEmpty();
    default:
        return jsonType.isEmpty();
    }
}

QString toolName(const QString &path)
{
    QStringList parts;
    for (const QString &segment : path.split('.', Qt::SkipEmptyParts)) {
        QString snake;
        for (const QChar c : segment) {
            if (c.isUpper()) {
                snake += '_';
                snake += c.toLower();
            } else {
                snake += c;
            }
        }
        parts.append(snake);
    }
    if (parts.size() == 1)
        parts.prepend(QStringLiteral("shotcut"));
    return parts.join('_');
}

QVariantMap toVariant(const MemberInfo &member)
{
    QVariantMap result{{"path", member.path}};
    if (!member.description.isEmpty())
        result["description"] = member.description;
    if (!member.type.isEmpty())
        result["type"] = member.type;
    if (member.isProperty)
        return result;
    if (!member.kind.isEmpty()) {
        result["kind"] = member.kind;
        if (member.kind == QLatin1String("read") || member.kind == QLatin1String("action"))
            result["tool"] = toolName(member.path);
        else if (member.kind == QLatin1String("edit"))
            result["tool"] = QStringLiteral("shotcut_edit");
    }
    result["params"] = paramsToVariant(member.params, false);
    if (member.takesOptions) {
        result["options"] = paramsToVariant(member.params, true);
        result["optionsRequired"] = member.optionsRequired;
    }
    return result;
}

QVariantMap toVariant(const ObjectInfo &object)
{
    QVariantList properties;
    QVariantList methods;
    for (const auto &member : object.members) {
        if (member.isProperty)
            properties.append(toVariant(member));
        else
            methods.append(toVariant(member));
    }
    QVariantMap result{{"path", object.path}, {"properties", properties}, {"methods", methods}};
    if (!object.description.isEmpty())
        result["description"] = object.description;
    return result;
}

} // namespace Api
} // namespace Shotcut
