/*
 * Copyright (c) 2023 Meltytech, LLC
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

#include "motiontrackermodel.h"
#include "shotcut_mlt_properties.h"
#include "qmltypes/qmlfilter.h"
#include "mltcontroller.h"

#include <QUuid>
#include <Mlt.h>
#include <Logger.h>

// This is hard-coded for now (minimum viable product).
static const int KEYFRAME_INTERVAL_FRAMES = 5;

class FindTrackersParser : public Mlt::Parser
{
private:
    MotionTrackerModel &m_model;

public:
    FindTrackersParser(MotionTrackerModel &model)
        : Mlt::Parser()
        , m_model{model}
    {}

    int on_start_filter(Mlt::Filter *filter)
    {
        if (QString::fromUtf8(filter->get("mlt_service")) == "opencv.tracker") {
            auto results = QString::fromLatin1(filter->get("results"));
            if (!results.isEmpty()) {
                auto name = QString::fromUtf8(filter->get(kTrackNameProperty));
                if (name.isEmpty()) {
                    name = m_model.nextName();
                    filter->set(kTrackNameProperty, name.toUtf8().constData());
                }
                auto key = m_model.add(name, results);
                if (!key.isEmpty()) {
                    filter->set(kUuidProperty, key.toUtf8().constData());
                }
            }
        }
        return 0;
    }
    int on_start_producer(Mlt::Producer *)
    {
        return 0;
    }
    int on_end_producer(Mlt::Producer *)
    {
        return 0;
    }
    int on_start_playlist(Mlt::Playlist *)
    {
        return 0;
    }
    int on_end_playlist(Mlt::Playlist *)
    {
        return 0;
    }
    int on_start_tractor(Mlt::Tractor *)
    {
        return 0;
    }
    int on_end_tractor(Mlt::Tractor *)
    {
        return 0;
    }
    int on_start_multitrack(Mlt::Multitrack *)
    {
        return 0;
    }
    int on_end_multitrack(Mlt::Multitrack *)
    {
        return 0;
    }
    int on_start_track()
    {
        return 0;
    }
    int on_end_track()
    {
        return 0;
    }
    int on_end_filter(Mlt::Filter *)
    {
        return 0;
    }
    int on_start_transition(Mlt::Transition *)
    {
        return 0;
    }
    int on_end_transition(Mlt::Transition *)
    {
        return 0;
    }
    int on_start_chain(Mlt::Chain *)
    {
        return 0;
    }
    int on_end_chain(Mlt::Chain *)
    {
        return 0;
    }
    int on_start_link(Mlt::Link *)
    {
        return 0;
    }
    int on_end_link(Mlt::Link *)
    {
        return 0;
    }
};

MotionTrackerModel::MotionTrackerModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_data[""] = {""}; // To show nothing in a combo
}

void MotionTrackerModel::load(Mlt::Producer *producer, bool reset)
{
    if (!producer) producer = MLT.producer();
    if (reset) {
        beginResetModel();
        m_data.clear();
        beginInsertRows(QModelIndex(), 0, 0);
        m_data[""] = {""};
        endInsertRows();
    }
    if (producer && producer->is_valid()) {
        FindTrackersParser(*this).start(*producer);
        Mlt::Properties retainList((mlt_properties) producer->get_data("xml_retain"));
        if (retainList.is_valid()) {
            Mlt::Playlist playlist((mlt_playlist) retainList.get_data(kPlaylistTrackId));
            if (playlist.is_valid() && playlist.type() == mlt_service_playlist_type) {
                FindTrackersParser(*this).start(playlist);
            }
        }
    }
    if (reset) endResetModel();
}

QString MotionTrackerModel::add(const QString &name, const QString &data)
{
    auto key = QUuid::createUuid().toString();
    if (!m_data.contains(key)) {
        auto row = rowCount();
        beginInsertRows(QModelIndex(), row, row);
        m_data[key] = {name, data, KEYFRAME_INTERVAL_FRAMES};
        LOG_DEBUG() << key << m_data[key].name;
        endInsertRows();
        return key;
    }
    return QString();
}

void MotionTrackerModel::updateData(const QString &key, const QString &data)
{
    auto keys = m_data.keys();
    auto row = keys.indexOf(key);
    if (row >= 0) {
        m_data[key].trackingData = data;
        auto i = createIndex(row, 0);
        emit dataChanged(i, i, {TrackingDataRole});
    }
}

void MotionTrackerModel::remove(const QString &key)
{
    if (m_data.contains(key)) {
        auto i = m_data.constBegin();
        for (int row = 0; i != m_data.constEnd(); ++i, ++row) {
            if (i.key() == key) {
                beginRemoveRows(QModelIndex(), row, row);
                m_data.remove(key);
                endInsertRows();
                break;
            }
        }
    }
}

void MotionTrackerModel::removeFromService(Mlt::Service *service)
{
    // look for an attached filter with mlt_service = opencv.tracker
    if (service && service->is_valid()) {
        auto producer = Mlt::Producer(*service).parent();
        for (int i = 0; i < producer.filter_count(); i++) {
            auto filter = producer.filter(i);
            if (filter && filter->is_valid()) {
                if (QString::fromUtf8(filter->get("mlt_service")) == "opencv.tracker") {
                    auto key = keyForFilter(filter);
                    if (!key.isEmpty())
                        remove(key);
                }
            }
        }
    }
}

void MotionTrackerModel::setName(QmlFilter *filter, const QString &name)
{
    if (filter && filter->service().is_valid()) {
        auto key = keyForFilter(&filter->service());
        if (!key.isEmpty() && m_data.contains(key)) {
            m_data[key].name = name;
        }
    }
}

QString MotionTrackerModel::nextName() const
{
    return tr("Tracker %1").arg(rowCount());
}

QString MotionTrackerModel::keyForRow(int row) const
{
    QString key;
    auto keys = m_data.keys();
    if (row >= 0 && row < keys.size()) {
        key = keys.at(row);
    }
    return key;
}

QString MotionTrackerModel::keyForFilter(Mlt::Service *service)
{
    QString key;
    if (service && service->is_valid())
        key = service->get(kUuidProperty);
    return key;
}

void MotionTrackerModel::reset(QmlFilter *filter, const QString &property, int row)
{
    auto key = keyForRow(row);
    if (!key.isEmpty() && filter && filter->service().is_valid() && !property.isEmpty()) {
        auto data = trackingData(key);

        if (!data.isEmpty()) {
            // Use a shotcut property to backup current values
            if (filter->get(kBackupProperty).isEmpty()) {
                filter->set(kBackupProperty, filter->get(property));
            } else {
                filter->set(property, filter->get(kBackupProperty));
            }
        }
    }
}

QList<MotionTrackerModel::TrackingItem> MotionTrackerModel::trackingData(const QString &key) const
{
    QList<TrackingItem> result;
    auto s = m_data.value(key, {}).trackingData;
    auto l = s.split(';');
    bool ok {false};
    Mlt::Properties props;

    for (const auto &i : l) {
        auto pair = i.split("~=");
        if (pair.size() == 2) {
            auto frame = pair.at(0).toInt(&ok);
            props.set("", pair.at(1).toLatin1().constData());
            auto rect = props.get_rect("");
            if (ok) {
                result << TrackingItem{frame, QRectF(rect.x, rect.y, rect.w, rect.h)};
            }
        }
    }
    return result;
}

QList<QRectF> MotionTrackerModel::trackingData(int row) const
{
    auto key = keyForRow(row);
    QList<QRectF> result;
    if (!key.isEmpty() && m_data.contains(key)) {
        for (const auto &a : trackingData(key)) {
            result << a.rect;
        }
    }
    return result;
}

int MotionTrackerModel::keyframeIntervalFrames(int row) const
{
    auto key = keyForRow(row);
    if (!key.isEmpty() && m_data.contains(key))
        return m_data.value(key).intervalFrames;
    return KEYFRAME_INTERVAL_FRAMES;
}

int MotionTrackerModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return m_data.size();
}

QVariant MotionTrackerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto key = keyForRow(index.row());
    if (!key.isEmpty()) {
        switch (role) {
        case Qt::DisplayRole:
            return m_data.value(key).name;
        case TrackingDataRole:
            return m_data.value(key).trackingData;
        default:
            break;
        }
    }
    return QVariant();
}

bool MotionTrackerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        auto key = index.data(Qt::UserRole).toString();
        if (m_data.contains(key)) {
            switch (role) {
            case Qt::DisplayRole:
                m_data[key].name = value.toString();
                emit dataChanged(index, index, {role});
                break;
            case TrackingDataRole:
                m_data[key].trackingData = value.toString();
                emit dataChanged(index, index, {role});
                break;
            default:
                break;
            }
            return true;
        }
    }
    return false;
}

Qt::ItemFlags MotionTrackerModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

//bool MotionTrackerModel::insertRows(int row, int count, const QModelIndex &parent)
//{
//    beginInsertRows(parent, row, row + count - 1);
//    endInsertRows();
//    return true;
//}

//bool MotionTrackerModel::removeRows(int row, int count, const QModelIndex &parent)
//{
//    beginRemoveRows(parent, row, row + count - 1);
//    // FIXME: Implement me!
//    endRemoveRows();
//    return true;
//}
