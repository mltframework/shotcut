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

#include "api/apimarkers.h"

#include "api/transaction.h"
#include "docks/timelinedock.h"
#include "mainwindow.h"
#include "models/markersmodel.h"
#include "models/multitrackmodel.h"
#include "settings.h"

#include <QColor>
#include <QVariantMap>

namespace Shotcut {
namespace Api {

namespace {

QVariantMap markerSnapshot(int index, const Markers::Marker &marker)
{
    QVariantMap result;
    result["index"] = index;
    result["text"] = marker.text;
    result["start"] = marker.start;
    result["end"] = marker.end;
    result["color"] = marker.color.name(QColor::HexRgb);
    result["isRange"] = marker.start != marker.end;
    return result;
}

} // namespace

ApiMarkers::ApiMarkers(QObject *parent)
    : ApiObject(parent)
{
    auto m = model();
    connect(m, &MarkersModel::modified, this, &ApiMarkers::changed);
    connect(m, &MarkersModel::rangesChanged, this, &ApiMarkers::changed);
    connect(m, &QAbstractItemModel::rowsInserted, this, &ApiMarkers::changed);
    connect(m, &QAbstractItemModel::rowsRemoved, this, &ApiMarkers::changed);
    connect(m, &QAbstractItemModel::modelReset, this, &ApiMarkers::changed);
    connect(m, &QAbstractItemModel::dataChanged, this, &ApiMarkers::changed);
}

MarkersModel *ApiMarkers::model() const
{
    return MAIN.timelineDock()->markersModel();
}

int ApiMarkers::count() const
{
    return model()->getMarkers().size();
}

QVariantList ApiMarkers::list() const
{
    QVariantList result;
    const auto markers = model()->getMarkers();
    for (int i = 0; i < markers.size(); ++i)
        result.append(markerSnapshot(i, markers.at(i)));
    return result;
}

QVariant ApiMarkers::marker(int index) const
{
    if (!checkIndex(index))
        return QVariant();
    return markerSnapshot(index, model()->getMarker(index));
}

QVariant ApiMarkers::add(const QVariantMap &options)
{
    if (!checkOptions("add", options) || !checkTimeline())
        return QVariant();
    Markers::Marker marker;
    marker.text = QStringLiteral("Marker %1").arg(model()->uniqueKey() + 1);
    marker.color = Settings.markerColor();
    marker.start = options.value("start").toInt();
    marker.end = marker.start;
    if (!applyOptions(options, marker) || !checkMarker(marker))
        return QVariant();
    Transaction::singleton().touch();
    const int index = count();
    model()->append(marker);
    return markerSnapshot(index, model()->getMarker(index));
}

QVariant ApiMarkers::update(int index, const QVariantMap &options)
{
    if (!checkOptions("update", options) || !checkIndex(index))
        return QVariant();
    if (options.isEmpty())
        return fail(ErrorCode::InvalidArgument, tr("Nothing to update."));
    Markers::Marker marker = model()->getMarker(index);
    if (marker.start == marker.end && options.contains("start") && !options.contains("end"))
        marker.end = options.value("start").toInt();
    if (!applyOptions(options, marker) || !checkMarker(marker))
        return QVariant();
    Transaction::singleton().touch();
    model()->update(index, marker);
    return markerSnapshot(index, model()->getMarker(index));
}

QVariant ApiMarkers::remove(int index)
{
    if (!checkIndex(index))
        return QVariant();
    const auto removed = markerSnapshot(index, model()->getMarker(index));
    Transaction::singleton().touch();
    model()->remove(index);
    return removed;
}

bool ApiMarkers::checkTimeline() const
{
    auto timeline = MAIN.timelineDock()->model();
    if (!timeline->tractor() || timeline->trackList().isEmpty()) {
        fail(ErrorCode::Unavailable, tr("Markers require a timeline with at least one track."));
        return false;
    }
    return true;
}

bool ApiMarkers::checkIndex(int index) const
{
    if (index < 0 || index >= count()) {
        fail(ErrorCode::NotFound, tr("Marker %1 does not exist.").arg(index));
        return false;
    }
    return true;
}

bool ApiMarkers::checkMarker(const Markers::Marker &marker) const
{
    if (!checkTimeline())
        return false;
    const int last = MAIN.timelineDock()->model()->tractor()->get_length() - 1;
    if (marker.start < 0 || marker.start > last) {
        fail(ErrorCode::InvalidArgument,
             tr("Marker start %1 is outside the timeline (0 to %2).").arg(marker.start).arg(last));
        return false;
    }
    if (marker.end < marker.start || marker.end > last) {
        fail(ErrorCode::InvalidArgument,
             tr("Marker end %1 must be between start %2 and %3.")
                 .arg(marker.end)
                 .arg(marker.start)
                 .arg(last));
        return false;
    }
    return true;
}

bool ApiMarkers::applyOptions(const QVariantMap &options, Markers::Marker &marker) const
{
    if (options.contains("start"))
        marker.start = options.value("start").toInt();
    if (options.contains("end"))
        marker.end = options.value("end").toInt();
    if (options.contains("text"))
        marker.text = options.value("text").toString();
    if (options.contains("color")) {
        const QColor color(options.value("color").toString());
        if (!color.isValid()) {
            fail(ErrorCode::InvalidArgument,
                 tr("\"%1\" is not a valid color.").arg(options.value("color").toString()));
            return false;
        }
        marker.color = color;
    }
    return true;
}

} // namespace Api
} // namespace Shotcut
