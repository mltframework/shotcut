/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "multitrackmodel.h"
#include "mltcontroller.h"
#include "mainwindow.h"
#include <QScopedPointer>
#include <QFileInfo>
#include <QtDebug>

static const quintptr NO_PARENT_ID = quintptr(-1);

MultitrackModel::MultitrackModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_tractor(0)
{
}

MultitrackModel::~MultitrackModel()
{
    delete m_tractor;
    m_tractor = 0;
}

int MultitrackModel::rowCount(const QModelIndex &parent) const
{
    if (!m_tractor)
        return 0;
    if (parent.isValid()) {
        if (parent.internalId() != NO_PARENT_ID)
            return 0;
        int i = m_trackList.at(parent.row()).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            int n = playlist.count();
//            qDebug() << __FUNCTION__ << parent << i << n;
            return n;
        } else {
            return 0;
        }
    }
    return m_trackList.count();
}

int MultitrackModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant MultitrackModel::data(const QModelIndex &index, int role) const
{
    if (!m_tractor || !index.isValid())
        return QVariant();
    if (index.parent().isValid()) {
        int i = m_trackList.at(index.internalId()).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
//            qDebug() << __FUNCTION__ << index.row();
            QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(index.row()));
            if (info)
            switch (role) {
            case NameRole:
            case ResourceRole:
            case Qt::DisplayRole: {
                QString result = QString::fromUtf8(info->resource);
                if (result == "<producer>" && info->producer
                        && info->producer->is_valid() && info->producer->get("mlt_service"))
                    result = QString::fromUtf8(info->producer->get("mlt_service"));
                // Use basename for display
                if (role == NameRole && result.startsWith('/'))
                    result = QFileInfo(result).fileName();
                return result;
            }
            case ServiceRole:
                if (info->producer && info->producer->is_valid())
                    return QString::fromUtf8(info->producer->get("mlt_service"));
                break;
            case IsBlankRole:
                return playlist.is_blank(index.row());
            case StartRole:
                return info->start;
            case DurationRole:
                return info->frame_count;
            case InPointRole:
                return info->frame_in;
            case OutPointRole:
                return info->frame_out;
            case FramerateRole:
                return info->fps;
            case IsAudioRole:
                return m_trackList[index.internalId()].type == AudioTrackType;
            default:
                break;
            }
        }
    }
    else {
        int i = m_trackList.at(index.row()).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            switch (role) {
            case NameRole:
            case Qt::DisplayRole:
                return m_trackList.at(index.row()).name;
            case DurationRole:
                return playlist.get_playtime();
            case IsMuteRole:
                return playlist.get_int("hide") & 2;
            case IsHiddenRole:
                return playlist.get_int("hide") & 1;
            case IsAudioRole:
                return m_trackList[index.row()].type == AudioTrackType;
            default:
                break;
            }
        }
    }
    return QVariant();
}

QModelIndex MultitrackModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column > 0)
        return QModelIndex();
//    qDebug() << __FUNCTION__ << row << column << parent;
    QModelIndex result;
    if (parent.isValid()) {
        int i = m_trackList.at(parent.row()).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (track) {
            Mlt::Playlist playlist((mlt_playlist) track->get_producer());
            if (row < playlist.count())
                result = createIndex(row, column, parent.row());
        }
    } else if (row < m_trackList.count()) {
        result = createIndex(row, column, NO_PARENT_ID);
    }
    return result;
}

QModelIndex MultitrackModel::parent(const QModelIndex &index) const
{
//    qDebug() << __FUNCTION__ << index;
    if (!index.isValid() || index.internalId() == NO_PARENT_ID)
        return QModelIndex();
    else
        return createIndex(index.internalId(), 0, NO_PARENT_ID);
}

QHash<int, QByteArray> MultitrackModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[ResourceRole] = "resource";
    roles[ServiceRole] = "mlt_service";
    roles[IsBlankRole] = "blank";
    roles[StartRole] = "start";
    roles[DurationRole] = "duration";
    roles[InPointRole] = "in";
    roles[OutPointRole] = "out";
    roles[FramerateRole] = "fps";
    roles[IsMuteRole] = "mute";
    roles[IsHiddenRole] = "hidden";
    roles[IsAudioRole] = "audio";
    return roles;
}

void MultitrackModel::load()
{
    if (m_tractor) {
        beginRemoveRows(QModelIndex(), 0, rowCount(QModelIndex()) - 1);
        delete m_tractor;
        m_tractor = 0;
        m_trackList.clear();
        endRemoveRows();
   }
    // In some versions of MLT, the resource property is the XML filename,
    // but the Mlt::Tractor(Service&) constructor will fail unless it detects
    // the type as tractor, and mlt_service_identify() needs the resource
    // property to say "<tractor>" to identify it as playlist type.
    MLT.producer()->set("resource", "<tractor>");
    MLT.profile().set_explicit(true);
    m_tractor = new Mlt::Tractor(*MLT.producer());
    if (!m_tractor->is_valid()) {
        delete m_tractor;
        m_tractor = 0;
        return;
    }

    addBlackTrackIfNeeded();
    addMissingTransitions();
    refreshTrackList();
    beginInsertRows(QModelIndex(), 0, m_trackList.count() - 1);
    endInsertRows();
    emit loaded();
}

void MultitrackModel::refreshTrackList()
{
    int n = m_tractor->count();
    int a = 0;
    int v = 0;
    bool isKdenlive = false;

    // Add video tracks in reverse order.
    for (int i = 0; i < n; ++i) {
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (!track)
            continue;
        if (QString(track->get("id")) == "black_track")
            isKdenlive = true;
        else if (!track->get("shotcut:playlist") && !track->get("shotcut:audio")) {
            int hide = track->get_int("hide");
             // hide: 0 = a/v, 2 = muted video track
            if (track->get("shotcut:video") || hide == 0 || hide == 2) {
                Track t;
                t.mlt_index = i;
                t.type = VideoTrackType;
                t.number = v++;
                t.name = track->get("shotcut:name");
                if (t.name.isEmpty())
                    t.name = QString("V%1").arg(v);
                m_trackList.prepend(t);
            }
        }
    }

    // Add audio tracks.
    for (int i = 0; i < n; ++i) {
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (!track)
            continue;
        if (QString(track->get("id")) == "black_track")
            isKdenlive = true;
        else if (isKdenlive && QString(track->get("id")) == "playlist1")
            // In Kdenlive, playlist1 is a special audio mixdown track.
            continue;
        else if (!track->get("shotcut:playlist") && !track->get("shotcut:video")) {
            int hide = track->get_int("hide");
            // hide: 1 = audio only track, 3 = muted audio-only track
            if (track->get("shotcut:audio") || hide == 1 || hide == 3) {
                Track t;
                t.mlt_index = i;
                t.type = AudioTrackType;
                t.number = a++;
                t.name = track->get("shotcut:name");
                if (t.name.isEmpty())
                    t.name = QString("A%1").arg(a);
                m_trackList.append(t);
//                qDebug() << __FUNCTION__ << QString(track->get("id")) << i;
            }
        }
    }
}

void MultitrackModel::addBlackTrackIfNeeded()
{
    return;
    // Make sure the first track is black silence and add it if needed
    bool found = false;
    int n = m_tractor->count();
    QScopedPointer<Mlt::Producer> track(m_tractor->track(0));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::Producer> clip(playlist.get_clip(0));
        if (clip && QString(clip->get("resource")) == "black")
            found = true;
    }
    if (!found) {
        // Move all existing tracks down by 1.
        for (int i = n; i > 0; i++) {
            Mlt::Producer* producer = m_tractor->track(n - 1);
            if (producer)
                m_tractor->set_track(*producer, n);
            delete producer;
        }
        Mlt::Producer producer(MLT.profile(), "color:black");
        m_tractor->set_track(producer, 0);
    }
}

void MultitrackModel::addMissingTransitions()
{
    // Make sure there is a mix for every track
}
