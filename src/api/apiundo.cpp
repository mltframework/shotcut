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

#include "api/apiundo.h"

#include "api/transaction.h"

#include <QJSEngine>

namespace Shotcut {
namespace Api {

ApiUndo::ApiUndo(QObject *parent)
    : ApiObject(parent)
{}

QJSValue ApiUndo::macro(const QString &text, const QJSValue &callback)
{
    auto *engine = qjsEngine(this);
    if (!engine || !callback.isCallable()) {
        fail(ErrorCode::InvalidArgument, tr("Give a synchronous JavaScript callback."));
        return {};
    }
    if (callback.property("constructor").property("name").toString()
        == QLatin1String("AsyncFunction")) {
        fail(ErrorCode::InvalidArgument, tr("Undo callbacks must not be async functions."));
        return {};
    }
    auto &transaction = Transaction::singleton();
    if (transaction.isOpen() && !transaction.isAuto() && !running()) {
        fail(ErrorCode::Busy, tr("Another edit transaction is already running."));
        return {};
    }
    ApiCallGuard callGuard;
    transaction.begin(text, Transaction::Origin::Script);
    ++m_depth;
    if (m_depth == 1)
        emit runningChanged();

    const QJSValue invoke = engine->evaluate(
        QStringLiteral("(function(callback) {"
                       "  try {"
                       "    const value = callback();"
                       "    if (value && typeof value.then === 'function')"
                       "      throw new Error('Undo callbacks must not return a Promise.');"
                       "    return {ok: true, value: value};"
                       "  } catch (error) { return {ok: false, error: error}; }"
                       "})"));
    const QJSValue outcome = invoke.call({callback});
    const bool threw = !outcome.property("ok").toBool();
    const QJSValue result = outcome.property(threw ? "error" : "value");
    if (threw)
        transaction.fail(result.toString());
    const bool failed = transaction.hasFailed();
    const QString reason = transaction.failureReason();
    transaction.end();
    --m_depth;
    if (m_depth == 0)
        emit runningChanged();

    if (threw) {
        engine->throwError(result);
        return {};
    }
    if (failed) {
        fail(ErrorCode::InvalidArgument, reason);
        return {};
    }
    return result;
}

} // namespace Api
} // namespace Shotcut
