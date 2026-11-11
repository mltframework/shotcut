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

#include "api/apiinvoker.h"

#include "api/apicatalog.h"
#include "api/apiobject.h"
#include "api/transaction.h"

#include <QMetaMethod>
#include <QMetaProperty>
#include <QObject>

#include <vector>

namespace Shotcut {
namespace Api {

namespace {

InvokeResult error(const QString &code, const QString &message)
{
    InvokeResult result;
    result.errorCode = code;
    result.errorMessage = message;
    return result;
}

QObject *objectValue(const QVariant &value)
{
    return value.metaType().flags().testFlag(QMetaType::PointerToQObject) ? value.value<QObject *>()
                                                                          : nullptr;
}

QVariant presentable(const QVariant &value)
{
    if (auto object = objectValue(value))
        return describe(object);
    return value;
}

bool isApiMethod(const QMetaMethod &method)
{
    return method.methodType() == QMetaMethod::Method && method.access() == QMetaMethod::Public;
}

} // namespace

QVariantMap describe(QObject *object)
{
    QVariantMap result;
    if (!object)
        return result;
    const QMetaObject *mo = object->metaObject();
    result["className"] = QString::fromLatin1(mo->className());

    QVariantMap properties;
    for (int i = QObject::staticMetaObject.propertyCount(); i < mo->propertyCount(); ++i) {
        const QMetaProperty property = mo->property(i);
        const QVariant value = property.read(object);
        if (auto child = objectValue(value))
            properties[property.name()] = QStringLiteral("<%1>").arg(
                child->metaObject()->className());
        else
            properties[property.name()] = value;
    }
    result["properties"] = properties;

    QVariantList methods;
    for (int i = QObject::staticMetaObject.methodCount(); i < mo->methodCount(); ++i) {
        const QMetaMethod method = mo->method(i);
        if (!isApiMethod(method) || method.attributes() & QMetaMethod::Cloned)
            continue;
        QVariantList parameters;
        const auto names = method.parameterNames();
        for (int p = 0; p < method.parameterCount(); ++p) {
            parameters.append(
                QVariantMap{{"name", QString::fromLatin1(names.value(p))},
                            {"type", QString::fromLatin1(method.parameterTypeName(p))}});
        }
        methods.append(QVariantMap{{"name", QString::fromLatin1(method.name())},
                                   {"returnType", QString::fromLatin1(method.typeName())},
                                   {"parameters", parameters}});
    }
    result["methods"] = methods;
    return result;
}

InvokeResult invokePath(QObject *root, const QString &path, const QVariantList &args)
{
    return invokePath(root, path, QVariant(args));
}

InvokeResult invokePath(QObject *root, const QString &path, const QVariantMap &namedArgs)
{
    return invokePath(root, path, QVariant(namedArgs));
}

namespace {

InvokeResult invokeMethod(QObject *object,
                          const QString &path,
                          const QMetaMethod &method,
                          const QVariantList &args);

// Picks the overload with the most parameters; moc emits one clone per defaulted parameter.
bool findMethod(const QMetaObject *mo, const QString &name, QMetaMethod *full, int *minCount)
{
    bool found = false;
    for (int i = QObject::staticMetaObject.methodCount(); i < mo->methodCount(); ++i) {
        const QMetaMethod candidate = mo->method(i);
        if (!isApiMethod(candidate) || QString::fromLatin1(candidate.name()) != name)
            continue;
        if (!found || candidate.parameterCount() > full->parameterCount())
            *full = candidate;
        *minCount = found ? qMin(*minCount, candidate.parameterCount())
                          : candidate.parameterCount();
        found = true;
    }
    return found;
}

QMetaMethod overloadWithCount(const QMetaObject *mo, const QString &name, int count)
{
    for (int i = QObject::staticMetaObject.methodCount(); i < mo->methodCount(); ++i) {
        const QMetaMethod candidate = mo->method(i);
        if (isApiMethod(candidate) && QString::fromLatin1(candidate.name()) == name
            && candidate.parameterCount() == count)
            return candidate;
    }
    return QMetaMethod();
}

QVariantList positionalArgs(const QString &path,
                            const QMetaMethod &full,
                            int minCount,
                            const QVariantMap &namedArgs,
                            InvokeResult *errorResult)
{
    const auto names = full.parameterNames();
    const bool hasOptions = hasOptionsParam(full);
    const int optionsIndex = hasOptions ? full.parameterCount() - 1 : -1;

    QStringList known;
    for (int p = 0; p < full.parameterCount(); ++p) {
        if (p != optionsIndex)
            known.append(QString::fromLatin1(names.value(p)));
    }
    // Option keys may be passed alongside the positional parameters or nested under "options".
    QVariantMap options;
    for (auto it = namedArgs.cbegin(); it != namedArgs.cend(); ++it) {
        if (known.contains(it.key()))
            continue;
        if (!hasOptions) {
            *errorResult = error(ErrorCode::InvalidArgument,
                                 QStringLiteral("Unknown argument \"%1\" for \"%2\". Expected: %3.")
                                     .arg(it.key(), path, known.join(", ")));
            return {};
        }
        if (it.key() == QLatin1String("options") && it.value().typeId() == QMetaType::QVariantMap)
            options.insert(it.value().toMap());
        else
            options.insert(it.key(), it.value());
    }

    int count = minCount;
    for (int p = 0; p < known.size(); ++p) {
        if (namedArgs.contains(known.at(p)))
            count = qMax(count, p + 1);
    }
    if (hasOptions && !options.isEmpty())
        count = full.parameterCount();

    QVariantList args;
    for (int p = 0; p < count; ++p) {
        if (p == optionsIndex) {
            args.append(options);
            continue;
        }
        if (!namedArgs.contains(known.at(p))) {
            *errorResult = error(ErrorCode::InvalidArgument,
                                 QStringLiteral("Missing argument \"%1\" for \"%2\".")
                                     .arg(known.at(p), path));
            return {};
        }
        args.append(namedArgs.value(known.at(p)));
    }
    return args;
}

} // namespace

InvokeResult invokePath(QObject *root, const QString &path, const QVariant &args)
{
    if (!root)
        return error(ErrorCode::Unavailable, QStringLiteral("The API is not available."));

    QObject *object = root;
    const QStringList segments = path.split('.', Qt::SkipEmptyParts);
    for (int i = 0; i + 1 < segments.size(); ++i) {
        const int index = object->metaObject()->indexOfProperty(segments.at(i).toLatin1());
        QObject *child = index >= 0
                             ? objectValue(object->metaObject()->property(index).read(object))
                             : nullptr;
        if (!child)
            return error(ErrorCode::NotFound,
                         QStringLiteral("\"%1\" is not an API object.")
                             .arg(segments.mid(0, i + 1).join('.')));
        object = child;
    }

    const bool named = args.typeId() == QMetaType::QVariantMap;
    const bool noArgs = named ? args.toMap().isEmpty() : args.toList().isEmpty();

    InvokeResult result;
    if (segments.isEmpty()) {
        result.ok = true;
        result.value = describe(object);
        return result;
    }

    const QString name = segments.last();
    const QMetaObject *mo = object->metaObject();

    const int propertyIndex = mo->indexOfProperty(name.toLatin1());
    if (propertyIndex >= QObject::staticMetaObject.propertyCount()) {
        if (!noArgs)
            return error(ErrorCode::InvalidArgument,
                         QStringLiteral("\"%1\" is a property and takes no arguments.").arg(path));
        result.ok = true;
        result.value = presentable(mo->property(propertyIndex).read(object));
        return result;
    }

    QMetaMethod full;
    int minCount = 0;
    if (!findMethod(mo, name, &full, &minCount))
        return error(ErrorCode::NotFound, QStringLiteral("\"%1\" does not exist.").arg(path));

    QVariantList positional;
    if (named) {
        InvokeResult argError;
        positional = positionalArgs(path, full, minCount, args.toMap(), &argError);
        if (!argError.errorCode.isEmpty())
            return argError;
    } else {
        positional = args.toList();
    }
    const QMetaMethod method = overloadWithCount(mo, name, positional.size());
    if (!method.isValid())
        return error(ErrorCode::InvalidArgument,
                     QStringLiteral("Wrong number of arguments for \"%1\".").arg(path));
    return invokeMethod(object, path, method, positional);
}

namespace {

InvokeResult invokeMethod(QObject *object,
                          const QString &path,
                          const QMetaMethod &method,
                          const QVariantList &args)
{
    InvokeResult result;
    std::vector<QVariant> converted;
    converted.reserve(args.size());
    for (int i = 0; i < args.size(); ++i) {
        QVariant value = args.at(i);
        const QMetaType target = method.parameterMetaType(i);
        if (target.id() != QMetaType::QVariant && value.metaType() != target
            && !value.convert(target)) {
            return error(ErrorCode::InvalidArgument,
                         QStringLiteral("Argument %1 of \"%2\" must be %3.")
                             .arg(i + 1)
                             .arg(path, QString::fromLatin1(target.name())));
        }
        converted.push_back(value);
    }

    const QMetaType returnType = method.returnMetaType();
    const bool returnsVoid = returnType.id() == QMetaType::Void;
    QVariant returnValue = returnsVoid ? QVariant() : QVariant(returnType, nullptr);

    std::vector<void *> argv;
    argv.push_back(returnsVoid ? nullptr : returnValue.data());
    for (int i = 0; i < args.size(); ++i) {
        // A QVariant parameter receives the QVariant itself, not its payload.
        if (method.parameterMetaType(i).id() == QMetaType::QVariant)
            argv.push_back(&converted[i]);
        else
            argv.push_back(converted[i].data());
    }

    {
        HostCallScope hostCall;
        ApiCallGuard callGuard;
        QMetaObject::metacall(object,
                              QMetaObject::InvokeMetaMethod,
                              method.methodIndex(),
                              argv.data());
        const auto apiError = ApiObject::lastError();
        if (apiError.isSet())
            return error(apiError.code, apiError.message);
    }

    if (returnType.id() == QMetaType::QVariant)
        returnValue = returnValue.value<QVariant>();
    result.ok = true;
    result.value = presentable(returnValue);
    return result;
}

} // namespace

} // namespace Api
} // namespace Shotcut
