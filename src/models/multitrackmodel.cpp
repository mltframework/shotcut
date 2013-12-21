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
#include "database.h"
#include <QScopedPointer>
#include <QFileInfo>
#include <QThreadPool>
#include <QPersistentModelIndex>
#include <QCryptographicHash>
#include <QRgb>

#include <QtDebug>

static const quintptr NO_PARENT_ID = quintptr(-1);
static const char* kAudioLevelsProperty = "shotcut:audio-levels";
static const char* kTrackNameProperty = "shotcut:name";
static const char* kShotcutPlaylistProperty = "shotcut:playlist";
static const char* kAudioTrackProperty = "shotcut:audio";
static const char* kVideoTrackProperty = "shotcut:video";

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
        // Get data for a clip.
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
            case AudioLevelsRole: {
                QVariantList* levels = (QVariantList*) info->producer->get_data(kAudioLevelsProperty);
                int channels = 2;
                if (info->producer && info->producer->is_valid() && info->producer->get_data(kAudioLevelsProperty))
                    return levels->mid(info->frame_in * channels, info->frame_count * channels);
                break;
            }
            default:
                break;
            }
        }
    }
    else {
        // Get data for a track.
        int i = m_trackList.at(index.row()).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            switch (role) {
            case NameRole:
            case Qt::DisplayRole:
                return track->get(kTrackNameProperty);
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
    roles[AudioLevelsRole] = "audioLevels";
    return roles;
}

void MultitrackModel::setTrackName(int row, const QString &value)
{
    if (row < m_trackList.size()) {
        int i = m_trackList.at(row).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (track) {
            track->set(kTrackNameProperty, value.toUtf8().constData());

            QModelIndex modelIndex = index(row, 0);
            QVector<int> roles;
            roles << NameRole;
            emit dataChanged(modelIndex, modelIndex, roles);
        }
    }
}

void MultitrackModel::setTrackMute(int row, bool mute)
{
    if (row < m_trackList.size()) {
        int i = m_trackList.at(row).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (track) {
            int hide = track->get_int("hide");
            if (mute)
                hide |= 2;
            else
                hide ^= 2;
            track->set("hide", hide);

            QModelIndex modelIndex = index(row, 0);
            QVector<int> roles;
            roles << IsMuteRole;
            emit dataChanged(modelIndex, modelIndex, roles);
        }
    }
}

void MultitrackModel::setTrackHidden(int row, bool hidden)
{
    if (row < m_trackList.size()) {
        int i = m_trackList.at(row).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (track) {
            int hide = track->get_int("hide");
            if (hidden)
                hide |= 1;
            else
                hide ^= 1;
            track->set("hide", hide);
            MLT.refreshConsumer();

            QModelIndex modelIndex = index(row, 0);
            QVector<int> roles;
            roles << IsHiddenRole;
            emit dataChanged(modelIndex, modelIndex, roles);
        }
    }
}

void MultitrackModel::trimClipIn(int trackIndex, int clipIndex, int delta)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));

        if (!info || info->frame_in < 0)
            return; // no work to do
        if (info->frame_in + delta < 0)
            delta = -info->frame_in; // clamp

        if ((clipIndex > 0 && playlist.is_blank(clipIndex - 1)) || (delta > 0)) {
            int in = info->frame_in + delta;
            int out = info->frame_out;
            playlist.resize_clip(clipIndex, in, out);
    
            QModelIndex index = createIndex(clipIndex, 0, trackIndex);
            QVector<int> roles;
            roles << DurationRole;
            roles << InPointRole;
            emit dataChanged(index, index, roles);
        }
        // Adjust left of the clip.
        if (clipIndex > 0 && playlist.is_blank(clipIndex - 1)) {
            int out = playlist.clip_length(clipIndex - 1) + delta - 1;
            if (out < 0) {
                qDebug() << "remove blank at left";
                beginRemoveRows(createIndex(trackIndex, 0, NO_PARENT_ID), clipIndex - 1, clipIndex - 1);
                playlist.remove(clipIndex - 1);
                endRemoveRows();
            } else {
                qDebug() << "adjust blank on left to" << out;
                playlist.resize_clip(clipIndex - 1, 0, out);
    
                QModelIndex index = createIndex(clipIndex - 1, 0, trackIndex);
                QVector<int> roles;
                roles << DurationRole;
                emit dataChanged(index, index, roles);
            }
        } else if (delta > 0) {
            qDebug() << "add blank on left duration" << delta - 1;
            int newIndex = clipIndex;
            beginInsertRows(createIndex(trackIndex, 0, NO_PARENT_ID), newIndex, newIndex);
            playlist.insert_blank(newIndex, delta - 1);
            endInsertRows();
        } else {
            // TODO start adding a transition
            return;
        }
    }
}

void MultitrackModel::notifyClipIn(int trackIndex, int clipIndex)
{
    if (trackIndex >= 0 && trackIndex < m_trackList.size() && clipIndex >= 0) {
        QModelIndex index = createIndex(clipIndex, 0, trackIndex);
        QVector<int> roles;
        roles << AudioLevelsRole;
        emit dataChanged(index, index, roles);
    }
}

void MultitrackModel::trimClipOut(int trackIndex, int clipIndex, int delta)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));

        if (!info || info->frame_out >= info->length)
            return; // no work to do
        if ((info->frame_out - delta) >= info->length)
            delta = info->frame_out - info->length + 1; // clamp

        // Adjust right of the clip.
        if (clipIndex >= 0 && (clipIndex + 1) < playlist.count() && playlist.is_blank(clipIndex + 1)) {
            int out = playlist.clip_length(clipIndex + 1) + delta - 1;
            if (out < 0) {
                qDebug() << "remove blank at right";
                beginRemoveRows(createIndex(trackIndex, 0, NO_PARENT_ID), clipIndex + 1, clipIndex + 1);
                playlist.remove(clipIndex + 1);
                endRemoveRows();
            } else {
                qDebug() << "adjust blank on right to" << out;
                playlist.resize_clip(clipIndex + 1, 0, out);
     
                QModelIndex index = createIndex(clipIndex + 1, 0, trackIndex);
                QVector<int> roles;
                roles << DurationRole;
                emit dataChanged(index, index, roles);
            }
        } else if (delta > 0 && (clipIndex + 1) < playlist.count())  {
            // Add blank to right.
            qDebug() << "add blank on right duration" << (delta - 1);
            int newIndex = clipIndex + 1;
            beginInsertRows(createIndex(trackIndex, 0, NO_PARENT_ID), newIndex, newIndex);
            playlist.insert_blank(newIndex, delta - 1);
            endInsertRows();
        } else if (clipIndex < playlist.count() - 1) {
            // TODO start adding a transition
            return;
        }
        int in = info->frame_in;
        int out = info->frame_out - delta;
        playlist.resize_clip(clipIndex, in, out);

        QModelIndex index = createIndex(clipIndex, 0, trackIndex);
        QVector<int> roles;
        roles << DurationRole;
        roles << OutPointRole;
        emit dataChanged(index, index, roles);
    }
}

void MultitrackModel::notifyClipOut(int trackIndex, int clipIndex)
{
    if (trackIndex >= 0 && trackIndex < m_trackList.size() && clipIndex >= 0) {
        QModelIndex index = createIndex(clipIndex, 0, trackIndex);
        QVector<int> roles;
        roles << AudioLevelsRole;
        emit dataChanged(index, index, roles);
    }
}

bool MultitrackModel::moveClip(int trackIndex, int clipIndex, int position)
{
    bool result = false;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        int targetIndex = playlist.get_clip_index_at(position);

        if ((clipIndex + 1) < playlist.count() && position >= playlist.get_playtime()) {
            // Clip relocated to end of playlist.
            moveClipToEnd(playlist, trackIndex, clipIndex, position);
            result = true;
        }
        else if ((targetIndex < (clipIndex - 1) || targetIndex > (clipIndex + 1))
            && playlist.is_blank_at(position) && playlist.clip_length(clipIndex) <= playlist.clip_length(targetIndex)) {
            // Relocate clip.
            relocateClip(playlist, trackIndex, clipIndex, position);
            result = true;
        }
        else if (targetIndex >= (clipIndex - 1) && targetIndex <= (clipIndex + 1)) {
            int length = playlist.clip_length(clipIndex);
            int targetIndexEnd = playlist.get_clip_index_at(position + length - 1);

            if ((playlist.is_blank_at(position) || targetIndex == clipIndex)
                && (playlist.is_blank_at(position + length - 1) || targetIndexEnd == clipIndex)) {

                if (position < 0) {
                    // Special case: dragged left of timeline origin.
                    Mlt::ClipInfo* info = playlist.clip_info(clipIndex);
                    playlist.resize_clip(clipIndex, info->frame_in - position, info->frame_out);
                    delete info;
                    QModelIndex index = createIndex(clipIndex, 0, trackIndex);
                    QVector<int> roles;
                    roles << DurationRole;
                    roles << InPointRole;
                    emit dataChanged(index, index, roles);
                    if (clipIndex > 0) {
                        QModelIndex parentIndex = createIndex(trackIndex, 0, NO_PARENT_ID);
                        beginMoveRows(parentIndex, clipIndex, clipIndex, parentIndex, 0);
                        playlist.move(clipIndex, 0);
                        endMoveRows();
                        consolidateBlanks(playlist, trackIndex);
                        clipIndex = 0;
                    }
                }
                // Reposition the clip within its current blank spot.
                moveClipInBlank(playlist, trackIndex, clipIndex, position);
                result = true;
            }
        }
    }
    return result;
}

void MultitrackModel::moveClipToEnd(Mlt::Playlist& playlist, int trackIndex, int clipIndex, int position)
{
    int n = playlist.count();
    int length = position - playlist.clip_start(n - 1) - playlist.clip_length(n - 1);

    if (clipIndex > 0 && playlist.is_blank(clipIndex - 1)) {
        // If there was a blank on the left adjust it.
        int duration = playlist.clip_length(clipIndex - 1) + playlist.clip_length(clipIndex);
        qDebug() << "adjust blank on left to" << duration;
        playlist.resize_clip(clipIndex - 1, 0, duration - 1);

        QModelIndex index = createIndex(clipIndex - 1, 0, trackIndex);
        QVector<int> roles;
        roles << DurationRole;
        emit dataChanged(index, index, roles);
    } else if ((clipIndex + 1) < n && playlist.is_blank(clipIndex + 1)) {
        // If there was a blank on the right adjust it.
        int duration = playlist.clip_length(clipIndex + 1) + playlist.clip_length(clipIndex);
        qDebug() << "adjust blank on right to" << duration;
        playlist.resize_clip(clipIndex + 1, 0, duration - 1);

        QModelIndex index = createIndex(clipIndex + 1, 0, trackIndex);
        QVector<int> roles;
        roles << DurationRole;
        emit dataChanged(index, index, roles);
    }
    // Add blank to end if needed.
    if (length > 0) {
        beginInsertRows(createIndex(trackIndex, 0, NO_PARENT_ID), n, n);
        playlist.blank(length - 1);
        endInsertRows();
    }
    // Finally, move clip into place.
    QModelIndex parentIndex = createIndex(trackIndex, 0, NO_PARENT_ID);
    beginMoveRows(parentIndex, clipIndex, clipIndex, parentIndex, playlist.count());
    playlist.move(clipIndex, playlist.count());
    endMoveRows();
    consolidateBlanks(playlist, trackIndex);
}

void MultitrackModel::relocateClip(Mlt::Playlist& playlist, int trackIndex, int clipIndex, int position)
{
    int targetIndex = playlist.get_clip_index_at(position);

    // Split target blank clip.
    playlist.split_at(position);
    if (clipIndex >= targetIndex)
        ++clipIndex;

    // Notify blank on left was adjusted.
    QModelIndex index = createIndex(targetIndex, 0, trackIndex);
    QVector<int> roles;
    roles << DurationRole;
    emit dataChanged(index, index, roles);

    // Adjust blank on right.
    int duration = playlist.clip_length(targetIndex + 1) - playlist.clip_length(clipIndex);
    if (duration > 0) {
        qDebug() << "adjust blank on right" << (targetIndex + 1) << " to" << duration;
        playlist.resize_clip(targetIndex + 1, 0, duration - 1);
        beginInsertRows(createIndex(trackIndex, 0, NO_PARENT_ID), targetIndex + 1, targetIndex + 1);
        endInsertRows();
    } else {
        qDebug() << "remove blank on right";
        int index = targetIndex + 1;
        playlist.remove(index);
    }

    // Move clip between split blanks.
    QModelIndex parentIndex = createIndex(trackIndex, 0, NO_PARENT_ID);
    beginMoveRows(parentIndex, clipIndex, clipIndex, parentIndex, targetIndex + 1);
    playlist.move(clipIndex, targetIndex + 1);
    endMoveRows();
    consolidateBlanks(playlist, trackIndex);
}

void MultitrackModel::moveClipInBlank(Mlt::Playlist& playlist, int trackIndex, int clipIndex, int position)
{
    int delta = position - playlist.clip_start(clipIndex);

    if (clipIndex > 0 && playlist.is_blank(clipIndex - 1)) {
        // Adjust blank on left.
        int duration = playlist.clip_length(clipIndex - 1) + delta;
        if (duration > 0) {
            qDebug() << "adjust blank on left" << (clipIndex - 1) << "to" << duration;
            playlist.resize_clip(clipIndex - 1, 0, duration - 1);

            QModelIndex index = createIndex(clipIndex - 1, 0, trackIndex);
            QVector<int> roles;
            roles << DurationRole;
            emit dataChanged(index, index, roles);
        } else {
            qDebug() << "remove blank on left";
            int index = clipIndex - 1;
            beginRemoveRows(createIndex(trackIndex, 0, NO_PARENT_ID), index, index);
            playlist.remove(index);
            endRemoveRows();
            consolidateBlanks(playlist, trackIndex);
        }
    } else if (delta > 0) {
        qDebug() << "add blank on left with duration" << delta;
        // Add blank to left.
        int index = qMax(clipIndex, 0);
        beginInsertRows(createIndex(trackIndex, 0, NO_PARENT_ID), index, index);
        playlist.insert_blank(index, delta - 1);
        endInsertRows();
        ++clipIndex;
    }

    if ((clipIndex + 1) < playlist.count() && playlist.is_blank(clipIndex + 1)) {
        // Adjust blank to right.
        int duration = playlist.clip_length(clipIndex + 1) - delta;
        if (duration > 0) {
            qDebug() << "adjust blank on right" << (clipIndex + 1) << "to" << duration;
            playlist.resize_clip(clipIndex + 1, 0, duration - 1);

            QModelIndex index = createIndex(clipIndex + 1, 0, trackIndex);
            QVector<int> roles;
            roles << DurationRole;
            emit dataChanged(index, index, roles);
        } else {
            qDebug() << "remove blank on right";
            int index = clipIndex + 1;
            beginRemoveRows(createIndex(trackIndex, 0, NO_PARENT_ID), index, index);
            playlist.remove(index);
            endRemoveRows();
            consolidateBlanks(playlist, trackIndex);
        }
    } else if (delta < 0 && (clipIndex + 1) < playlist.count()) {
        // Add blank to right.
        qDebug() << "add blank on right with duration" << -delta;
        beginInsertRows(createIndex(trackIndex, 0, NO_PARENT_ID), clipIndex + 1, clipIndex + 1);
        playlist.insert_blank(clipIndex + 1, (-delta - 1));
        endInsertRows();
    }
}

void MultitrackModel::consolidateBlanks(Mlt::Playlist &playlist, int trackIndex)
{
    for (int i = 1; i < playlist.count(); i++) {
        if (playlist.is_blank(i - 1) && playlist.is_blank(i)) {
            int out = playlist.clip_length(i - 1) + playlist.clip_length(i) - 1;
            playlist.resize_clip(i - 1, 0, out);
            QModelIndex index = createIndex(i - 1, 0, trackIndex);
            QVector<int> roles;
            roles << DurationRole;
            emit dataChanged(index, index, roles);
            beginRemoveRows(createIndex(trackIndex, 0, NO_PARENT_ID), i, i);
            playlist.remove(i--);
            endRemoveRows();
        }
        if (playlist.count() > 0) {
            int i = playlist.count() - 1;
            if (playlist.is_blank(i)) {
                beginRemoveRows(createIndex(trackIndex, 0, NO_PARENT_ID), i, i);
                playlist.remove(i);
            endRemoveRows();
            }
        }
    }
}

void MultitrackModel::audioLevelsReady(const QModelIndex& index)
{
    QVector<int> roles;
    roles << AudioLevelsRole;
    emit dataChanged(index, index, roles);
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
    getAudioLevels();
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
        else if (!track->get(kShotcutPlaylistProperty) && !track->get(kAudioTrackProperty)) {
            int hide = track->get_int("hide");
             // hide: 0 = a/v, 2 = muted video track
            if (track->get(kVideoTrackProperty) || hide == 0 || hide == 2) {
                Track t;
                t.mlt_index = i;
                t.type = VideoTrackType;
                t.number = v++;
                QString trackName = track->get(kTrackNameProperty);
                if (trackName.isEmpty())
                    trackName = QString("V%1").arg(v);
                track->set(kTrackNameProperty, trackName.toUtf8().constData());
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
        else if (!track->get(kShotcutPlaylistProperty) && !track->get(kVideoTrackProperty)) {
            int hide = track->get_int("hide");
            // hide: 1 = audio only track, 3 = muted audio-only track
            if (track->get(kAudioTrackProperty) || hide == 1 || hide == 3) {
                Track t;
                t.mlt_index = i;
                t.type = AudioTrackType;
                t.number = a++;
                QString trackName = track->get(kTrackNameProperty);
                if (trackName.isEmpty())
                    trackName = QString("A%1").arg(a);
                track->set(kTrackNameProperty, trackName.toUtf8().constData());
                m_trackList.append(t);
//                qDebug() << __FUNCTION__ << QString(track->get("id")) << i;
            }
        }
    }
}

static void deleteQVariantList(QVariantList* list)
{
    delete list;
}

class AudioLevelsTask : public QRunnable
{
    Mlt::Producer m_producer;
    MultitrackModel* m_model;
    QPersistentModelIndex m_index;
    Mlt::Producer* m_tempProducer;

public:
    AudioLevelsTask(Mlt::Producer& producer, MultitrackModel* model, const QModelIndex& index)
        : QRunnable()
        , m_producer(producer)
        , m_model(model)
        , m_index(index)
        , m_tempProducer(0)
    {
    }

    ~AudioLevelsTask()
    {
        delete m_tempProducer;
    }

    Mlt::Producer* tempProducer()
    {
        if (!m_tempProducer) {
            QString service = m_producer.get("mlt_service");
            if (service == "avformat")
                service = "avformat-novalidate";
            else if (service.startsWith("xml"))
                service = "xml-nogl";
            m_tempProducer = new Mlt::Producer(MLT.profile(), service.toUtf8().constData(), m_producer.get("resource"));
            if (m_tempProducer->is_valid()) {
                Mlt::Filter converter(MLT.profile(), "audioconvert");
                Mlt::Filter levels(MLT.profile(), "audiolevel");
                m_tempProducer->attach(converter);
                m_tempProducer->attach(levels);
            }
        }
        return m_tempProducer;
    }

    QString cacheKey()
    {
        QString key = QString("%1 audiolevels").arg(m_producer.get("resource"));
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(key.toUtf8());
        return hash.result().toHex();
    }

    void run()
    {
        int n = tempProducer()->get_playtime();
        // 2 channels interleaved of uchar values
        QVariantList* levels = new QVariantList;
        QImage image = DB.getThumbnail(cacheKey());
        if (image.isNull()) {
            const char* key[2] = { "meta.media.audio_level.0", "meta.media.audio_level.1"};
            // for each frame
            for (int i = 0; i < n; i++) {
                m_tempProducer->seek(i);
                Mlt::Frame* frame = m_tempProducer->get_frame();
                if (!frame->get_int("test_audio")) {
                    if (frame && frame->is_valid()) {
                        mlt_audio_format format = mlt_audio_s16;
                        int frequency = 48000;
                        // TODO: use project channel count
                        int channels = 2;
                        int samples = mlt_sample_calculator(m_producer.get_fps(), frequency, i);
                        frame->get_audio(format, frequency, channels, samples);
                        // for each channel
                        for (int channel = 0; channel < channels; channel++)
                            // Convert real to uint for caching as image.
                            // Scale by 0.9 because values may exceed 1.0 to indicate clipping.
                            *levels << 256 * qMin(frame->get_double(key[channel]) * 0.9, 1.0);
                    }
                }
                delete frame;
            }
            // Put into an image for caching.
            int channels = 2;
            QImage image((n + 3) / 4, channels, QImage::Format_ARGB32);
            n = image.width() * image.height();
            for (int i = 0; i < n; i ++) {
                QRgb p; 
                if ((4*i + 3) < levels->size()) {
                    p = qRgba(levels->at(4*i).toInt(), levels->at(4*i+1).toInt(), levels->at(4*i+2).toInt(), levels->at(4*i+3).toInt());
                } else {
                    int r = levels->at(4*i).toInt();
                    int g = (4*i+1) < levels->size() ? levels->at(4*i+1).toInt() : 0;
                    int b = (4*i+2) < levels->size() ? levels->at(4*i+2).toInt() : 0;
                    int a = 0;
                    p = qRgba(r, g, b, a);
                }
                image.setPixel(i / 2, i % channels, p);
            }
            DB.putThumbnail(cacheKey(), image);
        } else {
            // convert cached image
            int channels = 2;
            int n = image.width() * image.height();
            for (int i = 0; i < n; i++) {
                QRgb p = image.pixel(i / 2, i % channels);
                *levels << qRed(p);
                *levels << qGreen(p);
                *levels << qBlue(p);
                *levels << qAlpha(p);
            }
        }
        m_producer.set(kAudioLevelsProperty, levels, 0, (mlt_destructor) deleteQVariantList);
        if (m_index.isValid())
            m_model->audioLevelsReady(m_index);
    }
};

void MultitrackModel::getAudioLevels()
{
    for (int trackIx = 0; trackIx < m_trackList.size(); trackIx++) {
        int i = m_trackList.at(trackIx).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        Mlt::Playlist playlist(*track);
        for (int clipIx = 0; clipIx < playlist.count(); clipIx++) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(clipIx));
            if (clip && clip->is_valid() && !clip->is_blank() && clip->get_int("audio_index") > -1) {
                QModelIndex index = createIndex(clipIx, 0, trackIx);
                QThreadPool::globalInstance()->start(
                    new AudioLevelsTask(clip->parent(), this, index));
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
