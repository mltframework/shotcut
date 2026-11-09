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

#ifndef SHOTCUT_API_APIINVOKER_H
#define SHOTCUT_API_APIINVOKER_H

#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

class QObject;

namespace Shotcut {
namespace Api {

struct InvokeResult
{
    bool ok = false;
    QVariant value;
    QString errorCode;
    QString errorMessage;
};

// Resolves a dotted path from root (e.g. "timeline.clips") and invokes the
// Q_INVOKABLE method with that name, or reads the property.
// Arguments are positional (list) or named (map). Named arguments map to
// parameter names; a method whose only parameter is "options" receives the map.
// Omitted trailing parameters use their C++ defaults.
// QObject-valued results are returned as describe() maps.
InvokeResult invokePath(QObject *root, const QString &path, const QVariant &args);
InvokeResult invokePath(QObject *root, const QString &path, const QVariantList &args);
InvokeResult invokePath(QObject *root, const QString &path, const QVariantMap &namedArgs);

// Lists the public properties (with current values) and Q_INVOKABLE methods of an API object.
QVariantMap describe(QObject *object);

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APIINVOKER_H
