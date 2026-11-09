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

#include "api/apiobject.h"

#include "api/apicatalog.h"
#include "api/transaction.h"

#include "Logger.h"

#include <QJSEngine>
#include <QJSValue>

#include <algorithm>

namespace Shotcut {
namespace Api {

namespace {
ApiObject::Error s_lastError;
}

int ApiObject::s_hostCallDepth = 0;

ApiObject::ApiObject(QObject *parent)
    : QObject(parent)
{}

ApiObject::Error ApiObject::lastError()
{
    return s_lastError;
}

void ApiObject::clearLastError()
{
    s_lastError = Error();
}

QVariant ApiObject::fail(const QString &code, const QString &message) const
{
    s_lastError = {code, message};
    auto &transaction = Transaction::singleton();
    if (transaction.isOpen() && transaction.isScriptMacro())
        transaction.fail(message);
    LOG_DEBUG() << metaObject()->className() << code << message;
    if (s_hostCallDepth == 0) {
        if (auto engine = qjsEngine(this)) {
            QJSValue error = engine->newErrorObject(QJSValue::GenericError, message);
            error.setProperty(QStringLiteral("code"), code);
            engine->throwError(error);
        }
    }
    return QVariant();
}

bool ApiObject::checkOptions(const char *method, const QVariantMap &options) const
{
    const auto params = optionParams(metaObject(), method);
    for (auto it = options.cbegin(); it != options.cend(); ++it) {
        const auto param = std::find_if(params.cbegin(), params.cend(), [&](const ParamInfo &p) {
            return p.name == it.key();
        });
        if (param == params.cend()) {
            QStringList names;
            for (const auto &p : params)
                names.append(p.name);
            fail(ErrorCode::InvalidArgument,
                 tr("Unknown option \"%1\" for %2. Valid options: %3.")
                     .arg(it.key(), QString::fromLatin1(method), names.join(", ")));
            return false;
        }
        if (!matchesJsonType(it.value(), param->jsonType)) {
            fail(ErrorCode::InvalidArgument,
                 tr("Option \"%1\" must be of type %2.").arg(it.key(), param->jsonType));
            return false;
        }
    }
    for (const auto &param : params) {
        if (param.required && !options.contains(param.name)) {
            fail(ErrorCode::InvalidArgument, tr("Option \"%1\" is required.").arg(param.name));
            return false;
        }
    }
    return true;
}

HostCallScope::HostCallScope()
{
    ++ApiObject::s_hostCallDepth;
    ApiObject::clearLastError();
}

HostCallScope::~HostCallScope()
{
    --ApiObject::s_hostCallDepth;
}

} // namespace Api
} // namespace Shotcut
