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

#ifndef SHOTCUT_API_APIMARKERS_H
#define SHOTCUT_API_APIMARKERS_H

#include "api/apiobject.h"

#include <QVariantList>
#include <QVariantMap>

class MarkersModel;
namespace Markers {
class Marker;
}

namespace Shotcut {
namespace Api {

class ApiMarkers : public ApiObject
{
    Q_OBJECT
    Q_CLASSINFO("doc", "Timeline markers. A marker is a point or a range on the timeline.")
    Q_CLASSINFO("doc.count", "Number of markers.")
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    explicit ApiMarkers(QObject *parent = nullptr);

    int count() const;

    Q_CLASSINFO("mcp.list", "read")
    Q_CLASSINFO("doc.list",
                "Lists markers with index, text, start and end frames, color, and isRange.")
    Q_INVOKABLE QVariantList list() const;

    Q_CLASSINFO("mcp.marker", "read")
    Q_CLASSINFO("doc.marker", "Returns one marker.")
    Q_CLASSINFO("arg.marker.index", "Marker index, starting at 0.")
    Q_INVOKABLE QVariant marker(int index) const;

    Q_CLASSINFO("mcp.add", "edit")
    Q_CLASSINFO("doc.add",
                "Adds a point or range marker to the timeline and returns it. Requires a "
                "timeline with at least one track.")
    Q_CLASSINFO("arg.add.start", "integer!|First frame of the marker.")
    Q_CLASSINFO("arg.add.end", "integer|Last frame of a range marker; omit for a point marker.")
    Q_CLASSINFO("arg.add.text", "string|Marker name; defaults to \"Marker N\".")
    Q_CLASSINFO("arg.add.color",
                "string|Color as #rrggbb or an SVG color name; defaults to the marker color "
                "setting.")
    Q_INVOKABLE QVariant add(const QVariantMap &options);

    Q_CLASSINFO("mcp.update", "edit")
    Q_CLASSINFO("doc.update",
                "Changes a marker and returns it. Moving the start of a point marker moves the "
                "whole marker.")
    Q_CLASSINFO("arg.update.index", "Marker index.")
    Q_CLASSINFO("arg.update.start", "integer|First frame.")
    Q_CLASSINFO("arg.update.end", "integer|Last frame.")
    Q_CLASSINFO("arg.update.text", "string|Marker name.")
    Q_CLASSINFO("arg.update.color", "string|Color as #rrggbb or an SVG color name.")
    Q_INVOKABLE QVariant update(int index, const QVariantMap &options);

    Q_CLASSINFO("mcp.remove", "edit")
    Q_CLASSINFO("doc.remove",
                "Removes a marker and returns what it was. Later markers move down one index.")
    Q_CLASSINFO("arg.remove.index", "Marker index.")
    Q_INVOKABLE QVariant remove(int index);

signals:
    void changed();

private:
    MarkersModel *model() const;
    bool checkTimeline() const;
    bool checkIndex(int index) const;
    bool checkMarker(const Markers::Marker &marker) const;
    bool applyOptions(const QVariantMap &options, Markers::Marker &marker) const;
};

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APIMARKERS_H
