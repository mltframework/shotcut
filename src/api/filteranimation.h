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

#ifndef SHOTCUT_API_FILTERANIMATION_H
#define SHOTCUT_API_FILTERANIMATION_H

#include <MltProperties.h>
#include <QVariantList>

namespace Shotcut {
namespace Api {
// Internal structured-keyframe conversion for ApiFilters, not a reflected QML/MCP API.
// Kept separate so validation and MLT serialization can be tested without the application UI.
namespace FilterAnimation {
QVariantList read(Mlt::Properties &properties, const QString &name, bool rectangle, int duration);
bool write(Mlt::Properties &properties,
           const QString &name,
           bool rectangle,
           int duration,
           const QVariantList &keys,
           QString &error);
} // namespace FilterAnimation
} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_FILTERANIMATION_H
