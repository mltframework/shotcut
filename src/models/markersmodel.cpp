/*
 * Copyright (c) 2021 Meltytech, LLC
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

#include "markersmodel.h"

#include "commands/markercommands.h"
#include "mainwindow.h"
#include "shotcut_mlt_properties.h"

#include <Logger.h>

enum Columns {
    COLUMN_COLOR = 0,
    COLUMN_TEXT,
    COLUMN_START,
    COLUMN_END,
    COLUMN_DURATION,
    COLUMN_COUNT
};

static void markerToProperties(const Markers::Marker& marker, Mlt::Properties* properties, Mlt::Producer* producer)
{
    properties->set("text", qUtf8Printable(marker.text));
    properties->set("start", producer->frames_to_time(marker.start, mlt_time_clock));
    properties->set("end", producer->frames_to_time(marker.end, mlt_time_clock));
    auto s = QString::asprintf("#%02X%02X%02X", marker.color.red(), marker.color.green(), marker.color.blue());
    properties->set("color", s.toLatin1().constData());
}

static void propertiesToMarker(Mlt::Properties* properties, Markers::Marker& marker, Mlt::Producer* producer)
{
    marker.text = QString::fromUtf8(properties->get("text"));
    marker.start = producer->time_to_frames(properties->get("start"));
    marker.end = producer->time_to_frames(properties->get("end"));
    mlt_color color = properties->get_color("color");
    marker.color = QColor::fromRgb(color.r, color.g, color.b, 0xFF);
}

MarkersModel::MarkersModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_producer(nullptr)
{
}

MarkersModel::~MarkersModel()
{
}

void MarkersModel::load(Mlt::Producer* producer)
{
    beginResetModel();
    m_producer = producer;
    m_keys.clear();
    if (m_producer) {
        Mlt::Properties* markerList = m_producer->get_props(kShotcutMarkersProperty);
        if (markerList &&  markerList->is_valid()) {
            int count = markerList->count();
            for (int i = 0; i < count; i++) {
                Mlt::Properties* markerProperties = markerList->get_props_at(i);
                if (markerProperties && markerProperties->is_valid()) {
                    char* key = markerList->get_name(i);
                    int intKey = QString(key).toInt();
                    m_keys << intKey;
                    Markers::Marker marker;
                    propertiesToMarker(markerProperties, marker, producer);
                    updateRecentColors(marker.color);
                }
                delete markerProperties;
            }
        }
        delete markerList;
    }

    endResetModel();
}

Markers::Marker MarkersModel::getMarker(int markerIndex)
{
    Markers::Marker retMarker;
    Mlt::Properties* markerProperties = getMarkerProperties(markerIndex);
    if (!markerProperties || !markerProperties->is_valid()) {
        LOG_ERROR() << "Marker does not exist" << markerIndex;
        delete markerProperties;
    } else {
        propertiesToMarker(markerProperties, retMarker, m_producer);
    }
    return retMarker;
}

void MarkersModel::remove(int markerIndex)
{
    Mlt::Properties* markerProperties = getMarkerProperties(markerIndex);
    if (!markerProperties || !markerProperties->is_valid()) {
        LOG_ERROR() << "Marker does not exist" << markerIndex;
        delete markerProperties;
        return;
    }
    Markers::Marker oldMarker;
    propertiesToMarker(markerProperties, oldMarker, m_producer);
    Markers::DeleteCommand* command = new Markers::DeleteCommand(*this, oldMarker, markerIndex);
    MAIN.undoStack()->push(command);
    delete markerProperties;
}

void MarkersModel::doRemove(int markerIndex)
{
    if (!m_producer) {
        LOG_ERROR() << "No producer";
        return;
    }
    QModelIndex modelIndex = index(markerIndex, 0);
    if (!modelIndex.isValid()) {
        LOG_ERROR() << "Invalid Index: " << markerIndex;
        return;
    }
    if (modelIndex.row() >= m_keys.count()) {
        LOG_ERROR() << "Index out of bounds: " << modelIndex.row() << m_keys.count();
        return;
    }
    Mlt::Properties* markersListProperties = m_producer->get_props(kShotcutMarkersProperty);
    if (!markersListProperties || !markersListProperties->is_valid()) {
        LOG_ERROR() << "No Markers";
        delete markersListProperties;
        return;
    }
    auto marker = getMarker(markerIndex);
    beginRemoveRows(QModelIndex(), modelIndex.row(), modelIndex.row());
    markersListProperties->clear(qUtf8Printable(QString::number(m_keys[modelIndex.row()])));
    m_keys.removeAt(modelIndex.row());
    endRemoveRows();
    if (marker.end > marker.start) emit rangesChanged();

    delete markersListProperties;
    emit modified();
}

void MarkersModel::doInsert(int markerIndex,  const Markers::Marker& marker )
{
    if (!m_producer) {
        LOG_ERROR() << "No producer";
        return;
    }

    QModelIndex modelIndex;
    if (markerIndex == markerCount())
        // Allow inserting after the last marker
        modelIndex = createIndex(markerIndex, 0, (int)0);
    else
        modelIndex = index(markerIndex, 0);
    if (!modelIndex.isValid()) {
        LOG_ERROR() << "Invalid Index: " << markerIndex;
        return;
    }

    Mlt::Properties* markersListProperties = m_producer->get_props(kShotcutMarkersProperty);
    if (!markersListProperties || !markersListProperties->is_valid()) {
        delete markersListProperties;
        markersListProperties = new Mlt::Properties;
        m_producer->set(kShotcutMarkersProperty, *markersListProperties);
    }

    Mlt::Properties markerProperties;
    markerToProperties(marker, &markerProperties, m_producer);

    beginInsertRows(QModelIndex(), modelIndex.row(), modelIndex.row());
    int key = uniqueKey();
    markersListProperties->set(qUtf8Printable(QString::number(key)), markerProperties);
    m_keys.insert(modelIndex.row(), key);
    endInsertRows();
    updateRecentColors(marker.color);
    if (marker.end > marker.start) emit rangesChanged();

    delete markersListProperties;
    emit modified();
}

void MarkersModel::append( const Markers::Marker& marker )
{
    if (!m_producer) {
        LOG_ERROR() << "No producer";
        return;
    }
    Markers::AppendCommand* command = new Markers::AppendCommand(*this, marker, markerCount());
    MAIN.undoStack()->push(command);
}

void MarkersModel::doAppend( const Markers::Marker& marker )
{
    if (!m_producer) {
        LOG_ERROR() << "No producer";
        return;
    }

    Mlt::Properties* markersListProperties = m_producer->get_props(kShotcutMarkersProperty);
    if (!markersListProperties || !markersListProperties->is_valid()) {
        delete markersListProperties;
        markersListProperties = new Mlt::Properties;
        m_producer->set(kShotcutMarkersProperty, *markersListProperties);
    }

    Mlt::Properties markerProperties;
    markerToProperties(marker, &markerProperties, m_producer);

    int count = markerCount();
    beginInsertRows(QModelIndex(), count, count);
    int key = uniqueKey();
    markersListProperties->set(qUtf8Printable(QString::number(key)), markerProperties);
    m_keys.append(key);
    updateRecentColors(marker.color);
    endInsertRows();
    if (marker.end > marker.start) emit rangesChanged();

    delete markersListProperties;
    emit modified();
}

void MarkersModel::update(int markerIndex,  const Markers::Marker& marker)
{
    Mlt::Properties* markerProperties = getMarkerProperties(markerIndex);
    if (!markerProperties || !markerProperties->is_valid()) {
        LOG_ERROR() << "Marker does not exist" << markerIndex;
        delete markerProperties;
        return;
    }
    Markers::Marker oldMarker;
    propertiesToMarker(markerProperties, oldMarker, m_producer);

    Markers::UpdateCommand* command = new Markers::UpdateCommand(*this, marker, oldMarker, markerIndex);
    MAIN.undoStack()->push(command);
    delete markerProperties;
}

void MarkersModel::doUpdate(int markerIndex,  const Markers::Marker& marker)
{
    QModelIndex startIndex = index(markerIndex, 0);
    QModelIndex endIndex = index(markerIndex, COLUMN_COUNT - 1);
    if (!startIndex.isValid() || !endIndex.isValid()) {
        LOG_ERROR() << "Invalid Index: " << startIndex << endIndex;
        return;
    }
    Mlt::Properties* markerProperties = getMarkerProperties(markerIndex);
    if (!markerProperties || !markerProperties->is_valid()) {
        LOG_ERROR() << "Marker does not exist" << markerIndex;
        delete markerProperties;
        return;
    }
    Markers::Marker markerBefore;
    propertiesToMarker(markerProperties, markerBefore, m_producer);

    markerToProperties(marker, markerProperties, m_producer);
    delete markerProperties;
    updateRecentColors(marker.color);

    emit dataChanged(startIndex, endIndex, QVector<int>() << Qt::DisplayRole << TextRole << StartRole << EndRole << ColorRole);
    if ((markerBefore.end == markerBefore.start && marker.end > marker.start) ||
        (markerBefore.end != markerBefore.start && marker.end == marker.start) ||
        (markerBefore.text != marker.text)) {
        emit rangesChanged();
    }
    emit modified();
}

void MarkersModel::move(int markerIndex, int start, int end)
{
    Mlt::Properties* markerProperties = getMarkerProperties(markerIndex);
    if (!markerProperties || !markerProperties->is_valid()) {
        LOG_ERROR() << "Marker does not exist" << markerIndex;
        delete markerProperties;
        return;
    }
    Markers::Marker oldMarker;
    propertiesToMarker(markerProperties, oldMarker, m_producer);
    Markers::Marker newMarker = oldMarker;
    newMarker.start = start;
    newMarker.end = end;

    Markers::UpdateCommand* command = new Markers::UpdateCommand(*this, newMarker, oldMarker, markerIndex);
    MAIN.undoStack()->push(command);
}

void MarkersModel::setColor(int markerIndex, const QColor& color)
{
    Mlt::Properties* markerProperties = getMarkerProperties(markerIndex);
    if (!markerProperties || !markerProperties->is_valid()) {
        LOG_ERROR() << "Marker does not exist" << markerIndex;
        delete markerProperties;
        return;
    }
    Markers::Marker oldMarker;
    propertiesToMarker(markerProperties, oldMarker, m_producer);
    Markers::Marker newMarker = oldMarker;
    newMarker.color = color;

    Markers::UpdateCommand* command = new Markers::UpdateCommand(*this, newMarker, oldMarker, markerIndex);
    MAIN.undoStack()->push(command);
}

int MarkersModel::markerCount() const
{
    if (!m_producer) {
        return 0;
    }
    return m_keys.count();
}

int MarkersModel::keyIndex(int key) const
{
    int index = -1;
    for (int i = 0; i < m_keys.size(); i++) {
        if (m_keys[i] == key) {
            index = i;
            break;
        }
    }
    return index;
}

int MarkersModel::uniqueKey() const
{
    int key = 0;
    while (keyIndex(key) >= 0) {
        key++;
    }
    return key;
}

int MarkersModel::markerIndexForPosition(int position)
{
    QScopedPointer<Mlt::Properties> markerList(m_producer->get_props(kShotcutMarkersProperty));
    if (markerList &&  markerList->is_valid()) {
        for (const auto i : qAsConst(m_keys)) {
            QScopedPointer<Mlt::Properties> marker(markerList->get_props(qUtf8Printable(QString::number(i))));
            if (marker && marker->is_valid() && position == m_producer->time_to_frames(marker->get("start"))) {
                return keyIndex(i);
            }
        }
    }
    return -1;
}

QModelIndex MarkersModel::modelIndexForRow(int row)
{
    return index(row, 0);
}

QMap<int, QString> MarkersModel::ranges()
{
    QMap<int, QString> result;
    QScopedPointer<Mlt::Properties> markerList(m_producer->get_props(kShotcutMarkersProperty));
    if (markerList &&  markerList->is_valid()) {
        for (const auto i : qAsConst(m_keys)) {
            QScopedPointer<Mlt::Properties> marker(markerList->get_props(qUtf8Printable(QString::number(i))));
            if (marker && marker->is_valid()) {
                Markers::Marker m;
                propertiesToMarker(marker.get(), m, m_producer);
                if (m.end > m.start) {
                    result[i] = m.text;
                }
            }
        }
    }
    return result;
}

QStringList MarkersModel::recentColors()
{
    return m_recentColors.values();
}

Mlt::Properties* MarkersModel::getMarkerProperties(int markerIndex)
{
    Mlt::Properties* markerProperties = nullptr;
    if (!m_producer) {
        LOG_ERROR() << "No producer";
        return markerProperties;
    }
    QModelIndex modelIndex = index(markerIndex, 0);
    if (!modelIndex.isValid()) {
        LOG_ERROR() << "Invalid Index: " << markerIndex;
        return markerProperties;
    }
    Mlt::Properties* markersListProperties = m_producer->get_props(kShotcutMarkersProperty);
    if (!markersListProperties || !markersListProperties->is_valid()) {
        LOG_ERROR() << "No Markers";
    }
    else
    {
        markerProperties = markersListProperties->get_props(qUtf8Printable(QString::number(m_keys[modelIndex.row()])));
        if (!markerProperties || !markerProperties->is_valid()) {
            LOG_ERROR() << "Marker does not exist" << modelIndex.row();
            delete markerProperties;
            markerProperties = nullptr;
        }
    }
    delete markersListProperties;
    return markerProperties;
}

void MarkersModel::updateRecentColors(const QColor& color)
{
    m_recentColors.insert(color.rgb(), color.name());
    emit recentColorsChanged();
}

int MarkersModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return markerCount();
}

int MarkersModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return COLUMN_COUNT;
}

QVariant MarkersModel::data(const QModelIndex& index, int role) const
{
    QVariant result;

    switch (role) {
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::DecorationRole:
        case Qt::FontRole:
        case Qt::TextAlignmentRole:
        case Qt::CheckStateRole:
        case Qt::SizeHintRole:
            return result;
    }

    if (!m_producer) {
        LOG_DEBUG() << "No Producer: " << index.row() << index.column() << role;
        return result;
    }
    if (!index.isValid() || index.column() < 0 || index.column() >= COLUMN_COUNT || index.row() < 0 || index.row() >= markerCount()) {
        LOG_ERROR() << "Invalid Index: " << index.row() << index.column() << role;
        return result;
    }
    Mlt::Properties* markersListProperties = m_producer->get_props(kShotcutMarkersProperty);
    if (!markersListProperties || !markersListProperties->is_valid()) {
        LOG_DEBUG() << "No Markers: " << index.row() << index.column() << role;
        delete markersListProperties;
        return result;
    }
    Mlt::Properties* markerProperties = markersListProperties->get_props(qUtf8Printable(QString::number(m_keys[index.row()])));
    if (!markerProperties || !markerProperties->is_valid()) {
        LOG_DEBUG() << "Marker does not exist: " << index.row() << index.column() << role << m_keys[index.row()];
        delete markerProperties;
        delete markersListProperties;
        return result;
    }

    Markers::Marker marker;
    propertiesToMarker(markerProperties, marker, m_producer);
    switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
                case COLUMN_COLOR:
                    result = marker.color;
                    break;
                case COLUMN_TEXT:
                    result = marker.text;
                    break;
                case COLUMN_START:
                    result = QString(m_producer->frames_to_time(marker.start, mlt_time_smpte_df));
                    break;
                case COLUMN_END:
                    result = QString(m_producer->frames_to_time(marker.end, mlt_time_smpte_df));
                    break;
                case COLUMN_DURATION:
                    result = QString(m_producer->frames_to_time(marker.end - marker.start, mlt_time_smpte_df));
                    break;
                default:
                    LOG_ERROR() << "Invalid Column" << index.column() << role;
                    break;
            }
            break;
        case Qt::BackgroundRole:
            if (index.column() == COLUMN_COLOR) {
                result = marker.color;
            }
            break;
        case Qt::ForegroundRole:
            if (index.column() == COLUMN_COLOR) {
                // Pick a contrasting color
                if (marker.color.value() < 127) {
                    result = QColor(Qt::white);
                } else {
                    result = QColor(Qt::black);
                }
            }
            break;
        case TextRole:
            result = marker.text;
            break;
        case StartRole:
            result = marker.start;
            break;
        case EndRole:
            result = marker.end;
            break;
        case ColorRole:
            result = marker.color;
            break;
        default:
            LOG_ERROR() << "Invalid Role" << index.row() << index.column() << roleNames()[role] << role;
            break;
    }
    delete markersListProperties;
    delete markerProperties;
    return result;
}

QVariant MarkersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case COLUMN_COLOR:
            return tr("Color");
        case COLUMN_TEXT:
            return tr("Marker");
        case COLUMN_START:
            return tr("Start");
        case COLUMN_END:
            return tr("End");
        case COLUMN_DURATION:
            return tr("Duration");
        default:
            break;
        }
    }
    return QVariant();
}

QModelIndex MarkersModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    if (column < 0 || column >= COLUMN_COUNT || row < 0 || row >= markerCount())
        return QModelIndex();
    return createIndex(row, column, (int)0);
}

QModelIndex MarkersModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

QHash<int, QByteArray> MarkersModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[TextRole]        = "text";
    roles[StartRole]       = "start";
    roles[EndRole]         = "end";
    roles[ColorRole]       = "color";
    return roles;
}
