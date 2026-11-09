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

#ifndef SHOTCUT_API_APIOBJECT_H
#define SHOTCUT_API_APIOBJECT_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

namespace Shotcut {
namespace Api {

// Base class for objects in the scripting API. The same instances serve QML
// scripts directly and MCP tools through reflection, so errors are recorded
// here and only thrown as JavaScript exceptions when called from QML.
class ApiObject : public QObject
{
    Q_OBJECT

public:
    struct Error
    {
        QString code;
        QString message;
        bool isSet() const { return !code.isEmpty(); }
    };

    explicit ApiObject(QObject *parent = nullptr);

    static Error lastError();
    static void clearLastError();

protected:
    QVariant fail(const QString &code, const QString &message) const;
    // Validates option keys and types against the "arg.<method>.<key>" class info.
    bool checkOptions(const char *method, const QVariantMap &options) const;

private:
    friend class HostCallScope;
    static int s_hostCallDepth;
};

// Marks a call made by C++ (e.g. the MCP bridge) rather than by a QML script:
// clears the last error on entry and suppresses JavaScript exceptions.
class HostCallScope
{
public:
    HostCallScope();
    ~HostCallScope();
    Q_DISABLE_COPY(HostCallScope)
};

namespace ErrorCode {
inline const QString InvalidArgument = QStringLiteral("invalid_argument");
inline const QString NotFound = QStringLiteral("not_found");
inline const QString Unavailable = QStringLiteral("unavailable");
inline const QString Busy = QStringLiteral("busy");
inline const QString StaleRevision = QStringLiteral("stale_revision");
inline const QString ResultTooLarge = QStringLiteral("result_too_large");
} // namespace ErrorCode

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APIOBJECT_H
