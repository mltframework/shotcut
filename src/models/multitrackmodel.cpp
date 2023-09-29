/*
 * Copyright (c) 2013-2023 Meltytech, LLC
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
#include "settings.h"
#include "docks/playlistdock.h"
#include "util.h"
#include "audiolevelstask.h"
#include "shotcut_mlt_properties.h"
#include "controllers/filtercontroller.h"
#include "qmltypes/qmlmetadata.h"
#include "proxymanager.h"
#include "dialogs/longuitask.h"

#include <QScopedPointer>
#include <QApplication>
#include <qmath.h>
#include <QTimer>
#include <QMessageBox>

#include <Logger.h>

static const quintptr NO_PARENT_ID = quintptr(-1);
static const char *kShotcutDefaultTransition = "lumaMix";

MultitrackModel::MultitrackModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_tractor(0)
    , m_isMakingTransition(false)
{
    connect(this, SIGNAL(modified()), SLOT(adjustBackgroundDuration()));
    connect(this, SIGNAL(modified()), SLOT(adjustTrackFilters()));
    connect(this, SIGNAL(reloadRequested()), SLOT(reload()), Qt::QueuedConnection);
    connect(this, &MultitrackModel::created, this, &MultitrackModel::scaleFactorChanged);
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
//            LOG_DEBUG() << __FUNCTION__ << parent << i << n;
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
//            LOG_DEBUG() << __FUNCTION__ << index.row();
            QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(index.row()));
            if (info)
                switch (role) {
                case NameRole: {
                    QString result;
                    if (info->producer && info->producer->is_valid()) {
                        result = info->producer->get(kShotcutCaptionProperty);
                        if (result.isNull()) {
                            result = Util::baseName(ProxyManager::resource(*info->producer));
                            if (!::qstrcmp(info->producer->get("mlt_service"), "timewarp")) {
                                double speed = ::fabs(info->producer->get_double("warp_speed"));
                                result = QString("%1 (%2x)").arg(result).arg(speed);
                            }
                        }
                        if (result == "<producer>") {
                            result = QString::fromUtf8(info->producer->get("mlt_service"));
                        }
                        if (info->producer->get_int(kIsProxyProperty)) {
                            result.append("\n" + tr("(PROXY)"));
                        }
                    }
                    return result;
                }
                case CommentRole: {
                    QString result;
                    if (info->producer && info->producer->is_valid()) {
                        result = info->producer->get(kCommentProperty);
                    }
                    return result;
                }
                case ResourceRole:
                case Qt::DisplayRole: {
                    QString result = QString::fromUtf8(info->resource);
                    if (result == "<producer>" && info->producer
                            && info->producer->is_valid() && info->producer->get("mlt_service"))
                        result = QString::fromUtf8(info->producer->get("mlt_service"));
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
                    QVariant result;
                    if (info->producer && info->producer->is_valid()) {
                        info->producer->lock();
                        if (info->producer->get_data(kAudioLevelsProperty)) {
                            result = QVariant::fromValue(*((QVariantList *) info->producer->get_data(kAudioLevelsProperty)));
                        }
                        info->producer->unlock();
                    }
                    return result;
                }
                case FadeInRole: {
                    QScopedPointer<Mlt::Filter> filter(getFilter("fadeInVolume", info->producer));
                    if (!filter || !filter->is_valid())
                        filter.reset(getFilter("fadeInBrightness", info->producer));
                    if (!filter || !filter->is_valid())
                        filter.reset(getFilter("fadeInMovit", info->producer));
                    if (filter && filter->is_valid() && filter->get(kShotcutAnimInProperty))
                        return filter->get_int(kShotcutAnimInProperty);
                    else
                        return (filter && filter->is_valid()) ? filter->get_length() : 0;
                }
                case FadeOutRole: {
                    QScopedPointer<Mlt::Filter> filter(getFilter("fadeOutVolume", info->producer));
                    if (!filter || !filter->is_valid())
                        filter.reset(getFilter("fadeOutBrightness", info->producer));
                    if (!filter || !filter->is_valid())
                        filter.reset(getFilter("fadeOutMovit", info->producer));
                    if (filter && filter->is_valid() && filter->get(kShotcutAnimOutProperty))
                        return filter->get_int(kShotcutAnimOutProperty);
                    else
                        return (filter && filter->is_valid()) ? filter->get_length() : 0;
                }
                case IsTransitionRole:
                    return isTransition(playlist, index.row());
                case FileHashRole:
                    return Util::getHash(*info->producer);
                case SpeedRole: {
                    double speed = 1.0;
                    if (info->producer && info->producer->is_valid()) {
                        if (!qstrcmp("timewarp", info->producer->get("mlt_service")))
                            speed = info->producer->get_double("warp_speed");
                    }
                    return speed;
                }
                case IsFilteredRole:
                    return isFiltered(info->producer);
                case AudioIndexRole:
                    return QString::fromLatin1(info->producer->get("audio_index"));
                default:
                    break;
                }
        }
    } else {
        // Get data for a track.
        int i = m_trackList.at(index.row()).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            switch (role) {
            case NameRole:
            case Qt::DisplayRole:
                return QString::fromUtf8(track->get(kTrackNameProperty));
            case DurationRole:
                return playlist.get_playtime();
            case IsMuteRole:
                return playlist.get_int("hide") & 2;
            case IsHiddenRole:
                return playlist.get_int("hide") & 1;
            case IsAudioRole:
                return m_trackList[index.row()].type == AudioTrackType;
            case IsLockedRole:
                return track->get_int(kTrackLockProperty);
            case IsCompositeRole: {
                QScopedPointer<Mlt::Transition> transition(getVideoBlendTransition(i));
                if (transition && transition->is_valid()) {
                    if (!transition->get_int("disable"))
                        return true;
                }
                return false;
            }
            case IsFilteredRole:
                return isFiltered(track.data());
            case IsTopVideoRole:
                if (m_trackList[index.row()].type == AudioTrackType) return false;
                foreach (const Track &t, m_trackList) {
                    if (t.type == VideoTrackType && t.number > m_trackList[index.row()].number) {
                        return false;
                    }
                }
                return true;
            case IsBottomVideoRole:
                return m_trackList[index.row()].number == 0 && m_trackList[index.row()].type == VideoTrackType;
            case IsTopAudioRole:
                return m_trackList[index.row()].number == 0 && m_trackList[index.row()].type == AudioTrackType;
            case IsBottomAudioRole:
                if (m_trackList[index.row()].type == VideoTrackType) return false;
                foreach (const Track &t, m_trackList) {
                    if (t.type == AudioTrackType && t.number > m_trackList[index.row()].number) {
                        return false;
                    }
                }
                return true;
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
//    LOG_DEBUG() << __FUNCTION__ << row << column << parent;
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

QModelIndex MultitrackModel::makeIndex(int trackIndex, int clipIndex) const
{
    return index(clipIndex, 0, index(trackIndex));
}

QModelIndex MultitrackModel::parent(const QModelIndex &index) const
{
//    LOG_DEBUG() << __FUNCTION__ << index;
    if (!index.isValid() || index.internalId() == NO_PARENT_ID)
        return QModelIndex();
    else
        return createIndex(index.internalId(), 0, NO_PARENT_ID);
}

QHash<int, QByteArray> MultitrackModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[CommentRole] = "comment";
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
    roles[IsCompositeRole] = "composite";
    roles[IsLockedRole] = "locked";
    roles[FadeInRole] = "fadeIn";
    roles[FadeOutRole] = "fadeOut";
    roles[IsTransitionRole] = "isTransition";
    roles[FileHashRole] = "hash";
    roles[SpeedRole] = "speed";
    roles[IsFilteredRole] = "filtered";
    roles[IsTopVideoRole] = "isTopVideo";
    roles[IsBottomVideoRole] = "isBottomVideo";
    roles[IsTopAudioRole] = "isTopAudio";
    roles[IsBottomAudioRole] = "isBottomAudio";
    roles[AudioIndexRole] = "audioIndex";
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
            emit modified();
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
            emit modified();
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
            emit modified();
        }
    }
}

void MultitrackModel::setTrackComposite(int row, bool composite)
{
    if (row < m_trackList.size()) {
        int i = m_trackList.at(row).mlt_index;
        QScopedPointer<Mlt::Transition> transition(getVideoBlendTransition(i));
        if (transition && transition->is_valid()) {
            transition->set("disable", !composite);
        }
        MLT.refreshConsumer();

        QModelIndex modelIndex = index(row, 0);
        QVector<int> roles;
        roles << IsCompositeRole;
        emit dataChanged(modelIndex, modelIndex, roles);
        emit modified();
    }
}

void MultitrackModel::setTrackLock(int row, bool lock)
{
    if (row < m_trackList.size()) {
        int i = m_trackList.at(row).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        track->set(kTrackLockProperty, lock);

        QModelIndex modelIndex = index(row, 0);
        QVector<int> roles;
        roles << IsLockedRole;
        emit dataChanged(modelIndex, modelIndex, roles);
        emit modified();
    }
}

bool MultitrackModel::trimClipInValid(int trackIndex, int clipIndex, int delta, bool ripple)
{
    bool result = true;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));

        if (!info || (info->frame_in + delta) < 0 || (info->frame_in + delta) > info->frame_out)
            result = false;
        else if (!ripple && delta < 0 && clipIndex <= 0)
            result = false;
        else if (!ripple && delta < 0 && clipIndex > 0 && !playlist.is_blank(clipIndex - 1))
            result = false;
        else if (!ripple && delta > 0 && clipIndex > 0 && isTransition(playlist, clipIndex - 1))
            result = false;
    }
    return result;
}

int MultitrackModel::trimClipIn(int trackIndex, int clipIndex, int delta, bool ripple,
                                bool rippleAllTracks)
{
    int result = clipIndex;
    QList<int> otherTracksToRipple;
    int otherTracksPosition = -1;

    for (int i = 0; i < m_trackList.count(); ++i) {
        int mltIndex = m_trackList.at(i).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(mltIndex));
        if (!track)
            continue;

        //when not rippling, never touch the other tracks
        if (trackIndex != i && (!ripple || !rippleAllTracks))
            continue;

        if (rippleAllTracks) {
            if (track->get_int(kTrackLockProperty))
                continue;

            if (trackIndex != i && ripple) {
                otherTracksToRipple << i;
                continue;
            }
        }

        Mlt::Playlist playlist(*track);
        if (!playlist.is_valid()) {
            LOG_DEBUG() << "Invalid Playlist" << trackIndex;
            continue;
        }
        if (clipIndex >= playlist.count()) {
            LOG_DEBUG() << "Invalid Clip Index" << clipIndex;
            continue;
        }
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        if (!info) {
            LOG_DEBUG() << "Invalid clip info";
            continue;
        }
        // These are used to adjust filters but must be retrieved before changing clip length.
        int filterIn = MLT.filterIn(playlist, clipIndex);
        int filterOut = MLT.filterOut(playlist, clipIndex);

        Q_ASSERT(otherTracksPosition == -1);
        otherTracksPosition = info->start;

        if (info->frame_in + delta < 0)
            // clamp to clip start
            delta = -info->frame_in;
        if (clipIndex > 0 && playlist.is_blank(clipIndex - 1)
                && -delta > playlist.clip_length(clipIndex - 1))
            // clamp to duration of blank space
            delta = -playlist.clip_length(clipIndex - 1);
//        LOG_DEBUG() << "delta" << delta;

        playlist.resize_clip(clipIndex, info->frame_in + delta, info->frame_out);

        // Adjust filters.
        MLT.adjustClipFilters(*info->producer, filterIn, filterOut, delta, 0, delta);

        QModelIndex modelIndex = createIndex(clipIndex, 0, i);
        QVector<int> roles;
        roles << DurationRole;
        roles << InPointRole;
        emit dataChanged(modelIndex, modelIndex, roles);
        AudioLevelsTask::start(*info->producer, this, modelIndex);

        if (!ripple) {
            // Adjust left of the clip.
            if (clipIndex > 0 && playlist.is_blank(clipIndex - 1)) {
                int out = playlist.clip_length(clipIndex - 1) + delta - 1;
                if (out < 0) {
                    //                LOG_DEBUG() << "remove blank at left";
                    beginRemoveRows(index(i), clipIndex - 1, clipIndex - 1);
                    playlist.remove(clipIndex - 1);
                    endRemoveRows();
                    --result;
                } else {
                    //                LOG_DEBUG() << "adjust blank on left to" << out;
                    playlist.resize_clip(clipIndex - 1, 0, out);

                    QModelIndex index = createIndex(clipIndex - 1, 0, i);
                    QVector<int> roles;
                    roles << DurationRole;
                    emit dataChanged(index, index, roles);
                }
            } else if (delta > 0) {
                //            LOG_DEBUG() << "add blank on left duration" << delta - 1;
                beginInsertRows(index(i), clipIndex, clipIndex);
                playlist.insert_blank(clipIndex, delta - 1);
                endInsertRows();
                ++result;
            }
        }
        emit modified();
    }
    if (delta > 0) {
        foreach (int idx, otherTracksToRipple) {
            Q_ASSERT(otherTracksPosition != -1);
            removeRegion(idx, otherTracksPosition, delta);
        }
    } else {
        insertOrAdjustBlankAt(otherTracksToRipple, otherTracksPosition, -delta);
    }
    return result;
}

void MultitrackModel::notifyClipIn(int trackIndex, int clipIndex)
{
    if (trackIndex >= 0 && trackIndex < m_trackList.size() && clipIndex >= 0) {
        QModelIndex index = createIndex(clipIndex, 0, trackIndex);
        QVector<int> roles;
        roles << AudioLevelsRole;
        emit dataChanged(index, index, roles);
        MLT.refreshConsumer();
    }
    m_isMakingTransition = false;
}

bool MultitrackModel::trimClipOutValid(int trackIndex, int clipIndex, int delta, bool ripple)
{
    bool result = true;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        if (!info || (info->frame_out - delta) >= info->length
                || (info->frame_out - delta) < info->frame_in)
            result = false;
        else if (!ripple && delta < 0 && (clipIndex + 1) < playlist.count()
                 && !playlist.is_blank(clipIndex + 1))
            result = false;
        else if (!ripple && delta > 0 && (clipIndex + 1) < playlist.count()
                 && isTransition(playlist, clipIndex + 1))
            return false;
    }
    return result;
}

int MultitrackModel::trackHeight() const
{
    int result = m_tractor ? m_tractor->get_int(kTrackHeightProperty) : Settings.timelineTrackHeight();
    return qBound(10, result ? result : Settings.timelineTrackHeight(), 150);
}

void MultitrackModel::setTrackHeight(int height)
{
    if (m_tractor) {
        Settings.setTimelineTrackHeight(qBound(10, height, 150));
        m_tractor->set(kTrackHeightProperty, Settings.timelineTrackHeight());
        emit trackHeightChanged();
    }
}

double MultitrackModel::scaleFactor() const
{
    double result = m_tractor ? m_tractor->get_double(kTimelineScaleProperty) : 0;
    return (result > 0) ? qBound(0.0, result, 27.01) : (qPow(1.0, 3.0) + 0.01);
}

void MultitrackModel::setScaleFactor(double scale)
{
    if (m_tractor) {
        m_tractor->set(kTimelineScaleProperty, qBound(0.0, scale, 27.01));
        emit scaleFactorChanged();
    }
}

int MultitrackModel::trimClipOut(int trackIndex, int clipIndex, int delta, bool ripple,
                                 bool rippleAllTracks)
{
    QList<int> otherTracksToRipple;
    int result = clipIndex;
    int otherTracksPosition = -1;

    for (int i = 0; i < m_trackList.count(); ++i) {
        int mltIndex = m_trackList.at(i).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_tractor->track(mltIndex));
        if (!track)
            continue;

        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        // These are used to adjust filters but must be retrieved before changing clip length.
        int filterIn = MLT.filterIn(playlist, clipIndex);
        int filterOut = MLT.filterOut(playlist, clipIndex);

        //when not rippling, never touch the other tracks
        if (trackIndex != i && (!ripple || !rippleAllTracks))
            continue;

        if (rippleAllTracks) {
            if (track->get_int(kTrackLockProperty))
                continue;

            if (trackIndex != i && ripple) {
                otherTracksToRipple << i;
                continue;
            }
        }

        Q_ASSERT(otherTracksPosition == -1);
        otherTracksPosition = info->start + info->frame_count;

        if ((info->frame_out - delta) >= info->length)
            // clamp to clip duration
            delta = info->frame_out - info->length + 1;
        if ((clipIndex + 1) < playlist.count() && playlist.is_blank(clipIndex + 1)
                && -delta > playlist.clip_length(clipIndex + 1))
            delta = -playlist.clip_length(clipIndex + 1);
//        LOG_DEBUG() << "delta" << delta;

        if (!ripple) {
            // Adjust right of the clip.
            if (clipIndex >= 0 && (clipIndex + 1) < playlist.count() && playlist.is_blank(clipIndex + 1)) {
                int out = playlist.clip_length(clipIndex + 1) + delta - 1;
                if (out < 0) {
                    //                LOG_DEBUG() << "remove blank at right";
                    beginRemoveRows(index(i), clipIndex + 1, clipIndex + 1);
                    playlist.remove(clipIndex + 1);
                    endRemoveRows();
                } else {
                    //                LOG_DEBUG() << "adjust blank on right to" << out;
                    playlist.resize_clip(clipIndex + 1, 0, out);

                    QModelIndex index = createIndex(clipIndex + 1, 0, i);
                    QVector<int> roles;
                    roles << DurationRole;
                    emit dataChanged(index, index, roles);
                }
            } else if (delta > 0 && (clipIndex + 1) < playlist.count())  {
                // Add blank to right.
                //            LOG_DEBUG() << "add blank on right duration" << (delta - 1);
                int newIndex = clipIndex + 1;
                beginInsertRows(index(i), newIndex, newIndex);
                playlist.insert_blank(newIndex, delta - 1);
                endInsertRows();
            }
        }
        playlist.resize_clip(clipIndex, info->frame_in, info->frame_out - delta);

        // Adjust filters.
        MLT.adjustClipFilters(*info->producer, filterIn, filterOut, 0, delta, 0);

        QModelIndex index = createIndex(clipIndex, 0, i);
        QVector<int> roles;
        roles << DurationRole;
        roles << OutPointRole;
        emit dataChanged(index, index, roles);
        AudioLevelsTask::start(*info->producer, this, index);
        emit modified();
    }
    if (delta > 0) {
        foreach (int idx, otherTracksToRipple) {
            Q_ASSERT(otherTracksPosition != -1);
            removeRegion(idx, otherTracksPosition - delta, delta);
        }
    } else {
        insertOrAdjustBlankAt(otherTracksToRipple, otherTracksPosition, -delta);
    }
    return result;
}

void MultitrackModel::notifyClipOut(int trackIndex, int clipIndex)
{
    if (trackIndex >= 0 && trackIndex < m_trackList.size() && clipIndex >= 0) {
        QModelIndex index = createIndex(clipIndex, 0, trackIndex);
        QVector<int> roles;
        roles << AudioLevelsRole;
        emit dataChanged(index, index, roles);
        MLT.refreshConsumer();
    }
    m_isMakingTransition = false;
}

bool MultitrackModel::moveClip(int fromTrack, int toTrack, int clipIndex,
                               int position, bool ripple, bool rippleAllTracks)
{
//    LOG_DEBUG() << __FUNCTION__ << clipIndex << "fromTrack" << fromTrack << "toTrack" << toTrack;
    bool result = false;
    int i = m_trackList.at(fromTrack).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));

    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        QString xml = MLT.XML(info->producer);
        Mlt::Producer clip(MLT.profile(), "xml-string", xml.toUtf8().constData());

        if (clip.is_valid()) {
            clearMixReferences(fromTrack, clipIndex);
            clip.set_in_and_out(info->frame_in, info->frame_out);

            if (ripple) {
                int targetIndex = playlist.get_clip_index_at(position);
                int length = playlist.clip_length(clipIndex);
                int targetIndexEnd = playlist.get_clip_index_at(position + length - 1);
                if ((clipIndex + 1) < playlist.count() && position >= playlist.get_playtime()) {
                    // Clip relocated to end of playlist.
                    moveClipToEnd(playlist, toTrack, clipIndex, position, ripple, rippleAllTracks);
                    emit modified();
                } else if (fromTrack == toTrack && targetIndex >= clipIndex) {
                    // Push the clips.
                    int clipStart = playlist.clip_start(clipIndex);
                    int duration = position - clipStart;
                    QList<int> trackList;
                    trackList << fromTrack;
                    if (rippleAllTracks) {
                        for (int i = 0; i < m_trackList.count(); ++i) {
                            if (i == fromTrack)
                                continue;
                            int mltIndex = m_trackList.at(i).mlt_index;
                            QScopedPointer<Mlt::Producer> otherTrack(m_tractor->track(mltIndex));
                            if (otherTrack && otherTrack->get_int(kTrackLockProperty))
                                continue;
                            trackList << i;
                        }
                    }
                    insertOrAdjustBlankAt(trackList, clipStart, duration);
                    consolidateBlanks(playlist, fromTrack);
                    emit modified();
                } else if (fromTrack == toTrack && (playlist.is_blank_at(position) || targetIndex == clipIndex) &&
                           (playlist.is_blank_at(position + length - 1) || targetIndexEnd == clipIndex)) {
                    // Reposition the clip within its current blank spot.
                    moveClipInBlank(playlist, toTrack, clipIndex, position, ripple, rippleAllTracks);
                    emit modified();
                } else {
                    int clipPlaytime = clip.get_playtime();
                    int clipStart = playlist.clip_start(clipIndex);

                    // Remove clip
                    clearMixReferences(fromTrack, clipIndex);
                    emit removing(playlist.get_clip(clipIndex));
                    beginRemoveRows(index(fromTrack), clipIndex, clipIndex);
                    playlist.remove(clipIndex);
                    endRemoveRows();
                    consolidateBlanks(playlist, fromTrack);

                    // Ripple delete on all unlocked tracks.
                    if (clipPlaytime > 0 && rippleAllTracks)
                        for (int j = 0; j < m_trackList.count(); ++j) {
                            if (j == fromTrack)
                                continue;

                            int mltIndex = m_trackList.at(j).mlt_index;
                            QScopedPointer<Mlt::Producer> otherTrack(m_tractor->track(mltIndex));
                            if (otherTrack) {
                                if (otherTrack->get_int(kTrackLockProperty))
                                    continue;

                                removeRegion(j, clipStart, clipPlaytime);
                            }
                        }
                    consolidateBlanks(playlist, fromTrack);

                    // Insert clip
                    insertClip(toTrack, clip, position, rippleAllTracks, false);
                }
            } else {
                // Lift clip
                emit removing(playlist.get_clip(clipIndex));
                delete playlist.replace_with_blank(clipIndex);

                QModelIndex index = createIndex(clipIndex, 0, fromTrack);
                QVector<int> roles;
                roles << ResourceRole;
                roles << ServiceRole;
                roles << IsBlankRole;
                roles << IsTransitionRole;
                emit dataChanged(index, index, roles);

                consolidateBlanks(playlist, fromTrack);

                // Overwrite with clip
                if (position + clip.get_playtime() >= 0)
                    overwrite(toTrack, clip, position, false /* seek */);
                else
                    emit modified();
            }
        }
        result = true;
    }
    return result;
}

int MultitrackModel::overwriteClip(int trackIndex, Mlt::Producer &clip, int position, bool seek)
{
    createIfNeeded();
    int result = -1;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (position >= playlist.get_playtime() - 1) {
//            LOG_DEBUG() << __FUNCTION__ << "appending";
            removeBlankPlaceholder(playlist, trackIndex);
            int n = playlist.count();
            int length = position - playlist.clip_start(n - 1) - playlist.clip_length(n - 1);

            // Add blank to end if needed.
            if (length > 0) {
                beginInsertRows(index(trackIndex), n, n);
                playlist.blank(length - 1);
                endInsertRows();
                ++n;
            }

            // Append clip.
            int in = clip.get_in();
            int out = clip.get_out();
            clip.set_in_and_out(0, clip.get_length() - 1);
            beginInsertRows(index(trackIndex), n, n);
            playlist.append(clip.parent(), in, out);
            endInsertRows();
            AudioLevelsTask::start(clip.parent(), this, createIndex(n, 0, trackIndex));
            result = playlist.count() - 1;
        } else if (position + clip.get_playtime() > playlist.get_playtime()
                   // Handle straddling - new clip larger than another with blanks on both sides.
                   || playlist.get_clip_index_at(position) == playlist.get_clip_index_at(
                       position + clip.get_playtime() - 1)) {
//            LOG_DEBUG() << __FUNCTION__ << "overwriting blank space" << clip.get_playtime();
            int targetIndex = playlist.get_clip_index_at(position);

            if (position > playlist.clip_start(targetIndex)) {
                splitClip(trackIndex, targetIndex, position);

                // Notify item on left was adjusted.
                QModelIndex modelIndex = createIndex(targetIndex, 0, trackIndex);
                QVector<int> roles;
                roles << DurationRole;
                emit dataChanged(modelIndex, modelIndex, roles);
                AudioLevelsTask::start(clip.parent(), this, modelIndex);
                ++targetIndex;
            } else if (position < 0) {
                clip.set_in_and_out(-position, clip.get_out());
                QModelIndex modelIndex = createIndex(targetIndex, 0, trackIndex);
                // Notify clip on right was adjusted.
                QVector<int> roles;
                roles << InPointRole;
                roles << DurationRole;
                emit dataChanged(modelIndex, modelIndex, roles);
            }

            // Adjust clip on right.
            int duration = playlist.clip_length(targetIndex) - clip.get_playtime();
            if (duration > 0) {
//                LOG_DEBUG() << "adjust item on right" << (targetIndex) << " to" << duration;
                playlist.resize_clip(targetIndex, 0, duration - 1);
                QModelIndex modelIndex = createIndex(targetIndex, 0, trackIndex);
                // Notify clip on right was adjusted.
                QVector<int> roles;
                roles << DurationRole;
                emit dataChanged(modelIndex, modelIndex, roles);
                AudioLevelsTask::start(clip.parent(), this, modelIndex);
            } else {
//                LOG_DEBUG() << "remove item on right";
                clearMixReferences(trackIndex, targetIndex);
                beginRemoveRows(index(trackIndex), targetIndex, targetIndex);
                playlist.remove(targetIndex);
                endRemoveRows();
            }
            // Insert clip between subclips.
            int in = clip.get_in();
            int out = clip.get_out();
            clip.set_in_and_out(0, clip.get_length() - 1);
            beginInsertRows(index(trackIndex), targetIndex, targetIndex);
            playlist.insert(clip.parent(), targetIndex, in, out);
            endInsertRows();
            result = targetIndex;
        }
        if (result >= 0) {
            QModelIndex index = createIndex(result, 0, trackIndex);
            AudioLevelsTask::start(clip.parent(), this, index);
            emit modified();
            if (seek)
                emit seeked(playlist.clip_start(result) + playlist.clip_length(result));
        }
    }
    return result;
}

QString MultitrackModel::overwrite(int trackIndex, Mlt::Producer &clip, int position, bool seek,
                                   bool notify)
{
    createIfNeeded();
    Mlt::Playlist result(MLT.profile());
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        removeBlankPlaceholder(playlist, trackIndex);
        int targetIndex = playlist.get_clip_index_at(position);
        if (position >= playlist.get_playtime() - 1) {
//            LOG_DEBUG() << __FUNCTION__ << "appending";
            int n = playlist.count();
            int length = position - playlist.clip_start(n - 1) - playlist.clip_length(n - 1);

            // Add blank to end if needed.
            if (length > 0) {
                beginInsertRows(index(trackIndex), n, n);
                playlist.blank(length - 1);
                endInsertRows();
                ++n;
            }

            // Append clip.
            int in = clip.get_in();
            int out = clip.get_out();
            clip.set_in_and_out(0, clip.get_length() - 1);
            beginInsertRows(index(trackIndex), n, n);
            playlist.append(clip.parent(), in, out);
            endInsertRows();
            targetIndex = playlist.count() - 1;
        } else {
            int lastIndex = playlist.get_clip_index_at(position + clip.get_playtime());
//            LOG_DEBUG() << __FUNCTION__ << "overwriting with duration" << clip.get_playtime()
//                << "from" << targetIndex << "to" << lastIndex;

            // Add affected clips to result playlist.
            int i = targetIndex;
            if (position == playlist.clip_start(targetIndex))
                --i;
            for (; i <= lastIndex; i++) {
                Mlt::Producer *producer = playlist.get_clip(i);
                if (producer)
                    result.append(*producer);
                delete producer;
            }

            if (position > playlist.clip_start(targetIndex)) {
//                LOG_DEBUG() << "split starting item" <<  targetIndex;
                splitClip(trackIndex, targetIndex, position);
                ++targetIndex;
            } else if (position < 0) {
                clip.set_in_and_out(clip.get_in() - position, clip.get_out());
                position = 0;
                QModelIndex modelIndex = createIndex(targetIndex, 0, trackIndex);
                // Notify clip was adjusted.
                QVector<int> roles;
                roles << InPointRole;
                roles << DurationRole;
                emit dataChanged(modelIndex, modelIndex, roles);
            }

            int length = clip.get_playtime();
            while (length > 0 && targetIndex < playlist.count()) {
                if (playlist.clip_length(targetIndex) > length) {
//                    LOG_DEBUG() << "split last item" << targetIndex;
                    splitClip(trackIndex, targetIndex, position + length);
                }
//                LOG_DEBUG() << "length" << length << "item length" << playlist.clip_length(targetIndex);
                length -= playlist.clip_length(targetIndex);
//                LOG_DEBUG() << "delete item" << targetIndex;
                clearMixReferences(trackIndex, targetIndex);
                beginRemoveRows(index(trackIndex), targetIndex, targetIndex);
                playlist.remove(targetIndex);
                endRemoveRows();
            }

            // Insert clip between subclips.
            int in = clip.get_in();
            int out = clip.get_out();
            clip.set_in_and_out(0, clip.get_length() - 1);
            beginInsertRows(index(trackIndex), targetIndex, targetIndex);
            playlist.insert(clip.parent(), targetIndex, in, out);
            endInsertRows();
        }
        QModelIndex index = createIndex(targetIndex, 0, trackIndex);
        AudioLevelsTask::start(clip.parent(), this, index);
        if (notify) {
            emit overWritten(trackIndex, targetIndex);
            emit modified();
            emit seeked(playlist.clip_start(targetIndex) + playlist.clip_length(targetIndex), seek);
        }
    }
    return MLT.XML(&result);
}

int MultitrackModel::insertClip(int trackIndex, Mlt::Producer &clip, int position,
                                bool rippleAllTracks, bool seek, bool notify)
{
    createIfNeeded();
    int result = -1;
    int i = m_trackList.at(trackIndex).mlt_index;
    int clipPlaytime = clip.get_playtime();
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (position >= playlist.get_playtime() - 1) {
//            LOG_DEBUG() << __FUNCTION__ << "appending";
            removeBlankPlaceholder(playlist, trackIndex);
            int n = playlist.count();
            int length = position - playlist.clip_start(n - 1) - playlist.clip_length(n - 1);

            // Add blank to end if needed.
            if (length > 0) {
                beginInsertRows(index(trackIndex), n, n);
                playlist.blank(length - 1);
                endInsertRows();
                ++n;
            }

            // Append clip.
            int in = clip.get_in();
            int out = clip.get_out();
            clip.set_in_and_out(0, clip.get_length() - 1);
            beginInsertRows(index(trackIndex), n, n);
            playlist.append(clip.parent(), in, out);
            endInsertRows();
            result = playlist.count() - 1;
        } else {
//            LOG_DEBUG() << __FUNCTION__ << "inserting" << position << MLT.XML(&clip);
            int targetIndex = playlist.get_clip_index_at(position);

            if (position > playlist.clip_start(targetIndex)) {
                splitClip(trackIndex, targetIndex, position);

                // Notify item on left was adjusted.
                QModelIndex modelIndex = createIndex(targetIndex, 0, trackIndex);
                QVector<int> roles;
                roles << DurationRole;
                emit dataChanged(modelIndex, modelIndex, roles);
                AudioLevelsTask::start(clip.parent(), this, modelIndex);
                ++targetIndex;

                // Notify item on right was adjusted.
                modelIndex = createIndex(targetIndex, 0, trackIndex);
                emit dataChanged(modelIndex, modelIndex, roles);
                AudioLevelsTask::start(clip.parent(), this, modelIndex);
            } else if (position < 0) {
                clip.set_in_and_out(clip.get_in() - position, clip.get_out());
                position = 0;
                QModelIndex modelIndex = createIndex(targetIndex, 0, trackIndex);
                // Notify clip was adjusted.
                QVector<int> roles;
                roles << InPointRole;
                roles << DurationRole;
                emit dataChanged(modelIndex, modelIndex, roles);
            }

            // Insert clip between split blanks.
            beginInsertRows(index(trackIndex), targetIndex, targetIndex);
            if (qstrcmp("blank", clip.get("mlt_service"))) {
                int in = clip.get_in();
                int out = clip.get_out();
                clip.set_in_and_out(0, clip.get_length() - 1);
                playlist.insert(clip.parent(), targetIndex, in, out);
            } else {
                playlist.insert_blank(targetIndex, clipPlaytime - 1);
            }
            endInsertRows();
            result = targetIndex;
        }
        if (result >= 0) {
            if (rippleAllTracks) {
                //fill in/expand blanks in all the other tracks
                QList<int> tracksToInsertBlankInto;
                for (int j = 0; j < m_trackList.count(); ++j) {
                    if (j == trackIndex)
                        continue;
                    int mltIndex = m_trackList.at(j).mlt_index;
                    QScopedPointer<Mlt::Producer> otherTrack(m_tractor->track(mltIndex));
                    if (otherTrack->get_int(kTrackLockProperty))
                        continue;

                    tracksToInsertBlankInto << j;
                }
                if (!tracksToInsertBlankInto.isEmpty())
                    insertOrAdjustBlankAt(tracksToInsertBlankInto, position, clipPlaytime);
            }

            QModelIndex index = createIndex(result, 0, trackIndex);
            AudioLevelsTask::start(clip.parent(), this, index);
            if (notify) {
                emit inserted(trackIndex, result);
                emit modified();
                emit seeked(playlist.clip_start(result) + playlist.clip_length(result), seek);
            }
        }
    }
    return result;
}

int MultitrackModel::appendClip(int trackIndex, Mlt::Producer &clip, bool seek, bool notify)
{
    if (!createIfNeeded()) {
        return -1;
    }
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        removeBlankPlaceholder(playlist, trackIndex);
        i = playlist.count();
        int in = clip.get_in();
        int out = clip.get_out();
        clip.set_in_and_out(0, clip.get_length() - 1);
        beginInsertRows(index(trackIndex), i, i);
        playlist.append(clip.parent(), in, out);
        endInsertRows();
        QModelIndex index = createIndex(i, 0, trackIndex);
        AudioLevelsTask::start(clip.parent(), this, index);
        if (notify) {
            emit appended(trackIndex, i);
            emit modified();
            emit seeked(playlist.clip_start(i) + playlist.clip_length(i), seek);
        }
        return i;
    }
    return -1;
}

void MultitrackModel::removeClip(int trackIndex, int clipIndex, bool rippleAllTracks)
{
    if (trackIndex >= m_trackList.size()) {
        return;
    }

    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    int clipPlaytime = -1;
    int clipStart = -1;

    if (track) {
        Mlt::Playlist playlist(*track);
        if (clipIndex < playlist.count()) {
            // Shotcut does not like the behavior of remove() on a
            // transition (MLT mix clip). So, we null mlt_mix to prevent it.
            clearMixReferences(trackIndex, clipIndex);

            QScopedPointer<Mlt::Producer> producer(playlist.get_clip(clipIndex));
            if (producer) {
                clipPlaytime = producer->get_playtime();
                clipStart = playlist.clip_start(clipIndex);
            }

            emit removing(playlist.get_clip(clipIndex));
            beginRemoveRows(index(trackIndex), clipIndex, clipIndex);
            playlist.remove(clipIndex);
            endRemoveRows();
            consolidateBlanks(playlist, trackIndex);

            // Ripple all unlocked tracks.
            if (clipPlaytime > 0 && rippleAllTracks)
                for (int j = 0; j < m_trackList.count(); ++j) {
                    if (j == trackIndex)
                        continue;

                    int mltIndex = m_trackList.at(j).mlt_index;
                    QScopedPointer<Mlt::Producer> otherTrack(m_tractor->track(mltIndex));
                    if (otherTrack) {
                        if (otherTrack->get_int(kTrackLockProperty))
                            continue;

                        removeRegion(j, clipStart, clipPlaytime);
                    }
                }
            consolidateBlanks(playlist, trackIndex);
            emit modified();
        }
    }
}

void MultitrackModel::liftClip(int trackIndex, int clipIndex)
{
    if (trackIndex >= m_trackList.size()) {
        return;
    }

    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (clipIndex < playlist.count()) {
            // Shotcut does not like the behavior of replace_with_blank() on a
            // transition (MLT mix clip). So, we null mlt_mix to prevent it.
            clearMixReferences(trackIndex, clipIndex);

            emit removing(playlist.get_clip(clipIndex));
            delete playlist.replace_with_blank(clipIndex);

            QModelIndex index = createIndex(clipIndex, 0, trackIndex);
            QVector<int> roles;
            roles << ResourceRole;
            roles << ServiceRole;
            roles << IsBlankRole;
            roles << IsTransitionRole;
            emit dataChanged(index, index, roles);

            consolidateBlanks(playlist, trackIndex);

            emit modified();
        }
    }
}

void MultitrackModel::splitClip(int trackIndex, int clipIndex, int position)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        int in = info->frame_in;
        int out = info->frame_out;
        int filterIn = MLT.filterIn(playlist, clipIndex);
        int filterOut = MLT.filterOut(playlist, clipIndex);
        int duration = position - playlist.clip_start(clipIndex);
        int delta = info->frame_count - duration;

        if (playlist.is_blank(clipIndex)) {
            beginInsertRows(index(trackIndex), clipIndex, clipIndex);
            playlist.insert_blank(clipIndex, duration - 1);
            endInsertRows();
        } else {
            // Make copy of clip.
            Mlt::Producer producer(MLT.profile(), "xml-string",
                                   MLT.XML(info->producer).toUtf8().constData());

            // Connect a transition on the left to the new producer.
            if (isTransition(playlist, clipIndex - 1) && !playlist.is_blank(clipIndex)) {
                QScopedPointer<Mlt::Producer> p(playlist.get_clip(clipIndex - 1));
                Mlt::Tractor tractor(p->parent());
                if (tractor.is_valid()) {
                    QScopedPointer<Mlt::Producer> track_b(tractor.track(1));
                    track_b.reset(producer.cut(track_b->get_in(), track_b->get_out()));
                    tractor.set_track(*track_b, 1);
                }
            }

            // Remove fades that are usually not desired after split.
            QScopedPointer<Mlt::Filter> filter(getFilter("fadeOutVolume", &producer));
            if (filter && filter->is_valid())
                producer.detach(*filter);
            filter.reset(getFilter("fadeOutBrightness", &producer));
            if (filter && filter->is_valid())
                producer.detach(*filter);
            filter.reset(getFilter("fadeOutMovit", &producer));
            if (filter && filter->is_valid())
                producer.detach(*filter);
            filter.reset(getFilter("fadeInVolume", info->producer));
            if (filter && filter->is_valid())
                info->producer->detach(*filter);
            filter.reset(getFilter("fadeInBrightness", info->producer));
            if (filter && filter->is_valid())
                info->producer->detach(*filter);
            filter.reset(getFilter("fadeInMovit", info->producer));
            if (filter && filter->is_valid())
                info->producer->detach(*filter);

            beginInsertRows(index(trackIndex), clipIndex, clipIndex);
            playlist.insert(producer, clipIndex, in, in + duration - 1);
            endInsertRows();
            QModelIndex modelIndex = createIndex(clipIndex, 0, trackIndex);
            AudioLevelsTask::start(producer.parent(), this, modelIndex);
            MLT.adjustClipFilters(producer, filterIn, out, 0, delta, 0);
        }

        playlist.resize_clip(clipIndex + 1, in + duration, out);
        QModelIndex modelIndex = createIndex(clipIndex + 1, 0, trackIndex);
        QVector<int> roles;
        roles << DurationRole;
        roles << InPointRole;
        roles << FadeInRole;
        emit dataChanged(modelIndex, modelIndex, roles);

        if (!playlist.is_blank(clipIndex + 1)) {
            AudioLevelsTask::start(*info->producer, this, modelIndex);
            MLT.adjustClipFilters(*info->producer, in, filterOut, duration, 0, duration);
        }

        emit modified();
    }
}

void MultitrackModel::joinClips(int trackIndex, int clipIndex)
{
    if (clipIndex < 0) return;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (clipIndex >= playlist.count() - 1) return;
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        int in = info->frame_in;
        int out = info->frame_out;
        int delta = -playlist.clip_length(clipIndex + 1);

        // Move a fade out on the right clip onto the left clip.
        QScopedPointer<Mlt::Producer> clip(playlist.get_clip(clipIndex));
        info.reset(playlist.clip_info(clipIndex + 1));
        QScopedPointer<Mlt::Filter> filter(getFilter("fadeOutVolume", info->producer));
        if (filter && filter->is_valid())
            clip->parent().attach(*filter);
        filter.reset(getFilter("fadeOutBrightness", info->producer));
        if (filter && filter->is_valid())
            clip->parent().attach(*filter);
        filter.reset(getFilter("fadeOutMovit", info->producer));
        if (filter && filter->is_valid())
            clip->parent().attach(*filter);

        playlist.resize_clip(clipIndex, in, out - delta);
        QModelIndex modelIndex = createIndex(clipIndex, 0, trackIndex);
        QVector<int> roles;
        roles << DurationRole;
        roles << OutPointRole;
        roles << FadeOutRole;
        emit dataChanged(modelIndex, modelIndex, roles);
        AudioLevelsTask::start(clip->parent(), this, modelIndex);

        clearMixReferences(trackIndex, clipIndex + 1);
        emit removing(playlist.get_clip(clipIndex + 1));
        beginRemoveRows(index(trackIndex), clipIndex + 1, clipIndex + 1);
        playlist.remove(clipIndex + 1);
        endRemoveRows();

        MLT.adjustClipFilters(clip->parent(), in, out, 0, delta, 0);

        emit modified();
    }
}

static void moveBeforeFirstAudioFilter(Mlt::Producer *producer)
{
    int n = producer->filter_count();
    int index = 0;
    for (; index < n; index++) {
        QScopedPointer<Mlt::Filter> filter(producer->filter(index));
        if (filter && filter->is_valid() && !filter->get_int("_loader")) {
            QmlMetadata *meta = MAIN.filterController()->metadataForService(filter.data());
            if (meta && meta->isAudio()) {
                break;
            }
        }
    }
    producer->move_filter(n - 1, index);
}

void MultitrackModel::fadeIn(int trackIndex, int clipIndex, int duration)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        if (info && info->producer && info->producer->is_valid()) {
            bool isChanged = false;
            QScopedPointer<Mlt::Filter> filter;
            duration = qBound(0, duration, info->frame_count);

            if (m_trackList[trackIndex].type == VideoTrackType
                    // If video track index is not None.
                    && (!info->producer->get("video_index") || info->producer->get_int("video_index") != -1)) {

                // Get video filter.
                if (Settings.playerGPU())
                    filter.reset(getFilter("fadeInMovit", info->producer));
                else
                    filter.reset(getFilter("fadeInBrightness", info->producer));

                if (duration > 0) {
                    // Add video filter if needed.
                    if (!filter) {
                        if (Settings.playerGPU()) {
                            Mlt::Filter f(MLT.profile(), "movit.opacity");
                            f.set(kShotcutFilterProperty, "fadeInMovit");
                            f.set("alpha", i == bottomVideoTrackMltIndex() ? 1 : -1);
                            info->producer->attach(f);
                            filter.reset(new Mlt::Filter(f));
                        } else {
                            Mlt::Filter f(MLT.profile(), "brightness");
                            f.set(kShotcutFilterProperty, "fadeInBrightness");
                            if (i == bottomVideoTrackMltIndex()) {
                                f.set("alpha", 1);
                            } else {
                                f.set("alpha", 0);
                                f.set("level", 1);
                            }
                            info->producer->attach(f);
                            filter.reset(new Mlt::Filter(f));
                        }
                        moveBeforeFirstAudioFilter(info->producer);
                        filter->set_in_and_out(info->frame_in, info->frame_out);
                    }

                    // Adjust video filter.
                    if (Settings.playerGPU()) {
                        // Special handling for animation keyframes on movit.opacity.
                        filter->clear("opacity");
                        filter->anim_set("opacity", 0, 0, 0, mlt_keyframe_smooth_natural);
                        filter->anim_set("opacity", 1, duration - 1);
                    } else {
                        // Special handling for animation keyframes on brightness.
                        const char *key = filter->get_int("alpha") != 1 ? "alpha" : "level";
                        filter->clear(key);
                        filter->anim_set(key, 0, 0);
                        filter->anim_set(key, 1, duration - 1);
                    }
                    filter->set(kShotcutAnimInProperty, duration);
                    isChanged = true;
                } else if (filter) {
                    // Remove the video filter.
                    info->producer->detach(*filter);
                    filterAddedOrRemoved(info->producer);
                    filter->set(kShotcutAnimInProperty, duration);
                    isChanged = true;
                }
            }

            // If audio track index is not None.
            if (!info->producer->get("audio_index") || info->producer->get_int("audio_index") != -1) {
                // Get audio filter.
                filter.reset(getFilter("fadeInVolume", info->producer));

                if (duration > 0) {
                    // Add audio filter if needed.
                    if (!filter) {
                        Mlt::Filter f(MLT.profile(), "volume");
                        f.set(kShotcutFilterProperty, "fadeInVolume");
                        info->producer->attach(f);
                        filter.reset(new Mlt::Filter(f));
                        filter->set_in_and_out(info->frame_in, info->frame_out);
                    }

                    // Adjust audio filter.
                    filter->clear("level");
                    filter->anim_set("level", -60, 0);
                    filter->anim_set("level", 0, duration - 1);
                    filter->set(kShotcutAnimInProperty, duration);
                    isChanged = true;
                } else if (filter) {
                    // Remove the audio filter.
                    info->producer->detach(*filter);
                    filterAddedOrRemoved(info->producer);
                    filter->set(kShotcutAnimInProperty, duration);
                    isChanged = true;
                }
            }

            if (isChanged) {
                // Signal change.
                QModelIndex modelIndex = createIndex(clipIndex, 0, trackIndex);
                QVector<int> roles;
                roles << FadeInRole;
                emit dataChanged(modelIndex, modelIndex, roles);
                emit modified();
            }
        }
    }
}

void MultitrackModel::fadeOut(int trackIndex, int clipIndex, int duration)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        if (info && info->producer && info->producer->is_valid()) {
            QScopedPointer<Mlt::Filter> filter;
            duration = qBound(0, duration, info->frame_count);
            bool isChanged = false;

            if (m_trackList[trackIndex].type == VideoTrackType
                    // If video track index is not None.
                    && (!info->producer->get("video_index") || info->producer->get_int("video_index") != -1)) {

                // Get video filter.
                if (Settings.playerGPU())
                    filter.reset(getFilter("fadeOutMovit", info->producer));
                else
                    filter.reset(getFilter("fadeOutBrightness", info->producer));

                if (duration > 0) {
                    // Add video filter if needed.
                    if (!filter) {
                        if (Settings.playerGPU()) {
                            Mlt::Filter f(MLT.profile(), "movit.opacity");
                            f.set(kShotcutFilterProperty, "fadeOutMovit");
                            f.set("alpha", i == bottomVideoTrackMltIndex() ? 1 : -1);
                            info->producer->attach(f);
                            filter.reset(new Mlt::Filter(f));
                        } else {
                            Mlt::Filter f(MLT.profile(), "brightness");
                            f.set(kShotcutFilterProperty, "fadeOutBrightness");
                            if (i == bottomVideoTrackMltIndex()) {
                                f.set("alpha", 1);
                            } else {
                                f.set("alpha", 0);
                                f.set("level", 1);
                            }
                            info->producer->attach(f);
                            filter.reset(new Mlt::Filter(f));
                        }
                        moveBeforeFirstAudioFilter(info->producer);
                        filter->set_in_and_out(info->frame_in, info->frame_out);
                    }

                    // Adjust video filter.
                    if (Settings.playerGPU()) {
                        // Special handling for animation keyframes on movit.opacity.
                        filter->clear("opacity");
                        filter->anim_set("opacity", 1, info->frame_count - duration, 0, mlt_keyframe_smooth_natural);
                        filter->anim_set("opacity", 0, info->frame_count - 1);
                    } else {
                        // Special handling for animation keyframes on brightness.
                        const char *key = filter->get_int("alpha") != 1 ? "alpha" : "level";
                        filter->clear(key);
                        filter->anim_set(key, 1, info->frame_count - duration);
                        filter->anim_set(key, 0, info->frame_count - 1);
                    }
                    filter->set(kShotcutAnimOutProperty, duration);
                    isChanged = true;
                } else if (filter) {
                    // Remove the video filter.
                    info->producer->detach(*filter);
                    filterAddedOrRemoved(info->producer);
                    filter->set(kShotcutAnimOutProperty, duration);
                    isChanged = true;
                }
            }

            // If audio track index is not None.
            if (!info->producer->get("audio_index") || info->producer->get_int("audio_index") != -1) {
                // Get audio filter.
                filter.reset(getFilter("fadeOutVolume", info->producer));

                if (duration > 0) {
                    // Add audio filter if needed.
                    if (!filter) {
                        Mlt::Filter f(MLT.profile(), "volume");
                        f.set(kShotcutFilterProperty, "fadeOutVolume");
                        info->producer->attach(f);
                        filter.reset(new Mlt::Filter(f));
                        filter->set_in_and_out(info->frame_in, info->frame_out);
                    }

                    // Adjust audio filter.
                    filter->clear("level");
                    filter->anim_set("level", 0, info->frame_count - duration);
                    filter->anim_set("level", -60, info->frame_count - 1);
                    filter->set(kShotcutAnimOutProperty, duration);
                    isChanged = true;
                } else if (filter) {
                    // Remove the audio filter.
                    info->producer->detach(*filter);
                    filterAddedOrRemoved(info->producer);
                    filter->set(kShotcutAnimOutProperty, duration);
                    isChanged = true;
                }
            }

            if (isChanged) {
                // Signal change.
                QModelIndex modelIndex = createIndex(clipIndex, 0, trackIndex);
                QVector<int> roles;
                roles << FadeOutRole;
                emit dataChanged(modelIndex, modelIndex, roles);
                emit modified();
            }
        }
    }
}

bool MultitrackModel::addTransitionValid(int fromTrack, int toTrack, int clipIndex, int position,
                                         bool ripple)
{
    bool result = false;
    int i = m_trackList.at(toTrack).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (fromTrack == toTrack) {
            int targetIndex = playlist.get_clip_index_at(position);
            int previousIndex = clipIndex - 1 - (playlist.is_blank(clipIndex - 1) ? 1 : 0);
            int nextIndex = clipIndex + 1 + (playlist.is_blank(clipIndex + 1) ? 1 : 0);
            int endOfPreviousClip = playlist.clip_start(previousIndex) + playlist.clip_length(
                                        previousIndex) - 1;
            int endOfCurrentClip = position + playlist.clip_length(clipIndex) - 1;
            int startOfNextClip = playlist.clip_start(nextIndex);
            auto isBlankAtPosition = playlist.is_blank_at(position);
            auto isTransitionAtPreviousIndex = isTransition(playlist, previousIndex);
            auto isBlankAtEndOfCurrentClip = playlist.is_blank_at(endOfCurrentClip);
            auto isTransitionAtNextIndex = isTransition(playlist, nextIndex);

            if ((targetIndex < clipIndex && (endOfCurrentClip > endOfPreviousClip)
                    && (position > playlist.clip_start(previousIndex)) && !isBlankAtPosition
                    && !isTransitionAtPreviousIndex)
                    ||
                    (!ripple && (targetIndex >= clipIndex) && (position < startOfNextClip)
                     && !isBlankAtEndOfCurrentClip && !isTransitionAtNextIndex)) {
                result = true;
            }
        }
    }
    return result;
}

int MultitrackModel::addTransition(int trackIndex, int clipIndex, int position, bool ripple,
                                   bool rippleAllTracks)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        int targetIndex = playlist.get_clip_index_at(position);
        int previousIndex = clipIndex - 1 - (playlist.is_blank(clipIndex - 1) ? 1 : 0);
        int nextIndex = clipIndex + 1 + (playlist.is_blank(clipIndex + 1) ? 1 : 0);
        int endOfPreviousClip = playlist.clip_start(previousIndex) + playlist.clip_length(
                                    previousIndex) - 1;
        int endOfCurrentClip = position + playlist.clip_length(clipIndex) - 1;
        int startOfNextClip = playlist.clip_start(nextIndex);

        if ((targetIndex < clipIndex && endOfCurrentClip > endOfPreviousClip) || // dragged left
                (targetIndex >= clipIndex && position < startOfNextClip)) { // dragged right
            int duration = qAbs(position - playlist.clip_start(clipIndex));

            // Remove a blank duration from the transition duration.
            if (playlist.is_blank(clipIndex - 1) && targetIndex < clipIndex)
                duration -= playlist.clip_length(clipIndex - 1);
            else if (playlist.is_blank(clipIndex + 1) && targetIndex >= clipIndex)
                duration -= playlist.clip_length(clipIndex + 1);

            // Adjust/insert blanks
            moveClipInBlank(playlist, trackIndex, clipIndex, position, ripple, rippleAllTracks, duration);
            targetIndex = playlist.get_clip_index_at(position);

            // Create mix
            beginInsertRows(index(trackIndex), targetIndex + 1, targetIndex + 1);
            playlist.mix(targetIndex, duration);
            Mlt::Producer producer(playlist.get_clip(targetIndex + 1));
            if (producer.is_valid()) {
                producer.parent().set(kShotcutTransitionProperty, kShotcutDefaultTransition);
            }
            endInsertRows();

            // Add transitions
            Mlt::Transition dissolve(MLT.profile(), Settings.playerGPU() ? "movit.luma_mix" : "luma");
            Mlt::Transition crossFade(MLT.profile(), "mix:-1");
            if (!Settings.playerGPU()) {
                dissolve.set("alpha_over", 1);
                dissolve.set("fix_background_alpha", 1);
            }
            playlist.mix_add(targetIndex + 1, &dissolve);
            playlist.mix_add(targetIndex + 1, &crossFade);

            // Notify ins and outs changed
            QModelIndex modelIndex = createIndex(targetIndex, 0, trackIndex);
            QVector<int> roles;
            roles << StartRole;
            roles << OutPointRole;
            roles << DurationRole;
            emit dataChanged(modelIndex, modelIndex, roles);
            modelIndex = createIndex(targetIndex + 2, 0, trackIndex);
            roles.clear();
            roles << StartRole;
            roles << InPointRole;
            roles << DurationRole;
            roles << AudioLevelsRole;
            emit dataChanged(modelIndex, modelIndex, roles);
            emit modified();
            return targetIndex + 1;
        }
    }
    return -1;
}

void MultitrackModel::clearMixReferences(int trackIndex, int clipIndex)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::Producer> producer(playlist.get_clip(clipIndex - 1));
        if (producer && producer->is_valid()) {
            // Clear these since they are no longer valid.
            producer->set("mix_in", NULL, 0);
            producer->set("mix_out", NULL, 0);
            producer.reset(playlist.get_clip(clipIndex));
            if (producer && producer->is_valid()) {
                producer->parent().set("mlt_mix", NULL, 0);
                producer->set("mix_in", NULL, 0);
                producer->set("mix_out", NULL, 0);
            }
            producer.reset(playlist.get_clip(clipIndex + 1));
            if (producer && producer->is_valid()) {
                producer->set("mix_in", NULL, 0);
                producer->set("mix_out", NULL, 0);
            }
        }
    }
}

void MultitrackModel::removeTransition(int trackIndex, int clipIndex)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        clearMixReferences(trackIndex, clipIndex);
        emit removing(playlist.get_clip(clipIndex));
        beginRemoveRows(index(trackIndex), clipIndex, clipIndex);
        playlist.remove(clipIndex);
        endRemoveRows();
        --clipIndex;

        QModelIndex modelIndex = createIndex(clipIndex, 0, trackIndex);
        QVector<int> roles;
        roles << OutPointRole;
        roles << DurationRole;
        emit dataChanged(modelIndex, modelIndex, roles);
        modelIndex = createIndex(clipIndex + 1, 0, trackIndex);
        roles << InPointRole;
        roles << DurationRole;
        emit dataChanged(modelIndex, modelIndex, roles);
        emit modified();
    }
}

void MultitrackModel::removeTransitionByTrimIn(int trackIndex, int clipIndex, int delta)
{
    QModelIndex modelIndex = index(clipIndex, 0, index(trackIndex));
    clearMixReferences(trackIndex, clipIndex);
    int duration = -data(modelIndex, MultitrackModel::DurationRole).toInt();
    liftClip(trackIndex, clipIndex);
    trimClipOut(trackIndex, clipIndex - 1, duration, false, false);
    notifyClipOut(trackIndex, clipIndex - 1);
    if (delta) {
        trimClipIn(trackIndex, clipIndex, delta, false, false);
        notifyClipIn(trackIndex, clipIndex);
    }
}

void MultitrackModel::removeTransitionByTrimOut(int trackIndex, int clipIndex, int delta)
{
    QModelIndex modelIndex = index(clipIndex + 1, 0, index(trackIndex));
    clearMixReferences(trackIndex, clipIndex);
    int duration = -data(modelIndex, MultitrackModel::DurationRole).toInt();
    liftClip(trackIndex, clipIndex + 1);
    trimClipIn(trackIndex, clipIndex + 2, duration, false, false);
    notifyClipIn(trackIndex, clipIndex + 1);
    if (delta) {
        trimClipOut(trackIndex, clipIndex, delta, false, false);
        notifyClipOut(trackIndex, clipIndex);
    }
}

bool MultitrackModel::trimTransitionInValid(int trackIndex, int clipIndex, int delta)
{
    if (m_isMakingTransition) return false;
    bool result = false;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (clipIndex + 2 < playlist.count()) {
            Mlt::ClipInfo info;
            // Check if there is already a transition and its new length valid.
            if (isTransition(playlist, clipIndex + 1) && playlist.clip_length(clipIndex + 1) + delta > 0) {
                // Check clip A out point.
                playlist.clip_info(clipIndex, &info);
                info.frame_out -= delta;
                if (info.frame_out > info.frame_in && info.frame_out < info.length) {
                    // Check clip B in point.
                    playlist.clip_info(clipIndex + 2, &info);
                    info.frame_in -= playlist.clip_length(clipIndex + 1) + delta;
                    if (info.frame_in >= 0 && info.frame_in <= info.frame_out)
                        result = true;
                }
            }
        }
    }
    return result;
}

void MultitrackModel::trimTransitionIn(int trackIndex, int clipIndex, int delta)
{
//    LOG_DEBUG() << "clipIndex" << clipIndex << "delta" << delta;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);

        // Adjust the playlist "mix" entry.
        QScopedPointer<Mlt::Producer> producer(playlist.get_clip(clipIndex + 1));
        Mlt::Tractor tractor(producer->parent());
        if (!tractor.is_valid())
            return;
        QScopedPointer<Mlt::Producer> track_a(tractor.track(0));
        QScopedPointer<Mlt::Producer> track_b(tractor.track(1));
        int out = playlist.clip_length(clipIndex + 1) + delta - 1;
        playlist.block();
        track_a->set_in_and_out(track_a->get_in() - delta, track_a->get_out());
        track_b->set_in_and_out(track_b->get_in() - delta, track_b->get_out());
        playlist.unblock();
        tractor.multitrack()->set_in_and_out(0, out);
        tractor.set_in_and_out(0, out);
        producer->set("length", producer->frames_to_time(out + 1, mlt_time_clock));
        producer->set_in_and_out(0, out);

        // Adjust the transitions.
        QScopedPointer<Mlt::Service> service(tractor.producer());
        while (service && service->is_valid()) {
            if (service->type() == mlt_service_transition_type) {
                Mlt::Transition transition(*service);
                transition.set_in_and_out(0, out);
            }
            service.reset(service->producer());
        }

        // Adjust clip entry being trimmed.
        Mlt::ClipInfo info;
        playlist.clip_info(clipIndex, &info);
        playlist.resize_clip(clipIndex, info.frame_in, info.frame_out - delta);

        // Adjust filters.
        playlist.clip_info(clipIndex + 2, &info);
        MLT.adjustClipFilters(*info.producer, info.frame_in, info.frame_out, -(out + 1), 0, -delta);

        QVector<int> roles;
        roles << OutPointRole;
        roles << DurationRole;
        emit dataChanged(createIndex(clipIndex, 0, trackIndex),
                         createIndex(clipIndex + 1, 0, trackIndex), roles);
        emit modified();
    }
}

bool MultitrackModel::trimTransitionOutValid(int trackIndex, int clipIndex, int delta)
{
    if (m_isMakingTransition) return false;
    bool result = false;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (clipIndex > 1) {
            Mlt::ClipInfo info;
            // Check if there is already a transition.
            if (isTransition(playlist, clipIndex - 1)) {
                // Check clip A out point.
                playlist.clip_info(clipIndex - 2, &info);
                info.frame_out += playlist.clip_length(clipIndex - 1) + delta;
                if (info.frame_out > info.frame_in && info.frame_out < info.length) {
                    // Check clip B in point.
                    playlist.clip_info(clipIndex, &info);
                    info.frame_in += delta;
                    if (info.frame_in >= 0 && info.frame_in <= info.frame_out)
                        result = true;
                }
            }
        }
    }
    return result;
}

void MultitrackModel::trimTransitionOut(int trackIndex, int clipIndex, int delta)
{
//    LOG_DEBUG() << "clipIndex" << clipIndex << "delta" << delta;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);

        // Adjust the playlist "mix" entry.
        QScopedPointer<Mlt::Producer> producer(playlist.get_clip(clipIndex - 1));
        Mlt::Tractor tractor(producer->parent());
        if (!tractor.is_valid())
            return;
        QScopedPointer<Mlt::Producer> track_a(tractor.track(0));
        QScopedPointer<Mlt::Producer> track_b(tractor.track(1));
        int out = playlist.clip_length(clipIndex - 1) + delta - 1;
        playlist.block();
        track_a->set_in_and_out(track_a->get_in(), track_a->get_out() + delta);
        track_b->set_in_and_out(track_b->get_in(), track_b->get_out() + delta);
        playlist.unblock();
        tractor.multitrack()->set_in_and_out(0, out);
        tractor.set_in_and_out(0, out);
        producer->set("length", producer->frames_to_time(out + 1, mlt_time_clock));
        producer->set_in_and_out(0, out);

        // Adjust the transitions.
        QScopedPointer<Mlt::Service> service(tractor.producer());
        while (service && service->is_valid()) {
            if (service->type() == mlt_service_transition_type) {
                Mlt::Transition transition(*service);
                transition.set_in_and_out(0, out);
            }
            service.reset(service->producer());
        }

        // Adjust clip entry being trimmed.
        Mlt::ClipInfo info;
        playlist.clip_info(clipIndex, &info);
        playlist.resize_clip(clipIndex, info.frame_in + delta, info.frame_out);

        // Adjust filters.
        playlist.clip_info(clipIndex - 2, &info);
        MLT.adjustClipFilters(*info.producer, info.frame_in, info.frame_out, 0, -(out + 1), 0);

        QVector<int> roles;
        roles << OutPointRole;
        roles << DurationRole;
        emit dataChanged(createIndex(clipIndex - 1, 0, trackIndex),
                         createIndex(clipIndex - 1, 0, trackIndex), roles);
        roles.clear();
        roles << InPointRole;
        roles << DurationRole;
        emit dataChanged(createIndex(clipIndex, 0, trackIndex),
                         createIndex(clipIndex, 0, trackIndex), roles);
        emit modified();
    }
}

bool MultitrackModel::addTransitionByTrimInValid(int trackIndex, int clipIndex, int delta)
{
    Q_UNUSED(delta)
    bool result = false;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (clipIndex > 0) {
            // Check if preceding clip is not blank, not already a transition,
            // and there is enough frames before in point of current clip.
            if (!m_isMakingTransition && delta < 0 && !playlist.is_blank(clipIndex - 1)
                    && !isTransition(playlist, clipIndex - 1)) {
                Mlt::ClipInfo info;
                playlist.clip_info(clipIndex, &info);
                if (info.frame_in >= -delta)
                    result = true;
            } else if (m_isMakingTransition && isTransition(playlist, clipIndex - 1)) {
                // Invalid if transition length will be 0 or less.
                auto newTransitionDuration = playlist.clip_length(clipIndex - 1) - delta;
                result = newTransitionDuration > 0;
                if (result && clipIndex > 1) {
                    QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
                    // Invalid if left clip length will be 0 or less.
                    result = playlist.clip_length(clipIndex - 2) + delta > 0 &&
                             // Invalid if current clip in point will be less than 0.
                             info && info->frame_in - newTransitionDuration >= 0;
                }
            } else {
                result = m_isMakingTransition;
            }
        }
    }
    return result;
}

int MultitrackModel::addTransitionByTrimIn(int trackIndex, int clipIndex, int delta)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);

        // Create transition if it does not yet exist.
        if (!isTransition(playlist, clipIndex - 1)) {
            // Adjust filters.
            Mlt::ClipInfo info;
            playlist.clip_info(clipIndex, &info);
            MLT.adjustClipFilters(*info.producer, info.frame_in, info.frame_out, delta, 0, 0);

            // Insert the mix clip.
            beginInsertRows(index(trackIndex), clipIndex, clipIndex);
            playlist.mix_out(clipIndex - 1, -delta);
            QScopedPointer<Mlt::Producer> producer(playlist.get_clip(clipIndex));
            producer->parent().set(kShotcutTransitionProperty, kShotcutDefaultTransition);
            endInsertRows();

            // Add transitions.
            Mlt::Transition dissolve(MLT.profile(), Settings.playerGPU() ? "movit.luma_mix" : "luma");
            Mlt::Transition crossFade(MLT.profile(), "mix:-1");
            if (!Settings.playerGPU()) {
                dissolve.set("alpha_over", 1);
                dissolve.set("fix_background_alpha", 1);
            }
            playlist.mix_add(clipIndex, &dissolve);
            playlist.mix_add(clipIndex, &crossFade);

            // Notify clip A changed.
            QModelIndex modelIndex = createIndex(clipIndex - 1, 0, trackIndex);
            QVector<int> roles;
            roles << OutPointRole;
            roles << DurationRole;
            emit dataChanged(modelIndex, modelIndex, roles);
            emit modified();
            m_isMakingTransition = true;
            clipIndex += 1;
        } else if (m_isMakingTransition) {
            // Adjust a transition addition already in progress.
            // m_isMakingTransition will be set false when mouse button released via notifyClipOut().
            trimTransitionIn(trackIndex, clipIndex - 2, -delta);
        }
    }
    return clipIndex;
}

bool MultitrackModel::addTransitionByTrimOutValid(int trackIndex, int clipIndex, int delta)
{
    Q_UNUSED(delta)
    bool result = false;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (clipIndex + 1 < playlist.count()) {
            // Check if following clip is not blank, not already a transition,
            // and there is enough frames after out point of current clip.
            if (!m_isMakingTransition && delta < 0 && !playlist.is_blank(clipIndex + 1)
                    && !isTransition(playlist, clipIndex +  1)) {
                Mlt::ClipInfo info;
                playlist.clip_info(clipIndex, &info);
//                LOG_DEBUG() << "(info.length" << info.length << " - info.frame_out" << info.frame_out << ") =" << (info.length - info.frame_out) << " >= -delta" << -delta;
                if ((info.length - info.frame_out) >= -delta)
                    result = true;
            } else if (m_isMakingTransition && isTransition(playlist, clipIndex + 1)) {
                // Invalid if transition length will be 0 or less.
                auto newTransitionDuration = playlist.clip_length(clipIndex + 1) - delta;
//                LOG_DEBUG() << "playlist.clip_length(clipIndex + 1)" << playlist.clip_length(clipIndex + 1) << "- delta" << delta << "=" << (playlist.clip_length(clipIndex + 1) - delta);
                result = newTransitionDuration > 0;
                if (result && clipIndex + 2 < playlist.count()) {
                    QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
                    // Invalid if right clip length will be 0 or less.
                    result = playlist.clip_length(clipIndex + 2) + delta > 0 &&
                             // Invalid if current clip out point would exceed its duration.
                             info && info->frame_out + newTransitionDuration < info->length;
                }
            } else {
                result = m_isMakingTransition;
            }
        }
    }
    return result;
}

void MultitrackModel::addTransitionByTrimOut(int trackIndex, int clipIndex, int delta)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);

        // Create transition if it does not yet exist.
        if (!isTransition(playlist, clipIndex + 1)) {
            // Adjust filters.
            Mlt::ClipInfo info;
            playlist.clip_info(clipIndex, &info);
            MLT.adjustClipFilters(*info.producer, info.frame_in, info.frame_out, 0, delta, 0);

            // Insert the mix clip.
            beginInsertRows(index(trackIndex), clipIndex + 1, clipIndex + 1);
            playlist.mix_in(clipIndex, -delta);
            QScopedPointer<Mlt::Producer> producer(playlist.get_clip(clipIndex + 1));
            producer->parent().set(kShotcutTransitionProperty, kShotcutDefaultTransition);
            endInsertRows();

            // Add transitions.
            Mlt::Transition dissolve(MLT.profile(), Settings.playerGPU() ? "movit.luma_mix" : "luma");
            Mlt::Transition crossFade(MLT.profile(), "mix:-1");
            if (!Settings.playerGPU()) {
                dissolve.set("alpha_over", 1);
                dissolve.set("fix_background_alpha", 1);
            }
            playlist.mix_add(clipIndex + 1, &dissolve);
            playlist.mix_add(clipIndex + 1, &crossFade);

            // Notify clip B changed.
            QModelIndex modelIndex = createIndex(clipIndex + 2, 0, trackIndex);
            QVector<int> roles;
            roles << InPointRole;
            roles << DurationRole;
            emit dataChanged(modelIndex, modelIndex, roles);
            emit modified();
            m_isMakingTransition = true;
        } else if (m_isMakingTransition) {
            // Adjust a transition addition already in progress.
            // m_isMakingTransition will be set false when mouse button released via notifyClipIn().
            delta = playlist.clip_start(clipIndex + 1) - (playlist.clip_start(clipIndex) + playlist.clip_length(
                                                              clipIndex) + delta);
            trimTransitionOut(trackIndex, clipIndex + 2, delta);
        }
    }
}

bool MultitrackModel::removeTransitionByTrimInValid(int trackIndex, int clipIndex, int delta)
{
    bool result = false;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (clipIndex > 1) {
            // Check if there is a transition and its new length is 0 or less.
            if (isTransition(playlist, clipIndex - 1)
                    && playlist.clip_length(clipIndex - 1) - qAbs(delta) <= 0
                    && ((delta < 0 && !m_isMakingTransition) || (delta > 0 && m_isMakingTransition))) {
                result = true;
                m_isMakingTransition = false;
            }
        }
    }
    return result;
}

bool MultitrackModel::removeTransitionByTrimOutValid(int trackIndex, int clipIndex, int delta)
{
    bool result = false;
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (clipIndex + 2 < playlist.count()) {
            // Check if there is a transition and its new length is 0 or less.
//            LOG_DEBUG() << "transition length" << playlist.clip_length(clipIndex + 1) << "delta" << delta << playlist.clip_length(clipIndex + 1) - qAbs(delta);
            if (isTransition(playlist, clipIndex + 1)
                    && playlist.clip_length(clipIndex + 1) - qAbs(delta) <= 0
                    && ((delta < 0 && !m_isMakingTransition) || (delta > 0 && m_isMakingTransition))) {
                result = true;
                m_isMakingTransition = false;
            }
        }
    }
    return result;
}

void MultitrackModel::filterAddedOrRemoved(Mlt::Producer *producer)
{
    if (!m_tractor || !producer || !producer->is_valid())
        return;
    mlt_service service = producer->get_service();

    // Check if it was on the multitrack tractor.
    if (service == m_tractor->get_service())
        emit filteredChanged();
    else if (producer->get(kMultitrackItemProperty)) {
        // Check if it was a clip.
        QString s = QString::fromLatin1(producer->get(kMultitrackItemProperty));
        auto parts = s.split(':');
        if (parts.length() == 2) {
            QModelIndex modelIndex = createIndex(parts[0].toInt(), 0, parts[1].toInt());
            QVector<int> roles;
            roles << FadeInRole;
            roles << FadeOutRole;
            emit dataChanged(modelIndex, modelIndex, roles);
        }
    } else for (int i = 0; i < m_trackList.size(); i++) {
            // Check if it was on one of the tracks.
            QScopedPointer<Mlt::Producer> track(m_tractor->track(m_trackList[i].mlt_index));
            if (service == track.data()->get_service()) {
                QModelIndex modelIndex = index(i, 0);
                QVector<int> roles;
                roles << IsFilteredRole;
                emit dataChanged(modelIndex, modelIndex, roles);
                break;
            }
        }
}

void MultitrackModel::onFilterChanged(Mlt::Service *filter)
{
    if (filter && filter->is_valid()) {
        Mlt::Service service(mlt_service(filter->get_data("service")));
        if (service.is_valid() && service.get(kMultitrackItemProperty)) {
            QString s = QString::fromLatin1(service.get(kMultitrackItemProperty));
            auto parts = s.split(':');
            if (parts.length() == 2) {
                QModelIndex modelIndex = createIndex(parts[0].toInt(), 0, parts[1].toInt());
                QVector<int> roles;
                const char *name = filter->get(kShotcutFilterProperty);
                if (!qstrcmp("fadeInMovit", name) ||
                        !qstrcmp("fadeInBrightness", name) ||
                        !qstrcmp("fadeInVolume", name))
                    roles << FadeInRole;
                if (!qstrcmp("fadeOutMovit", name) ||
                        !qstrcmp("fadeOutBrightness", name) ||
                        !qstrcmp("fadeOutVolume", name))
                    roles << FadeOutRole;
                if (roles.length())
                    emit dataChanged(modelIndex, modelIndex, roles);
            }
        }
    }
}

void MultitrackModel::moveClipToEnd(Mlt::Playlist &playlist, int trackIndex, int clipIndex,
                                    int position, bool ripple, bool rippleAllTracks)
{
    int n = playlist.count();
    int length = position - playlist.clip_start(n - 1) - playlist.clip_length(n - 1);
    int clipPlaytime = playlist.clip_length(clipIndex);
    int clipStart = playlist.clip_start(clipIndex);

    if (!ripple) {
        if (clipIndex > 0 && playlist.is_blank(clipIndex - 1)) {
            // If there was a blank on the left adjust it.
            int duration = playlist.clip_length(clipIndex - 1) + playlist.clip_length(clipIndex);
//            LOG_DEBUG() << "adjust blank on left to" << duration;
            playlist.resize_clip(clipIndex - 1, 0, duration - 1);

            QModelIndex index = createIndex(clipIndex - 1, 0, trackIndex);
            QVector<int> roles;
            roles << DurationRole;
            emit dataChanged(index, index, roles);
        } else if ((clipIndex + 1) < n && playlist.is_blank(clipIndex + 1)) {
            // If there was a blank on the right adjust it.
            int duration = playlist.clip_length(clipIndex + 1) + playlist.clip_length(clipIndex);
//            LOG_DEBUG() << "adjust blank on right to" << duration;
            playlist.resize_clip(clipIndex + 1, 0, duration - 1);

            QModelIndex index = createIndex(clipIndex + 1, 0, trackIndex);
            QVector<int> roles;
            roles << DurationRole;
            emit dataChanged(index, index, roles);
        } else {
            // Add new blank
            beginInsertRows(index(trackIndex), clipIndex, clipIndex);
            playlist.insert_blank(clipIndex, playlist.clip_length(clipIndex) - 1);
            endInsertRows();
            ++clipIndex;
            ++n;
        }
    }
    // Add blank to end if needed.
    if (length > 0) {
        beginInsertRows(index(trackIndex), n, n);
        playlist.blank(length - 1);
        endInsertRows();
    }
    // Finally, move clip into place.
    QModelIndex parentIndex = index(trackIndex);
    if (playlist.count() < clipIndex || playlist.count() > clipIndex + 1) {
        beginMoveRows(parentIndex, clipIndex, clipIndex, parentIndex, playlist.count());
        playlist.move(clipIndex, playlist.count());
        endMoveRows();
        consolidateBlanks(playlist, trackIndex);
    }

    // Ripple all unlocked tracks.
    if (clipPlaytime > 0 && ripple && rippleAllTracks)
        for (int j = 0; j < m_trackList.count(); ++j) {
            if (j == trackIndex)
                continue;

            int mltIndex = m_trackList.at(j).mlt_index;
            QScopedPointer<Mlt::Producer> otherTrack(m_tractor->track(mltIndex));
            if (otherTrack) {
                if (otherTrack->get_int(kTrackLockProperty))
                    continue;

                removeRegion(j, clipStart, clipPlaytime);
            }
        }
}

void MultitrackModel::moveClipInBlank(Mlt::Playlist &playlist, int trackIndex, int clipIndex,
                                      int position, bool ripple, bool rippleAllTracks, int duration)
{
    int clipPlaytime = duration ? duration : playlist.clip_length(clipIndex);
    int clipStart = playlist.clip_start(clipIndex);
    int delta = position - clipStart;

    if (clipIndex > 0 && playlist.is_blank(clipIndex - 1)) {
        // Adjust blank on left.
        int duration = playlist.clip_length(clipIndex - 1) + delta;
        if (duration > 0) {
//            LOG_DEBUG() << "adjust blank on left" << (clipIndex - 1) << "to" << duration;
            playlist.resize_clip(clipIndex - 1, 0, duration - 1);

            QModelIndex index = createIndex(clipIndex - 1, 0, trackIndex);
            QVector<int> roles;
            roles << DurationRole;
            emit dataChanged(index, index, roles);
        } else {
//            LOG_DEBUG() << "remove blank on left";
            int i = clipIndex - 1;
            beginRemoveRows(index(trackIndex), i, i);
            playlist.remove(i);
            endRemoveRows();
            consolidateBlanks(playlist, trackIndex);
            --clipIndex;
        }
    } else if (delta > 0) {
//        LOG_DEBUG() << "add blank on left with duration" << delta;
        // Add blank to left.
        int i = qMax(clipIndex, 0);
        beginInsertRows(index(trackIndex), i, i);
        playlist.insert_blank(i, delta - 1);
        endInsertRows();
        ++clipIndex;
    }

    if (!ripple && (clipIndex + 1) < playlist.count() && playlist.is_blank(clipIndex + 1)) {
        // Adjust blank to right.
        int duration = playlist.clip_length(clipIndex + 1) - delta;
        if (duration > 0) {
//            LOG_DEBUG() << "adjust blank on right" << (clipIndex + 1) << "to" << duration;
            playlist.resize_clip(clipIndex + 1, 0, duration - 1);

            QModelIndex index = createIndex(clipIndex + 1, 0, trackIndex);
            QVector<int> roles;
            roles << DurationRole;
            emit dataChanged(index, index, roles);
        } else {
//            LOG_DEBUG() << "remove blank on right";
            int i = clipIndex + 1;
            beginRemoveRows(index(trackIndex), i, i);
            playlist.remove(i);
            endRemoveRows();
            consolidateBlanks(playlist, trackIndex);
        }
    } else if (!ripple && delta < 0 && (clipIndex + 1) < playlist.count()) {
        // Add blank to right.
//        LOG_DEBUG() << "add blank on right with duration" << -delta;
        beginInsertRows(index(trackIndex), clipIndex + 1, clipIndex + 1);
        playlist.insert_blank(clipIndex + 1, (-delta - 1));
        endInsertRows();
    }

    // Ripple all unlocked tracks.
    if (clipPlaytime > 0 && ripple && rippleAllTracks) {
        QList<int> otherTracksToRipple;
        for (int i = 0; i < m_trackList.count(); ++i) {
            if (i == trackIndex)
                continue;
            int mltIndex = m_trackList.at(i).mlt_index;
            QScopedPointer<Mlt::Producer> otherTrack(m_tractor->track(mltIndex));
            if (otherTrack && otherTrack->get_int(kTrackLockProperty))
                continue;
            otherTracksToRipple << i;
        }
        if (position < clipStart) {
            foreach (int idx, otherTracksToRipple)
                removeRegion(idx, position, clipStart - position);
        } else {
            insertOrAdjustBlankAt(otherTracksToRipple, clipStart, position - clipStart);
            consolidateBlanks(playlist, trackIndex);
        }
    }
}

void MultitrackModel::consolidateBlanks(Mlt::Playlist &playlist, int trackIndex)
{
    for (int i = 1; i < playlist.count(); i++) {
        if (playlist.is_blank(i - 1) && playlist.is_blank(i)) {
            int out = playlist.clip_length(i - 1) + playlist.clip_length(i) - 1;
            playlist.resize_clip(i - 1, 0, out);
            QModelIndex idx = createIndex(i - 1, 0, trackIndex);
            QVector<int> roles;
            roles << DurationRole;
            emit dataChanged(idx, idx, roles);
            beginRemoveRows(index(trackIndex), i, i);
            playlist.remove(i--);
            endRemoveRows();
        }
    }
    if (playlist.count() > 0) {
        int i = playlist.count() - 1;
        if (playlist.is_blank(i)) {
            beginRemoveRows(index(trackIndex), i, i);
            playlist.remove(i);
            endRemoveRows();
        }
    }
    if (playlist.count() == 0) {
        beginInsertRows(index(trackIndex), 0, 0);
        playlist.blank(0);
        endInsertRows();
    }
}

void MultitrackModel::consolidateBlanksAllTracks()
{
    if (!m_tractor) return;
    int i = 0;
    foreach (Track t, m_trackList) {
        Mlt::Producer *track = m_tractor->track(t.mlt_index);
        if (track) {
            Mlt::Playlist playlist(*track);
            consolidateBlanks(playlist, i);
        }
        ++i;
    }
}

void MultitrackModel::audioLevelsReady(const QModelIndex &index)
{
    QVector<int> roles;
    roles << AudioLevelsRole;
    emit dataChanged(index, index, roles);
}

bool MultitrackModel::createIfNeeded()
{
    if (!m_tractor) {
        m_tractor = new Mlt::Tractor(MLT.profile());
        MLT.profile().set_explicit(true);
        m_tractor->set(kShotcutXmlProperty, 1);
        retainPlaylist();
        addBackgroundTrack();
        addVideoTrack();
        emit created();
    } else if (!m_trackList.count()) {
        addVideoTrack();
    }
    return true;
}

void MultitrackModel::addBackgroundTrack()
{
    Mlt::Playlist playlist(MLT.profile());
    playlist.set("id", kBackgroundTrackId);
    Mlt::Producer producer(MLT.profile(), "color:0");
    producer.set("mlt_image_format", "rgba");
    producer.set("length", 1);
    producer.set("id", "black");
    // Allow mixing against frames produced by this producer.
    producer.set("set.test_audio", 0);
    playlist.append(producer);
    m_tractor->set_track(playlist, m_tractor->count());
}

void MultitrackModel::adjustBackgroundDuration()
{
    if (!m_tractor) return;
    int duration = getDuration();
    Mlt::Producer *track = m_tractor->track(0);
    if (track) {
        Mlt::Playlist playlist(*track);
        Mlt::Producer *clip = playlist.get_clip(0);
        if (clip) {
            if (duration != clip->parent().get_length()) {
                clip->parent().set("length", clip->parent().frames_to_time(duration, mlt_time_clock));
                clip->parent().set_in_and_out(0, duration - 1);
                clip->set("length", clip->parent().frames_to_time(duration, mlt_time_clock));
                clip->set_in_and_out(0, duration - 1);
                playlist.resize_clip(0, 0, duration - 1);
                emit durationChanged();
            }
            delete clip;
        }
        delete track;
    }
}

void MultitrackModel::adjustServiceFilterDurations(Mlt::Service &service, int duration)
{
    // Use kFilterOutProperty to track duration changes
    if (service.get(kFilterOutProperty)) {
        int oldOut = service.get_int(kFilterOutProperty);
        int n = service.filter_count();
        for (int i = 0; i < n; i++) {
            QScopedPointer<Mlt::Filter> filter(service.filter(i));
            if (filter && filter->is_valid() && !filter->get_int("_loader")) {
                int in = filter->get_in();
                int out = filter->get_out();
                // Only change out if it is pinned (same as old track duration)
                if (out == oldOut || in < 0) {
                    out = duration - 1;
                    filter->set_in_and_out(qMax(in, 0), out);
                }
            }
        }
    }
    service.set(kFilterOutProperty, duration - 1);
}

bool MultitrackModel::warnIfInvalid(Mlt::Service &service)
{
    if (!service.is_valid()) {
        const char *plugin = Settings.playerGPU() ? "Movit overlay" : "frei0r cairoblend";
        const char *plugins = Settings.playerGPU() ? "Movit" : "frei0r";
        LongUiTask::cancel();
        QMessageBox::critical(&MAIN, qApp->applicationName(),
                              tr("Error: Shotcut could not find the %1 plugin on your system.\n\n"
                                 "Please install the %2 plugins.").arg(plugin).arg(plugins));
        return true;
    }
    return false;
}

Mlt::Transition *MultitrackModel::getVideoBlendTransition(int trackIndex) const
{
    auto transition = getTransition("frei0r.cairoblend", trackIndex);
    if (!transition)
        transition = getTransition("movit.overlay", trackIndex);
    if (!transition)
        transition = getTransition("qtblend", trackIndex);
    return transition;
}

int MultitrackModel::bottomVideoTrackMltIndex() const
{
    return mltIndexForTrack(bottomVideoTrackIndex());
}

void MultitrackModel::refreshVideoBlendTransitions()
{
    int a_track = bottomVideoTrackMltIndex();
    // For each video track
    for (auto &t : m_trackList) {
        if (t.type == VideoTrackType) {
            QScopedPointer<Mlt::Transition> transition(getVideoBlendTransition(t.mlt_index));
            if (transition && transition->is_valid()) {
                // Normalize its video blending transition
                if (transition->get_a_track() != 0) {
                    transition->set("a_track", a_track);
                }
                if (t.number) {
                    transition->clear("disable");
                } else {
                    transition->set("disable", 1);
                }
            }
        }
    }
}

int MultitrackModel::bottomVideoTrackIndex() const
{
    int track = -1;
    for (int i = 0; i < m_trackList.size(); ++i) {
        if (m_trackList[i].type == VideoTrackType) {
            track = i;
        }
    }
    return track;
}

int MultitrackModel::mltIndexForTrack(int trackIndex) const
{
    int i = -1;
    if (trackIndex >= 0 && trackIndex < m_trackList.size()) {
        i = m_trackList[trackIndex].mlt_index;
    }
    return i;
}

void MultitrackModel::adjustTrackFilters()
{
    if (!m_tractor) return;
    int duration = getDuration();

    // Adjust filters on the tractor.
    adjustServiceFilterDurations(*m_tractor, duration);

    // Adjust filters on the tracks.
    foreach (Track t, m_trackList) {
        QScopedPointer<Mlt::Producer> track(m_tractor->track(t.mlt_index));
        if (track && track->is_valid())
            adjustServiceFilterDurations(*track, duration);
    }
}

std::unique_ptr<Mlt::ClipInfo> MultitrackModel::findClipByUuid(const QUuid &uuid, int &trackIndex,
                                                               int &clipIndex)
{
    for (trackIndex = 0; trackIndex < trackList().size(); trackIndex++) {
        int i = trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            for (clipIndex = 0; clipIndex < playlist.count(); clipIndex++) {
                Mlt::ClipInfo *info;
                if ((info = playlist.clip_info(clipIndex))) {
                    if (MLT.uuid(*info->cut) == uuid)
                        return std::unique_ptr<Mlt::ClipInfo>(info);
                }
            }
        }
    }
    return nullptr;
}

std::unique_ptr<Mlt::ClipInfo> MultitrackModel::getClipInfo(int trackIndex, int clipIndex)
{
    Mlt::ClipInfo *result = nullptr;
    if (clipIndex >= 0 && trackIndex >= 0 && trackIndex < trackList().size()) {
        int i = trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            result = playlist.clip_info(clipIndex);
        }
    }
    return std::unique_ptr<Mlt::ClipInfo>(result);
}

QString MultitrackModel::getTrackName(int trackIndex)
{
    QString name;
    if (trackIndex < m_trackList.size()) {
        QScopedPointer<Mlt::Producer> track(m_tractor->track(m_trackList.at(trackIndex).mlt_index));
        if (track)
            name = track->get(kTrackNameProperty);
    }
    return name;
}

int MultitrackModel::addAudioTrack()
{
    if (!m_tractor) {
        m_tractor = new Mlt::Tractor(MLT.profile());
        MLT.profile().set_explicit(true);
        m_tractor->set(kShotcutXmlProperty, 1);
        retainPlaylist();
        addBackgroundTrack();
        addAudioTrack();
        emit created();
        emit modified();
        return 0;
    }

    // Get the new track index.
    int i = m_tractor->count();

    // Create the MLT track.
    Mlt::Playlist playlist(MLT.profile());
    playlist.set(kAudioTrackProperty, 1);
    playlist.set("hide", 1);
    playlist.blank(0);
    m_tractor->set_track(playlist, i);
    MLT.updateAvformatCaching(m_tractor->count());

    // Add the mix transition.
    Mlt::Transition mix(MLT.profile(), "mix");
    mix.set("always_active", 1);
    mix.set("sum", 1);
    m_tractor->plant_transition(mix, 0, i);

    // Get the new, logical audio-only index.
    int a = 0;
    foreach (Track t, m_trackList) {
        if (t.type == AudioTrackType)
            ++a;
    }

    // Add the shotcut logical audio track.
    Track t;
    t.mlt_index = i;
    t.type = AudioTrackType;
    t.number = a++;
    QString trackName = QString("A%1").arg(a);
    playlist.set(kTrackNameProperty, trackName.toUtf8().constData());
    beginInsertRows(QModelIndex(), m_trackList.count(), m_trackList.count());
    m_trackList.append(t);
    endInsertRows();
    emit modified();
    return m_trackList.count() - 1;
}

int MultitrackModel::addVideoTrack()
{
    if (!m_tractor) {
        createIfNeeded();
        return 0;
    }

    // Get the new track index.
    int i = m_tractor->count();

    // Create the MLT track.
    Mlt::Playlist playlist(MLT.profile());
    playlist.set(kVideoTrackProperty, 1);
    playlist.blank(0);
    m_tractor->set_track(playlist, i);
    MLT.updateAvformatCaching(m_tractor->count());

    // Add the mix transition.
    Mlt::Transition mix(MLT.profile(), "mix");
    mix.set("always_active", 1);
    mix.set("sum", 1);
    m_tractor->plant_transition(mix, 0, i);

    // Add the composite transition.
    Mlt::Transition composite(MLT.profile(),
                              Settings.playerGPU() ? "movit.overlay" : "frei0r.cairoblend");
    if (!composite.is_valid() && !Settings.playerGPU()) {
        composite = Mlt::Transition(MLT.profile(), "qtblend");
    } else if (composite.is_valid() && !Settings.playerGPU()) {
        composite.set("threads", 0);
    }
    if (warnIfInvalid(composite)) {
        return -1;
    }
    composite.set("disable", 1);
    foreach (Track t, m_trackList) {
        if (t.type == VideoTrackType) {
            composite.set("disable", 0);
            break;
        }
    }

    // Get the new, logical video-only index.
    int v = 0;
    int last_mlt_index = 0;
    foreach (Track t, m_trackList) {
        if (t.type == VideoTrackType) {
            ++v;
            last_mlt_index = t.mlt_index;
        }
    }
    m_tractor->plant_transition(composite, last_mlt_index, i);

    // Add the shotcut logical video track.
    Track t;
    t.mlt_index = i;
    t.type = VideoTrackType;
    t.number = v++;
    QString trackName = QString("V%1").arg(v);
    playlist.set(kTrackNameProperty, trackName.toUtf8().constData());
    beginInsertRows(QModelIndex(), 0, 0);
    m_trackList.prepend(t);
    endInsertRows();
    emit modified();
    return 0;
}

void MultitrackModel::removeTrack(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < m_trackList.size()) {
        const Track &track = m_trackList.value(trackIndex);
        QScopedPointer<Mlt::Field> field(m_tractor->field());
        QScopedPointer<Mlt::Transition> transition(getVideoBlendTransition(track.mlt_index));

        // Get the playlist, loop over its clips, and emit removing
        QScopedPointer<Mlt::Producer> producer(m_tractor->track(track.mlt_index));
        if (producer) {
            Mlt::Playlist playlist(*producer);
            for (int i = 0; i < playlist.count(); ++i) {
                if (!playlist.is_blank(i))
                    emit removing(playlist.get_clip(i));
            }
        }

        // Remove transitions.
        if (transition && transition->is_valid())
            field->disconnect_service(*transition);
        transition.reset(getTransition("mix", track.mlt_index));
        if (transition && transition->is_valid())
            field->disconnect_service(*transition);

//        foreach (Track t, m_trackList) LOG_DEBUG() << (t.type == VideoTrackType?"Video":"Audio") << "track number" << t.number << "mlt_index" << t.mlt_index;
//        LOG_DEBUG() << trackIndex << "mlt_index" << track.mlt_index;

        // Remove track.
        beginRemoveRows(QModelIndex(), trackIndex, trackIndex);
        m_tractor->remove_track(track.mlt_index);
        m_trackList.removeAt(trackIndex);

//        foreach (Track t, m_trackList) LOG_DEBUG() << (t.type == VideoTrackType?"Video":"Audio") << "track number" << t.number << "mlt_index" << t.mlt_index;

        // Renumber other tracks.
        int row = 0;
        foreach (Track t, m_trackList) {
            if (t.mlt_index > track.mlt_index)
                --m_trackList[row].mlt_index;
            if (t.type == track.type && t.number > track.number) {
                --m_trackList[row].number;
                QModelIndex modelIndex = index(row, 0);

                // Disable compositing on the bottom video track.
                if (m_trackList[row].number == 0 && t.type == VideoTrackType) {
                    QScopedPointer<Mlt::Transition> transition(getVideoBlendTransition(1));
                    if (transition && transition->is_valid())
                        transition->set("disable", 1);
                    emit dataChanged(modelIndex, modelIndex, QVector<int>() << IsBottomVideoRole << IsCompositeRole);
                }

                // Rename default track names.
                QScopedPointer<Mlt::Producer> mltTrack(m_tractor->track(m_trackList[row].mlt_index));
                QString trackNameTemplate = (t.type == VideoTrackType) ? QString("V%1") : QString("A%1");
                QString trackName = trackNameTemplate.arg(t.number + 1);
                if (mltTrack && mltTrack->get(kTrackNameProperty) == trackName) {
                    trackName = trackNameTemplate.arg(m_trackList[row].number + 1);
                    mltTrack->set(kTrackNameProperty, trackName.toUtf8().constData());
                    emit dataChanged(modelIndex, modelIndex, QVector<int>() << NameRole);
                }
            }
            ++row;
        }
        endRemoveRows();
        MLT.updateAvformatCaching(m_tractor->count());
//        foreach (Track t, m_trackList) LOG_DEBUG() << (t.type == VideoTrackType?"Video":"Audio") << "track number" << t.number << "mlt_index" << t.mlt_index;
    }
    emit modified();
}

void MultitrackModel::retainPlaylist()
{
    if (!MAIN.playlist())
        MAIN.playlistDock()->model()->createIfNeeded();
    Mlt::Playlist playlist(*MAIN.playlist());
    playlist.set("id", kPlaylistTrackId);
    QString retain = QString("xml_retain %1").arg(kPlaylistTrackId);
    m_tractor->set(retain.toUtf8().constData(), playlist.get_service(), 0);
}

void MultitrackModel::loadPlaylist()
{
    Mlt::Properties retainList((mlt_properties) m_tractor->get_data("xml_retain"));
    if (retainList.is_valid()) {
        Mlt::Playlist playlist((mlt_playlist) retainList.get_data(kPlaylistTrackId));
        if (playlist.is_valid() && playlist.type() == mlt_service_playlist_type) {
            MAIN.playlistDock()->model()->setPlaylist(playlist);
        } else {
            Mlt::Playlist legacyPlaylist((mlt_playlist) retainList.get_data(kLegacyPlaylistTrackId));
            if (legacyPlaylist.is_valid() && legacyPlaylist.type() == mlt_service_playlist_type)
                MAIN.playlistDock()->model()->setPlaylist(legacyPlaylist);
        }
    }
    retainPlaylist();
}

void MultitrackModel::removeRegion(int trackIndex, int position, int length)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        int clipIndex = playlist.get_clip_index_at(position);

        if (clipIndex >= 0 && clipIndex < playlist.count()) {
            int clipStart = playlist.clip_start(clipIndex);
            int playtime = playlist.get_playtime();
            playlist.block(playlist.get_playlist());

            if (position + length > playtime)
                length -= (position + length - playtime);

            if (clipStart < position) {
                splitClip(trackIndex, clipIndex, position);
                ++clipIndex;
            }

            while (length > 0) {
                if (playlist.clip_length(clipIndex) > length) {
                    splitClip(trackIndex, clipIndex, position + length);
                }
                length -= playlist.clip_length(clipIndex);
                if (clipIndex < playlist.count()) {
                    // Shotcut does not like the behavior of remove() on a
                    // transition (MLT mix clip). So, we null mlt_mix to prevent it.
                    clearMixReferences(trackIndex, clipIndex);
                    emit removing(playlist.get_clip(clipIndex));
                    beginRemoveRows(index(trackIndex), clipIndex, clipIndex);
                    playlist.remove(clipIndex);
                    endRemoveRows();
                }
            }
            playlist.unblock(playlist.get_playlist());
            consolidateBlanks(playlist, trackIndex);
        }
    }
}

bool MultitrackModel::isTransition(Mlt::Playlist &playlist, int clipIndex) const
{
    QScopedPointer<Mlt::Producer> producer(playlist.get_clip(clipIndex));
    if (producer && producer->parent().get(kShotcutTransitionProperty))
        return true;
    return false;
}

void MultitrackModel::insertTrack(int trackIndex, TrackType type)
{
    if (!m_tractor || trackIndex <= 0) {
        addVideoTrack();
        return;
    }

    // Get the new track index.
    Track &currentTrack = m_trackList[qBound(0, trackIndex, m_trackList.count() - 1)];
    int currentTrackNumber = currentTrack.number;
    int new_mlt_index = currentTrack.mlt_index;
    QScopedPointer<Mlt::Transition> lowerVideoTransition;
    const char *videoTransitionName = Settings.playerGPU() ? "movit.overlay" : "frei0r.cairoblend";
    bool isInsertBottomVideoTrack = false;

    if (type == VideoTrackType) {
        ++new_mlt_index;
        lowerVideoTransition.reset(getVideoBlendTransition(currentTrack.mlt_index));

        // Handle special case of insert bottom video track.
        if ((trackIndex > 0 && currentTrack.type == AudioTrackType) || trackIndex >= m_trackList.count()) {
            Track &upperTrack = m_trackList[qBound(0, trackIndex - 1, m_trackList.count() - 1)];
            if (upperTrack.type == VideoTrackType) {
                new_mlt_index = 1;
                lowerVideoTransition.reset(getVideoBlendTransition(upperTrack.mlt_index));
                isInsertBottomVideoTrack = true;
            }
        }
    }

    // When requesting a  new top track.
    if (trackIndex >= m_trackList.count()) {
        if (type == AudioTrackType) {
            addAudioTrack();
            return;
        } else if (type == VideoTrackType) {
            new_mlt_index = currentTrack.mlt_index;
        }
    }

//    foreach (Track t, m_trackList) LOG_DEBUG() << (t.type == VideoTrackType?"Video":"Audio") << "track number" << t.number << "mlt_index" << t.mlt_index;
//    LOG_DEBUG() << "trackIndex" << trackIndex << "mlt_index" << i;

    // Compute new track numbers.
    int videoTrackCount = 0;
    int last_mlt_index = 0;
    int row = 0;
    foreach (Track t, m_trackList) {
        if (t.type == type) {
            if ((t.type == VideoTrackType && (t.number > currentTrackNumber || isInsertBottomVideoTrack)) ||
                    (t.type == AudioTrackType && t.number >= currentTrackNumber)) {
                // Rename default track names.
                QScopedPointer<Mlt::Producer> mltTrack(m_tractor->track(t.mlt_index));
                QString trackNameTemplate = (t.type == VideoTrackType) ? QString("V%1") : QString("A%1");
                QString trackName = trackNameTemplate.arg(++t.number);
                if (mltTrack && mltTrack->get(kTrackNameProperty) == trackName) {
                    trackName = trackNameTemplate.arg(t.number + 1);
                    mltTrack->set(kTrackNameProperty, trackName.toUtf8().constData());
                    QModelIndex modelIndex = index(row, 0);
                    emit dataChanged(modelIndex, modelIndex, QVector<int>() << NameRole);
                }
                ++m_trackList[row].number;
            }
        }
        // Increment the mlt_index for tracks that are moving.
        if (t.mlt_index >= new_mlt_index)
            ++m_trackList[row].mlt_index;
        // Count the number of video tracks and get the mlt_index of the last video track.
        if (t.type == VideoTrackType) {
            ++videoTrackCount;
            last_mlt_index = t.mlt_index;
        }
        ++row;
    }

//    foreach (Track t, m_trackList) LOG_DEBUG() << (t.type == VideoTrackType?"Video":"Audio") << "track number" << t.number << "mlt_index" << t.mlt_index;

    // Create the MLT track.
    Mlt::Playlist playlist(MLT.profile());
    if (type == VideoTrackType) {
        playlist.set(kVideoTrackProperty, 1);
    } else if (type == AudioTrackType) {
        playlist.set(kAudioTrackProperty, 1);
        playlist.set("hide", 1);
    }
    playlist.blank(0);
    m_tractor->insert_track(playlist, new_mlt_index);
    MLT.updateAvformatCaching(m_tractor->count());

    // Add the mix transition.
    Mlt::Transition mix(MLT.profile(), "mix");
    mix.set("always_active", 1);
    mix.set("sum", 1);
    m_tractor->plant_transition(mix, 0, new_mlt_index);

    if (type == VideoTrackType) {
        // Add the composite transition.
        Mlt::Transition composite(MLT.profile(), videoTransitionName);
        if (!composite.is_valid() && !Settings.playerGPU()) {
            composite = Mlt::Transition(MLT.profile(), "qtblend");
        } else if (composite.is_valid() && !Settings.playerGPU()) {
            composite.set("threads", 0);
        }
        if (warnIfInvalid(composite)) {
            return;
        }
        if (lowerVideoTransition && lowerVideoTransition->is_valid()) {
            QScopedPointer<Mlt::Service> consumer(lowerVideoTransition->consumer());
            if (consumer->is_valid()) {
                // Insert the new transition.
                if (new_mlt_index == 1) {
                    // Special case of insert new bottom video track
                    LOG_DEBUG() << "inserting transition" << 0 << new_mlt_index;
                    QScopedPointer<Mlt::Service> lowerProducer(lowerVideoTransition->producer());
                    if (lowerProducer->is_valid()) {
                        composite.connect(*lowerProducer, 0, new_mlt_index);
                        Mlt::Transition t((mlt_transition) lowerProducer->get_service());
                        lowerVideoTransition->connect(composite, new_mlt_index, t.get_int("b_track"));
                    }
                } else {
                    LOG_DEBUG() << "inserting transition" << last_mlt_index << new_mlt_index;
                    composite.connect(*lowerVideoTransition, last_mlt_index, new_mlt_index);
                    Mlt::Transition t((mlt_transition) consumer->get_service());
                    t.connect(composite, t.get_int("a_track"), t.get_int("b_track"));
                }
            } else {
                // Append the new transition.
                LOG_DEBUG() << "appending transition";
                m_tractor->plant_transition(composite, last_mlt_index, new_mlt_index);
            }
        } else {
            // Append the new transition.
            LOG_DEBUG() << "appending transition";
            m_tractor->plant_transition(composite, last_mlt_index, new_mlt_index);
        }
    }

    // Add the shotcut logical video track.
    Track t;
    t.mlt_index = new_mlt_index;
    t.type = type;
    t.number = 0;
    QString trackName;
    if (t.type == VideoTrackType) {
        t.number = videoTrackCount - trackIndex;
        trackName = QString("V%1");
    } else if (t.type == AudioTrackType) {
        t.number = trackIndex - videoTrackCount;
        trackName = QString("A%1");
    }
    trackName = trackName.arg(t.number + 1);
    playlist.set(kTrackNameProperty, trackName.toUtf8().constData());
    beginInsertRows(QModelIndex(), trackIndex, trackIndex);
    m_trackList.insert(trackIndex, t);
    refreshVideoBlendTransitions();
    endInsertRows();
    emit modified();
//    foreach (Track t, m_trackList) LOG_DEBUG() << (t.type == VideoTrackType?"Video":"Audio") << "track number" << t.number << "mlt_index" << t.mlt_index;
}

void MultitrackModel::moveTrack(int fromTrackIndex, int toTrackIndex)
{
    LOG_DEBUG() << "From: " << fromTrackIndex << "To: " << toTrackIndex;
    MLT.pause();
    if (fromTrackIndex >= m_trackList.count() || fromTrackIndex < 0
            || toTrackIndex >= m_trackList.count() || toTrackIndex < 0) {
        LOG_DEBUG() << "Invalid track index" << fromTrackIndex << toTrackIndex;
        return;
    }
    if (fromTrackIndex == toTrackIndex) {
        LOG_DEBUG() << "Do not move track to itself" << fromTrackIndex;
        return;
    }

    int fromMltIndex = m_trackList.at(fromTrackIndex).mlt_index;
    int toMltIndex = m_trackList.at(toTrackIndex).mlt_index;

    // Take a copy of the track
    QScopedPointer<Mlt::Producer> producer(m_tractor->multitrack()->track(fromMltIndex));

    // Take a copy of all the transitions
    // Save the transitions in order of their new track index
    QMap<int, Mlt::Transition> videoTransitions;
    QMap<int, Mlt::Transition> audioTransitions;
    QScopedPointer<Mlt::Service> service(m_tractor->producer());
    while (service && service->is_valid()) {
        if (service->type() == mlt_service_transition_type) {
            Mlt::Transition t((mlt_transition) service->get_service());
            int newBTrack = t.get_b_track();
            if (newBTrack == fromMltIndex) {
                newBTrack = toMltIndex;
            } else {
                if (newBTrack > fromMltIndex) {
                    newBTrack--;
                }
                if (newBTrack >= toMltIndex) {
                    newBTrack++;
                }
            }
            if (service->get("mlt_service") != QString("mix") ) {
                videoTransitions[newBTrack] = *service;
            } else {
                audioTransitions[newBTrack] = *service;
            }
        }
        service.reset(service->producer());
    }


    if (toTrackIndex > fromTrackIndex) {
        beginMoveRows(QModelIndex(), fromTrackIndex, fromTrackIndex, QModelIndex(), toTrackIndex + 1);
    } else {
        beginMoveRows(QModelIndex(), fromTrackIndex, fromTrackIndex, QModelIndex(), toTrackIndex);
    }

    // Clear all default track names (will regenerate in refreshTrackList)
    foreach (const Track &t, m_trackList) {
        QScopedPointer<Mlt::Producer> mltTrack(m_tractor->track(t.mlt_index));
        QString trackNameTemplate = (t.type == VideoTrackType) ? QString("V%1") : QString("A%1");
        QString trackName = trackNameTemplate.arg(t.number + 1);
        if (mltTrack && mltTrack->get(kTrackNameProperty) == trackName) {
            mltTrack->Mlt::Properties::clear(kTrackNameProperty);
        }
    }

    // Remove all the transitions
    // (remove_track() and insert_track() would mess them up)
    service.reset(m_tractor->producer());
    QScopedPointer<Mlt::Field> field(m_tractor->field());
    while (service && service->is_valid()) {
        Mlt::Service s(service->get_service());
        service.reset(service->producer());
        if (s.type() == mlt_service_transition_type) {
            field->disconnect_service(s);
            s.disconnect_all_producers();
        }
    }

    // Remove the track and add it in the new position
    m_tractor->remove_track(fromMltIndex);
    m_tractor->insert_track(*producer, toMltIndex);

    // Add the transitions back in the new positions
    for (auto i = audioTransitions.keyBegin(); i != audioTransitions.keyEnd(); i++) {
        int bTrack = *i;
        int aTrack = 0;
        if (audioTransitions[bTrack].is_valid()) {
            Mlt::Transition aTransition (MLT.profile(), audioTransitions[bTrack].get("mlt_service"));
            aTransition.inherit(audioTransitions[bTrack]);
            m_tractor->plant_transition(aTransition, aTrack, bTrack);
        }
        if (videoTransitions[bTrack].is_valid()) {
            Mlt::Transition vTransition (MLT.profile(), videoTransitions[bTrack].get("mlt_service"));
            vTransition.set("1", videoTransitions[bTrack].get("1"));
            if (bTrack == 1) {
                vTransition.set("disable", 1);
            } else {
                // video transitions mix with track 1
                // (except track 1 which mixes with track 0)
                aTrack = 1;
                vTransition.set("disable", 0);
                QString mode = vTransition.get("1");
                if (mode.isEmpty()) {
                    vTransition.set("1", "normal");
                }
            }
            m_tractor->plant_transition(vTransition, aTrack, bTrack);
        }
    }

    m_trackList.clear();
    refreshTrackList();

    endMoveRows();
    emit dataChanged(index(0), index(m_trackList.size() - 1),
                     QVector<int>() << IsTopVideoRole << IsBottomVideoRole << IsTopAudioRole << IsBottomAudioRole <<
                     IsCompositeRole << NameRole);
    emit modified();
}

void MultitrackModel::insertOrAdjustBlankAt(QList<int> tracks, int position, int length)
{
    foreach (int trackIndex, tracks) {
        int mltIndex = m_trackList.at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> otherTrack(m_tractor->track(mltIndex));

        if (otherTrack) {
            Mlt::Playlist trackPlaylist(*otherTrack);
            // Check if frame before position is blank
            int idx = trackPlaylist.get_clip_index_at(position - 1);
            if (trackPlaylist.is_blank(idx)) {
                trackPlaylist.resize_clip(idx, 0, trackPlaylist.clip_length(idx) + length - 1);
                QModelIndex modelIndex = createIndex(idx, 0, trackIndex);
                emit dataChanged(modelIndex, modelIndex, QVector<int>() << DurationRole);
                continue;
            }

            // Check if frame as position is blank
            idx = trackPlaylist.get_clip_index_at(position);
            if (trackPlaylist.is_blank(idx)) {
                trackPlaylist.resize_clip(idx, 0, trackPlaylist.clip_length(idx) + length - 1);
                QModelIndex modelIndex = createIndex(idx, 0, trackIndex);
                emit dataChanged(modelIndex, modelIndex, QVector<int>() << DurationRole);
            } else if (length > 0) {
                int insertBlankAtIdx = idx;
                if (trackPlaylist.clip_start(idx) < position) {
                    splitClip(trackIndex, idx, position);
                    insertBlankAtIdx = idx + 1;
                }
                beginInsertRows(index(trackIndex), insertBlankAtIdx, insertBlankAtIdx);
                trackPlaylist.insert_blank(insertBlankAtIdx, length - 1);
                endInsertRows();
            } else {
                Q_ASSERT(!"unsupported");
            }
        }
    }
}

bool MultitrackModel::mergeClipWithNext(int trackIndex, int clipIndex, bool dryrun)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (!track)
        return false;
    Mlt::Playlist playlist(*track);
    if (clipIndex >= playlist.count() + 1)
        return false;
    Mlt::ClipInfo clip1;
    Mlt::ClipInfo clip2;
    playlist.clip_info(clipIndex, &clip1);
    playlist.clip_info(clipIndex + 1, &clip2);

    if (QString::fromUtf8(clip1.resource) != QString::fromUtf8(clip2.resource))
        return false;

    if (clip1.frame_out + 1 != clip2.frame_in)
        return false;

    if (dryrun)
        return true;

    // Consolidate filters
    QStringList filters {"fadeInVolume", "fadeOutVolume", "fadeInBrightness", "fadeOutBrightness", "fadeInMovit", "fadeOutMovit"};
    for (const auto &s : filters) {
        QScopedPointer<Mlt::Filter> filter(getFilter(s, clip1.producer));
        if (filter && filter->is_valid()) {
            filter.reset(getFilter(s, clip2.producer));
            if (filter && filter->is_valid()) {
                clip2.producer->detach(*filter);
            }
        }
    }
    Mlt::Controller::copyFilters(*clip2.producer, *clip1.producer);
    QModelIndex modelIndex = createIndex(clipIndex, 0, trackIndex);
    QVector<int> roles;
    roles << FadeInRole;
    roles << FadeOutRole;
    emit dataChanged(modelIndex, modelIndex, roles);

    liftClip(trackIndex, clipIndex + 1);
    trimClipOut(trackIndex, clipIndex, -clip2.frame_count, false, false);

    emit modified();
    return true;
}

bool MultitrackModel::isFiltered(Mlt::Producer *producer) const
{
    if (!producer)
        producer = m_tractor;
    if (producer && producer->is_valid()) {
        int count = producer->filter_count();
        for (int i = 0; i < count; i++) {
            QScopedPointer<Mlt::Filter> filter(producer->filter(i));
            if (filter && filter->is_valid() && !filter->get_int("_loader"))
                return true;
        }
    }
    return false;
}

int MultitrackModel::getDuration()
{
    int n = 0;
    if (m_tractor) {
        foreach (Track t, m_trackList) {
            QScopedPointer<Mlt::Producer> track(m_tractor->track(t.mlt_index));
            if (track && track->is_valid())
                n = qMax(n, track->get_length());
        }
    }
    return n;
}

void MultitrackModel::load()
{
    if (m_tractor) {
        beginResetModel();
        delete m_tractor;
        m_tractor = 0;
        m_trackList.clear();
        endResetModel();
    }
    // In some versions of MLT, the resource property is the XML filename,
    // but the Mlt::Tractor(Service&) constructor will fail unless it detects
    // the type as tractor, and mlt_service_identify() needs the resource
    // property to say "<tractor>" to identify it as playlist type.
    MLT.producer()->set("mlt_type", "mlt_producer");
    MLT.producer()->set("resource", "<tractor>");
    MLT.profile().set_explicit(true);
    m_tractor = new Mlt::Tractor(*MLT.producer());
    if (!m_tractor->is_valid()) {
        delete m_tractor;
        m_tractor = 0;
        return;
    }

    loadPlaylist();
    addBlackTrackIfNeeded();
    MLT.updateAvformatCaching(m_tractor->count());
    refreshTrackList();
    convertOldDoc();
    consolidateBlanksAllTracks();
    adjustBackgroundDuration();
    adjustTrackFilters();
    if (m_trackList.count() > 0) {
        beginInsertRows(QModelIndex(), 0, m_trackList.count() - 1);
        endInsertRows();
        getAudioLevels();
    }
    emit loaded();
    emit filteredChanged();
    emit scaleFactorChanged();
}

void MultitrackModel::reload(bool asynchronous)
{
    if (m_tractor) {
        if (asynchronous) {
            emit reloadRequested();
        } else {
            beginResetModel();
            endResetModel();
            getAudioLevels();
            emit filteredChanged();
        }
    }
}

void MultitrackModel::replace(int trackIndex, int clipIndex, Mlt::Producer &clip, bool copyFilters)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    Mlt::Producer track(m_tractor->track(i));
    if (track.is_valid()) {
//        LOG_DEBUG() << __FUNCTION__ << "replace" << position << MLT.XML(&clip);
        Mlt::Playlist playlist(track);
        int in = clip.get_in();
        int out = clip.get_out();
        Mlt::Producer oldClip(playlist.get_clip(clipIndex));
        Q_ASSERT(oldClip.is_valid());
        int clipPlaytime = oldClip.get_playtime();
        int transitionIn = oldClip.parent().get(kFilterInProperty) ? oldClip.get_in() -
                           oldClip.parent().get_int(kFilterInProperty) : 0;
        int transitionOut = oldClip.parent().get(kFilterOutProperty) ? oldClip.parent().get_int(
                                kFilterOutProperty) - oldClip.get_out() : 0;

        in += transitionIn;
        out -= transitionOut;
        if (clip.get_in() > 0 || clip.get_out() == clip.get_playtime() - 1)
            out = in + clipPlaytime - 1;
        else
            in = out - clipPlaytime + 1;
        clip.set_in_and_out(in, out);
        if (copyFilters) {
            auto parent = oldClip.parent();
            parent.set(kFilterInProperty, oldClip.get_in());
            parent.set(kFilterOutProperty, oldClip.get_out());
            Mlt::Controller::copyFilters(parent, clip);
            Mlt::Controller::adjustFilters(clip);
        }
        emit removing(playlist.get_clip(clipIndex));
        beginRemoveRows(index(trackIndex), clipIndex, clipIndex);
        playlist.remove(clipIndex);
        endRemoveRows();
        beginInsertRows(index(trackIndex), clipIndex, clipIndex);
        playlist.insert_blank(clipIndex, clipPlaytime - 1);
        endInsertRows();
        overwrite(trackIndex, clip, playlist.clip_start(clipIndex), false);

        // Handle transition on the left
        if (transitionIn && isTransition(playlist, clipIndex - 1)) {
            Mlt::Producer producer(playlist.get_clip(clipIndex - 1));
            if (producer.is_valid()) {
                Mlt::Tractor tractor(MLT_TRACTOR(producer.get_parent()));
                Q_ASSERT(tractor.is_valid());
                Mlt::Producer track(tractor.track(1));
                if (!qstrcmp(track.parent().get(kShotcutHashProperty),
                             oldClip.parent().get(kShotcutHashProperty))) {
                    Mlt::Producer cut(clip.cut(in - transitionIn, in - 1));
                    tractor.set_track(cut, 1);
                }
            }
        }
        // Handle transition on the right
        if (transitionOut && isTransition(playlist, clipIndex + 1)) {
            Mlt::Producer producer(playlist.get_clip(clipIndex + 1));
            if (producer.is_valid()) {
                Mlt::Tractor tractor(MLT_TRACTOR(producer.get_parent()));
                Q_ASSERT(tractor.is_valid());
                Mlt::Producer track(tractor.track(0));
                if (!qstrcmp(track.parent().get(kShotcutHashProperty),
                             oldClip.parent().get(kShotcutHashProperty))) {
                    Mlt::Producer cut(clip.cut(out + 1, out + transitionOut));
                    tractor.set_track(cut, 0);
                }
            }
        }
    }
}

void MultitrackModel::close()
{
    if (!m_tractor) return;
    if (m_trackList.count() > 0) {
        beginRemoveRows(QModelIndex(), 0, m_trackList.count() - 1);
        m_trackList.clear();
        endRemoveRows();
    }
    delete m_tractor;
    m_tractor = 0;
    emit closed();
}

int MultitrackModel::clipIndex(int trackIndex, int position)
{
    int i = m_trackList.at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
    if (track) {
        Mlt::Playlist playlist(*track);
        return playlist.get_clip_index_at(position);
    }
    return -1; // error
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
        QString trackId = track->get("id");
        if (trackId == "black_track")
            isKdenlive = true;
        else if (trackId == kBackgroundTrackId)
            continue;
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

                // Always disable compositing on V1.
                if (v == 1) {
                    QScopedPointer<Mlt::Transition> transition(getVideoBlendTransition(1));
                    if (transition && transition->is_valid())
                        transition->set("disable", 1);
                }
            }
        }
    }

    // Add audio tracks.
    for (int i = 0; i < n; ++i) {
        QScopedPointer<Mlt::Producer> track(m_tractor->track(i));
        if (!track)
            continue;
        QString trackId = track->get("id");
        if (trackId == "black_track")
            isKdenlive = true;
        else if (isKdenlive && trackId == "playlist1")
            // In Kdenlive, playlist1 is a special audio mixdown track.
            continue;
        else if (trackId == kPlaylistTrackId || trackId == kLegacyPlaylistTrackId)
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
//                LOG_DEBUG() << __FUNCTION__ << QString(track->get("id")) << i;
            }
        }
    }
}

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
                AudioLevelsTask::start(clip->parent(), this, index);
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
        if (clip && QString(clip->get("id")) == "black")
            found = true;
    }
    if (!found) {
        // Move all existing tracks down by 1.
        for (int i = n; i > 0; i++) {
            Mlt::Producer *producer = m_tractor->track(n - 1);
            if (producer)
                m_tractor->set_track(*producer, n);
            delete producer;
        }
        Mlt::Producer producer(MLT.profile(), "color:0");
        producer.set("mlt_image_format", "rgba");
        m_tractor->set_track(producer, 0);
    }
}

void MultitrackModel::convertOldDoc()
{
    QScopedPointer<Mlt::Field> field(m_tractor->field());

    // Convert composite to frei0r.cairoblend.
    int n = m_tractor->count();
    for (int i = 1; i < n; ++i) {
        QScopedPointer<Mlt::Transition> transition(getTransition("composite", i));
        if (transition) {
            Mlt::Transition composite(MLT.profile(), "frei0r.cairoblend");
            composite.set("disable", transition->get_int("disable"));
            field->disconnect_service(*transition);
            m_tractor->plant_transition(composite, transition->get_int("a_track"), i);
        }
    }

    // Remove movit.rect filters.
    QScopedPointer<Mlt::Service> service(m_tractor->producer());
    while (service && service->is_valid()) {
        if (service->type() == mlt_service_filter_type) {
            Mlt::Filter f((mlt_filter) service->get_service());
            if (QString::fromLatin1(f.get("mlt_service")) == "movit.rect") {
                field->disconnect_service(f);
            }
        }
        service.reset(service->producer());
    }

    // Change a_track of composite transitions to bottom video track.
    int a_track = bottomVideoTrackMltIndex();
    foreach (Track t, m_trackList) {
        if (t.type == VideoTrackType) {
            QScopedPointer<Mlt::Transition> transition(getVideoBlendTransition(t.mlt_index));
            if (transition && transition->is_valid() && transition->get_a_track() != 0)
                transition->set("a_track", a_track);
        }
    }

    // Ensure the background track clears the test_audio flag on frames.
    QScopedPointer<Mlt::Producer> track(m_tractor->track(0));
    if (track) {
        Mlt::Playlist playlist(*track);
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(0));
        if (info && info->producer->is_valid() && QString(info->producer->get("id")) == "black")
            info->producer->set("set.test_audio", 0);
    }
}

Mlt::Transition *MultitrackModel::getTransition(const QString &name, int trackIndex) const
{
    QScopedPointer<Mlt::Service> service(m_tractor->producer());
    while (service && service->is_valid()) {
        if (service->type() == mlt_service_transition_type) {
            Mlt::Transition t((mlt_transition) service->get_service());
            if (name == t.get("mlt_service") && t.get_b_track() == trackIndex)
                return new Mlt::Transition(t);
        }
        service.reset(service->producer());
    }
    return nullptr;
}

Mlt::Filter *MultitrackModel::getFilter(const QString &name, int trackIndex) const
{
    QScopedPointer<Mlt::Service> service(m_tractor->producer());
    while (service && service->is_valid()) {
        if (service->type() == mlt_service_filter_type) {
            Mlt::Filter f((mlt_filter) service->get_service());
            if (name == f.get("mlt_service") && f.get_track() == trackIndex)
                return new Mlt::Filter(f);
        }
        service.reset(service->producer());
    }
    return 0;
}

Mlt::Filter *MultitrackModel::getFilter(const QString &name, Mlt::Service *service) const
{
    return MLT.getFilter(name, service);
}

void MultitrackModel::removeBlankPlaceholder(Mlt::Playlist &playlist, int trackIndex)
{
    if (playlist.count() == 1 && playlist.is_blank(0)) {
        beginRemoveRows(index(trackIndex), 0, 0);
        playlist.remove(0);
        endRemoveRows();
    }
}
