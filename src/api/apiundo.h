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

#ifndef SHOTCUT_API_APIUNDO_H
#define SHOTCUT_API_APIUNDO_H

#include "api/apiobject.h"

#include <QJSValue>

namespace Shotcut {
namespace Api {

class ApiUndo : public ApiObject
{
    Q_OBJECT
    Q_CLASSINFO("doc", "Explicit synchronous script undo grouping. MCP uses shotcut_edit instead.")
    Q_CLASSINFO("doc.running", "Whether a script undo callback is executing.")
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit ApiUndo(QObject *parent = nullptr);
    bool running() const { return m_depth > 0; }

    Q_CLASSINFO("doc.macro",
                "Calls a synchronous JavaScript callback and returns its result. Edits form one "
                "Script: undo entry. An exception or API error rolls back the entire group, "
                "even if caught inside the callback. Nested macros share the outer group. "
                "Promises and async functions are not supported. QML only; MCP uses shotcut_edit.")
    Q_CLASSINFO("arg.macro.text", "Undo label, without the Script: prefix.")
    Q_CLASSINFO("arg.macro.callback", "object|Synchronous JavaScript function containing the edits.")
    Q_INVOKABLE QJSValue macro(const QString &text, const QJSValue &callback);

signals:
    void runningChanged();

private:
    int m_depth = 0;
};

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APIUNDO_H
