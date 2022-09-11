/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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

#include "dialogs/longuitask.h"
#include "timelinecommands.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"
#include "settings.h"
#include "proxymanager.h"
#include "dialogs/longuitask.h"
#include "controllers/filtercontroller.h"
#include "qmltypes/qmlmetadata.h"
#include "util.h"
#include <Logger.h>


#include <QMetaObject>

namespace Timeline {

Mlt::Producer *deserializeProducer(QString &xml)
{
    return new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData());
}

AppendCommand::AppendCommand(MultitrackModel &model, int trackIndex, const QString &xml,
                             bool skipProxy, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_xml(xml)
    , m_undoHelper(m_model)
    , m_skipProxy(skipProxy)
{
    setText(QObject::tr("Append to track"));
}

void AppendCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex;
    LongUiTask longTask(QObject::tr("Append to Timeline"));
    m_undoHelper.recordBeforeState();
    Mlt::Producer *producer = longTask.runAsync<Mlt::Producer *>(QObject::tr("Preparing"),
                                                                 deserializeProducer, m_xml);
    if (producer->type() == mlt_service_playlist_type) {
        Mlt::Playlist playlist(*producer);
        int count = playlist.count();
        for (int i = 0; i < count; i++) {
            longTask.reportProgress(QObject::tr("Appending"), i, count);
            QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(i));
            Mlt::Producer clip = Mlt::Producer(info->producer);
            if (!m_skipProxy) ProxyManager::generateIfNotExists(clip);
            clip.set_in_and_out(info->frame_in, info->frame_out);
            m_model.appendClip(m_trackIndex, clip);
        }
    } else {
        if (!m_skipProxy) ProxyManager::generateIfNotExists(*producer);
        m_model.appendClip(m_trackIndex, *producer);
    }
    longTask.reportProgress(QObject::tr("Finishing"), 0, 0);
    delete producer;
    m_undoHelper.recordAfterState();
}

void AppendCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex;
    m_undoHelper.undoChanges();
}

InsertCommand::InsertCommand(MultitrackModel &model, MarkersModel &markersModel, int trackIndex,
                             int position, const QString &xml, bool seek, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_markersModel(markersModel)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(m_model)
    , m_seek(seek)
    , m_rippleAllTracks(Settings.timelineRippleAllTracks())
    , m_rippleMarkers(Settings.timelineRippleMarkers())
    , m_markersShift(0)
{
    setText(QObject::tr("Insert into track"));
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
}

void InsertCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "position" << m_position;
    int shift = 0;
    m_undoHelper.recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    if (clip.type() == mlt_service_playlist_type) {
        LongUiTask longTask(QObject::tr("Add Files"));
        Mlt::Playlist playlist(clip);
        int n = playlist.count();
        int i = n;
        while (i--) {
            QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(i));
            clip = Mlt::Producer(info->producer);
            longTask.reportProgress(QFileInfo(ProxyManager::resource(clip)).fileName(), n - i - 1, n);
            ProxyManager::generateIfNotExists(clip);
            clip.set_in_and_out(info->frame_in, info->frame_out);
            bool lastClip = i == 0;
            m_model.insertClip(m_trackIndex, clip, m_position, m_rippleAllTracks, false, lastClip);
            shift += info->frame_count;
        }
    } else {
        shift = clip.get_playtime();
        ProxyManager::generateIfNotExists(clip);
        m_model.insertClip(m_trackIndex, clip, m_position, m_rippleAllTracks, m_seek);
    }
    m_undoHelper.recordAfterState();
    if (m_rippleMarkers && shift > 0) {
        m_markersShift = shift;
        m_markersModel.doShift(m_position, m_markersShift);
    }
}

void InsertCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "position" << m_position;
    m_undoHelper.undoChanges();
    if (m_rippleMarkers && m_markersShift > 0) {
        m_markersModel.doShift(m_position + m_markersShift, -m_markersShift);
    }
}

OverwriteCommand::OverwriteCommand(MultitrackModel &model, int trackIndex,
                                   int position, const QString &xml, bool seek, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(m_model)
    , m_seek(seek)
{
    setText(QObject::tr("Overwrite onto track"));
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
}

void OverwriteCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "position" << m_position;
    m_undoHelper.recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    if (clip.type() == mlt_service_playlist_type) {
        LongUiTask longTask(QObject::tr("Add Files"));
        Mlt::Playlist playlist(clip);
        int position = m_position;
        int n = playlist.count();
        for (int i = 0; i < n; i++) {
            QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(i));
            clip = Mlt::Producer(info->producer);
            longTask.reportProgress(QFileInfo(ProxyManager::resource(clip)).fileName(), i, n);
            ProxyManager::generateIfNotExists(clip);
            clip.set_in_and_out(info->frame_in, info->frame_out);
            bool lastClip = i == (n - 1);
            m_model.overwrite(m_trackIndex, clip, position, false, lastClip);
            position += info->frame_count;
        }
    } else {
        ProxyManager::generateIfNotExists(clip);
        m_model.overwrite(m_trackIndex, clip, m_position, m_seek);
    }
    m_undoHelper.recordAfterState();
}

void OverwriteCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "position" << m_position;
    m_undoHelper.undoChanges();
}

LiftCommand::LiftCommand(MultitrackModel &model, int trackIndex,
                         int clipIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Lift from track"));
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
}

void LiftCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    m_undoHelper.recordBeforeState();
    m_model.liftClip(m_trackIndex, m_clipIndex);
    m_undoHelper.recordAfterState();
}

void LiftCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    m_undoHelper.undoChanges();
}

RemoveCommand::RemoveCommand(MultitrackModel &model, MarkersModel &markersModel, int trackIndex,
                             int clipIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_markersModel(markersModel)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_undoHelper(m_model)
    , m_rippleAllTracks(Settings.timelineRippleAllTracks())
    , m_rippleMarkers(Settings.timelineRippleMarkers())
    , m_markerRemoveStart(-1)
    , m_markerRemoveEnd(-1)
{
    setText(QObject::tr("Remove from track"));
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
}

void RemoveCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;

    if (m_rippleMarkers) {
        // Remove and shift markers as appropriate
        bool markersModified = false;
        m_markers = m_markersModel.getMarkers();
        if (m_markers.size() > 0) {
            auto mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(mlt_index));
            if (track && track->is_valid()) {
                Mlt::Playlist playlist(*track);
                m_markerRemoveStart = playlist.clip_start(m_clipIndex);
                m_markerRemoveEnd = m_markerRemoveStart + playlist.clip_length(m_clipIndex);
            }
        }
        if (m_markers.size() > 0 && m_markerRemoveStart >= 0) {
            QList<Markers::Marker> newMarkers = m_markers;
            for (int i = 0; i < newMarkers.size(); i++) {
                Markers::Marker &marker = newMarkers[i];
                if (marker.start >= m_markerRemoveStart &&
                        marker.start <= m_markerRemoveEnd) {
                    // This marker is in the removed segment. Remove it
                    newMarkers.removeAt(i);
                    i--;
                    markersModified = true;
                } else if (marker.start > m_markerRemoveEnd) {
                    // This marker is after the removed segment. Shift it left
                    marker.start -= m_markerRemoveEnd - m_markerRemoveStart;
                    marker.end -= m_markerRemoveEnd - m_markerRemoveStart;
                    markersModified = true;
                }
            }
            if (markersModified) {
                m_markersModel.doReplace(newMarkers);
            }
        }
        if (!markersModified) {
            m_markerRemoveStart = -1;
            m_markers.clear();
        }
    }

    m_undoHelper.recordBeforeState();
    m_model.removeClip(m_trackIndex, m_clipIndex, m_rippleAllTracks);
    m_undoHelper.recordAfterState();
}

void RemoveCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    m_undoHelper.undoChanges();
    if (m_rippleMarkers && m_markerRemoveStart >= 0) {
        m_markersModel.doReplace(m_markers);
    }
}

NameTrackCommand::NameTrackCommand(MultitrackModel &model, int trackIndex,
                                   const QString &name, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_name(name)
    , m_oldName(model.data(m_model.index(trackIndex), MultitrackModel::NameRole).toString())
{
    setText(QObject::tr("Change track name"));
}

void NameTrackCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "name" << m_name;
    m_model.setTrackName(m_trackIndex, m_name);
}

void NameTrackCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "name" << m_name;
    m_model.setTrackName(m_trackIndex, m_oldName);
}

MergeCommand::MergeCommand(MultitrackModel &model, int trackIndex, int clipIndex,
                           QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Merge adjacent clips"));
}

void MergeCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipindex" << m_clipIndex;
    m_undoHelper.recordBeforeState();
    m_model.mergeClipWithNext(m_trackIndex, m_clipIndex, false);
    m_undoHelper.recordAfterState();
}

void MergeCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipindex" << m_clipIndex;
    m_undoHelper.undoChanges();
}


MuteTrackCommand::MuteTrackCommand(MultitrackModel &model, int trackIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_oldValue(model.data(m_model.index(trackIndex), MultitrackModel::IsMuteRole).toBool())
{
    setText(QObject::tr("Toggle track mute"));
}

void MuteTrackCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "mute" << !m_oldValue;
    m_model.setTrackMute(m_trackIndex, !m_oldValue);
}

void MuteTrackCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "mute" << !m_oldValue;
    m_model.setTrackMute(m_trackIndex, m_oldValue);
}

HideTrackCommand::HideTrackCommand(MultitrackModel &model, int trackIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_oldValue(model.data(m_model.index(trackIndex), MultitrackModel::IsHiddenRole).toBool())
{
    setText(QObject::tr("Toggle track hidden"));
}

void HideTrackCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "hide" << !m_oldValue;
    m_model.setTrackHidden(m_trackIndex, !m_oldValue);
}

void HideTrackCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "hide" << !m_oldValue;
    m_model.setTrackHidden(m_trackIndex, m_oldValue);
}

CompositeTrackCommand::CompositeTrackCommand(MultitrackModel &model, int trackIndex, bool value,
                                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_value(value)
    , m_oldValue(model.data(m_model.index(trackIndex), MultitrackModel::IsCompositeRole).toBool())
{
    setText(QObject::tr("Change track compositing"));
}

void CompositeTrackCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "composite" << m_value;
    m_model.setTrackComposite(m_trackIndex, m_value);
}

void CompositeTrackCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "composite" << m_value;
    m_model.setTrackComposite(m_trackIndex, m_oldValue);
}

LockTrackCommand::LockTrackCommand(MultitrackModel &model, int trackIndex, bool value,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_value(value)
    , m_oldValue(model.data(m_model.index(trackIndex), MultitrackModel::IsLockedRole).toBool())
{
    setText(QObject::tr("Lock track"));
}

void LockTrackCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "lock" << m_value;
    m_model.setTrackLock(m_trackIndex, m_value);
}

void LockTrackCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "lock" << m_value;
    m_model.setTrackLock(m_trackIndex, m_oldValue);
}

MoveClipCommand::MoveClipCommand(MultitrackModel &model, MarkersModel &markersModel, int trackDelta,
                                 bool ripple, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_markersModel(markersModel)
    , m_trackDelta(trackDelta)
    , m_ripple(ripple)
    , m_rippleAllTracks(Settings.timelineRippleAllTracks())
    , m_rippleMarkers(Settings.timelineRippleMarkers())
    , m_undoHelper(m_model)
    , m_redo(false)
    , m_start(-1)
    , m_markerOldStart(-1)
    , m_markerNewStart(-1)
{
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
    m_undoHelper.recordBeforeState();
}

void MoveClipCommand::redo()
{
    LOG_DEBUG() << "track delta" << m_trackDelta;
    int trackIndex, clipIndex;
    QList<Mlt::Producer> newSelection;

    if (!m_redo) {
        if (m_selection.size() > 1)
            setText(QObject::tr("Move %n timeline clips", nullptr, m_selection.size()));
        else
            setText(QObject::tr("Move timeline clip"));
    }
    if (m_ripple && !m_trackDelta && m_selection.size() == 1) {
        if (m_start == -1) {
            QScopedPointer<Mlt::ClipInfo> info(m_model.findClipByUuid(MLT.uuid(m_selection.first()), trackIndex,
                                                                      clipIndex));
            if (info && info->cut) {
                auto newStart = info->cut->get_int(kPlaylistStartProperty);
                auto mlt_index = m_model.trackList().at(trackIndex).mlt_index;
                QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(mlt_index));
                if (track) {
                    Mlt::Playlist playlist(*track);
                    auto targetIndex = playlist.get_clip_index_at(newStart);
                    if (targetIndex >= clipIndex || // pushing clips on same track
                            // pulling clips on same track
                            (playlist.is_blank_at(newStart) && targetIndex == clipIndex - 1)) {
                        m_start = newStart;
                        m_trackIndex = trackIndex;
                        m_clipIndex = clipIndex;
                        m_markerOldStart = info->start;
                        m_markerNewStart = newStart;
                    }
                }
            }
        }
        if (m_start >= 0) {
            // Use old behavior to push or pull clips on the same track.
            m_model.moveClip(m_trackIndex, m_trackIndex, m_clipIndex, m_start, m_ripple, m_rippleAllTracks);
            m_undoHelper.recordAfterState();
            redoMarkers();
            m_redo = true;
            return;
        }
    }
    // First, remove each clip.
    int i = 0;
    for (auto &clip : m_selection) {
        if (!m_redo) {
            // On the initial pass, remove clips while recording info about them.
            QScopedPointer<Mlt::ClipInfo> info(m_model.findClipByUuid(MLT.uuid(clip), trackIndex, clipIndex));
            if (info && info->producer && info->producer->is_valid() && info->cut) {
                m_newTrackIndexList << qBound(0, trackIndex + m_trackDelta, m_model.trackList().size() - 1);
                info->producer->pass_property(*info->cut, kPlaylistStartProperty);
                m_playlistStartList << info->cut->get_int(kPlaylistStartProperty);
                info->producer->set(kTrackIndexProperty, trackIndex);
                m_trackIndexList << trackIndex;
                m_clipIndexList << clipIndex;
                m_inList << info->frame_in;
                m_outList << info->frame_out;
                newSelection << info->producer;
                if (m_markerOldStart < 0 || m_markerOldStart > info->start) {
                    // Record the left most clip position being moved
                    m_markerOldStart = info->start;
                    m_markerNewStart = info->cut->get_int(kPlaylistStartProperty);
                }
            }
        } else {
            // On redo, use the recorded indices to remove them.
            trackIndex = m_trackIndexList[i];
            clipIndex = m_clipIndexList[i];
        }
        if (m_ripple)
            m_model.removeClip(trackIndex, clipIndex, m_rippleAllTracks);
        else
            m_model.liftClip(trackIndex, clipIndex);
        ++i;
    }
    if (!m_redo) {
        m_selection = newSelection;
    }
    // Next, add the save clips to their new locations.
    i = 0;
    for (auto &clip : m_selection) {
        auto toTrack = m_newTrackIndexList[i];
        auto start = m_playlistStartList[i];
        clip.set_in_and_out(m_inList[i], m_outList[i]);
        if (start + clip.get_playtime() >= 0) {
//            LOG_DEBUG() << "adding clip" << m_clipIndexList[i] << "to track" << toTrack << "start" << start;
            if (m_ripple)
                m_model.insertClip(toTrack, clip, start, m_rippleAllTracks);
            else
                m_model.overwrite(toTrack, clip, start, false);
        }
        ++i;
    }

    if (!m_redo) {
        m_redo = true;
        m_undoHelper.recordAfterState();
    }

    redoMarkers();
}

void MoveClipCommand::undo()
{
    LOG_DEBUG() << "track delta" << m_trackDelta;
    m_undoHelper.undoChanges();
    if (m_rippleMarkers && m_markerOldStart >= 0) {
        m_markersModel.doReplace(m_markers);
    }
}

void MoveClipCommand::redoMarkers()
{
    if (m_rippleMarkers && m_markerOldStart >= 0) {
        m_markers = m_markersModel.getMarkers();
        QList<Markers::Marker> newMarkers = m_markers;
        bool markersModified = false;
        int startDelta = m_markerNewStart - m_markerOldStart;
        for (int i = 0; i < newMarkers.size(); i++) {
            Markers::Marker &marker = newMarkers[i];
            if (marker.start < m_markerOldStart &&
                    marker.start > m_markerNewStart) {
                // This marker is in the overwritten segment. Remove it
                newMarkers.removeAt(i);
                i--;
                markersModified = true;
            } else if (marker.start >= m_markerOldStart) {
                // This marker is after the start of the moved segment. Shift it with the move
                marker.start += startDelta;
                marker.end += startDelta;
                markersModified = true;
            }
        }
        if (markersModified) {
            m_markersModel.doReplace(newMarkers);
        } else {
            m_markerOldStart = -1;
            m_markers.clear();
        }
    }
}

TrimClipInCommand::TrimClipInCommand(MultitrackModel &model, MarkersModel &markersModel,
                                     int trackIndex, int clipIndex, int delta, bool ripple, bool redo, QUndoCommand *parent)
    : TrimCommand(parent)
    , m_model(model)
    , m_markersModel(markersModel)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_ripple(ripple)
    , m_rippleAllTracks(Settings.timelineRippleAllTracks())
    , m_rippleMarkers(Settings.timelineRippleMarkers() && m_ripple)
    , m_redo(redo)
{
    setText(QObject::tr("Trim clip in point"));
}

void TrimClipInCommand::redo()
{
    if (m_rippleMarkers) {
        // Remove and shift markers as appropriate
        bool markersModified = false;
        m_markers = m_markersModel.getMarkers();
        if (m_markers.size() > 0) {
            auto mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(mlt_index));
            if (track && track->is_valid()) {
                Mlt::Playlist playlist(*track);
                m_markerRemoveStart = playlist.clip_start(m_clipIndex);
                m_markerRemoveEnd = m_markerRemoveStart + m_delta;
            }
        }
        if (m_markers.size() > 0 && m_markerRemoveStart >= 0) {
            QList<Markers::Marker> newMarkers = m_markers;
            for (int i = 0; i < newMarkers.size(); i++) {
                Markers::Marker &marker = newMarkers[i];
                if (marker.start >= m_markerRemoveStart &&
                        marker.start <= m_markerRemoveEnd) {
                    // This marker is in the removed segment. Remove it
                    newMarkers.removeAt(i);
                    i--;
                    markersModified = true;
                } else if (marker.start > m_markerRemoveEnd) {
                    // This marker is after the removed segment. Shift it left
                    marker.start -= m_markerRemoveEnd - m_markerRemoveStart;
                    marker.end -= m_markerRemoveEnd - m_markerRemoveStart;
                    markersModified = true;
                }
            }
            if (markersModified) {
                m_markersModel.doReplace(newMarkers);
            }
        }
        if (!markersModified) {
            m_markerRemoveStart = -1;
            m_markers.clear();
        }
    }

    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
        m_undoHelper.reset(new UndoHelper(m_model));
        if (m_ripple) {
            m_undoHelper->setHints(UndoHelper::SkipXML);
        } else {
            m_undoHelper->setHints(UndoHelper::RestoreTracks);
        }
        m_undoHelper->recordBeforeState();
        m_model.trimClipIn(m_trackIndex, m_clipIndex, m_delta, m_ripple, m_rippleAllTracks);
        m_undoHelper->recordAfterState();
    } else {
        Q_ASSERT(m_undoHelper);
        m_undoHelper->recordAfterState();
        m_redo = true;
    }
}

void TrimClipInCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
    Q_ASSERT(m_undoHelper);
    m_undoHelper->undoChanges();
    if (m_rippleMarkers && m_markerRemoveStart >= 0) {
        m_markersModel.doReplace(m_markers);
    }
}

bool TrimClipInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipInCommand *that = static_cast<const TrimClipInCommand *>(other);
    LOG_DEBUG() << "this clipIndex" << m_clipIndex << "that clipIndex" << that->m_clipIndex;
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex
            || that->m_ripple != m_ripple || that->m_rippleAllTracks != m_rippleAllTracks
            || that->m_rippleMarkers != m_rippleMarkers)
        return false;
    Q_ASSERT(m_undoHelper);
    m_undoHelper->recordAfterState();
    m_delta += static_cast<const TrimClipInCommand *>(other)->m_delta;
    return true;
}

TrimClipOutCommand::TrimClipOutCommand(MultitrackModel &model, MarkersModel &markersModel,
                                       int trackIndex, int clipIndex, int delta, bool ripple, bool redo, QUndoCommand *parent)
    : TrimCommand(parent)
    , m_model(model)
    , m_markersModel(markersModel)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_ripple(ripple)
    , m_rippleAllTracks(Settings.timelineRippleAllTracks())
    , m_rippleMarkers(Settings.timelineRippleMarkers() && m_ripple)
    , m_redo(redo)
{
    setText(QObject::tr("Trim clip out point"));
}

void TrimClipOutCommand::redo()
{
    if (m_rippleMarkers) {
        // Remove and shift markers as appropriate
        bool markersModified = false;
        m_markers = m_markersModel.getMarkers();
        if (m_markers.size() > 0) {
            auto mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(mlt_index));
            if (track && track->is_valid()) {
                Mlt::Playlist playlist(*track);
                m_markerRemoveStart = playlist.clip_start(m_clipIndex) + playlist.clip_length(
                                          m_clipIndex) - m_delta;
                if (!m_redo) {
                    // For the first redo, the clip has already been trimmed by the timeline dock.
                    // So remove the delta that has already been applied
                    m_markerRemoveStart += m_delta;
                }
                m_markerRemoveEnd = m_markerRemoveStart + m_delta;
            }
        }
        if (m_markers.size() > 0 && m_markerRemoveStart >= 0) {
            QList<Markers::Marker> newMarkers = m_markers;
            for (int i = 0; i < newMarkers.size(); i++) {
                Markers::Marker &marker = newMarkers[i];
                if (marker.start >= m_markerRemoveStart &&
                        marker.start <= m_markerRemoveEnd) {
                    // This marker is in the removed segment. Remove it
                    newMarkers.removeAt(i);
                    i--;
                    markersModified = true;
                } else if (marker.start > m_markerRemoveEnd) {
                    // This marker is after the removed segment. Shift it left
                    marker.start -= m_markerRemoveEnd - m_markerRemoveStart;
                    marker.end -= m_markerRemoveEnd - m_markerRemoveStart;
                    markersModified = true;
                }
            }
            if (markersModified) {
                m_markersModel.doReplace(newMarkers);
            }
        }
        if (!markersModified) {
            m_markerRemoveStart = -1;
            m_markers.clear();
        }
    }

    if (m_redo) {
        m_undoHelper.reset(new UndoHelper(m_model));
        if (!m_ripple)
            m_undoHelper->setHints(UndoHelper::SkipXML);
        m_undoHelper->recordBeforeState();
        m_clipIndex = m_model.trimClipOut(m_trackIndex, m_clipIndex, m_delta, m_ripple, m_rippleAllTracks);
        m_undoHelper->recordAfterState();
    } else {
        Q_ASSERT(m_undoHelper);
        m_undoHelper->recordAfterState();
        m_redo = true;
    }
}

void TrimClipOutCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
    Q_ASSERT(m_undoHelper);
    m_undoHelper->undoChanges();
    if (m_rippleMarkers && m_markerRemoveStart >= 0) {
        m_markersModel.doReplace(m_markers);
    }
}

bool TrimClipOutCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipOutCommand *that = static_cast<const TrimClipOutCommand *>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex
            || that->m_ripple != m_ripple || that->m_rippleAllTracks != m_rippleAllTracks
            || that->m_rippleMarkers != m_rippleMarkers)
        return false;
    Q_ASSERT(m_undoHelper);
    m_undoHelper->recordAfterState();
    m_delta += static_cast<const TrimClipOutCommand *>(other)->m_delta;
    return true;
}

SplitCommand::SplitCommand(MultitrackModel &model, int trackIndex,
                           int clipIndex, int position, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_position(position)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Split clip"));
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
}

void SplitCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position" <<
                m_position;
    m_undoHelper.recordBeforeState();
    m_model.splitClip(m_trackIndex, m_clipIndex, m_position);
    m_undoHelper.recordAfterState();
}

void SplitCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position" <<
                m_position;
    m_undoHelper.undoChanges();
}

FadeInCommand::FadeInCommand(MultitrackModel &model, int trackIndex, int clipIndex, int duration,
                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_duration(qMax(duration, 0))
{
    QModelIndex modelIndex = m_model.index(clipIndex, 0, m_model.index(trackIndex));
    m_previous = model.data(modelIndex, MultitrackModel::FadeInRole).toInt();
    setText(QObject::tr("Adjust fade in"));
}

void FadeInCommand::redo()
{
    m_model.fadeIn(m_trackIndex, m_clipIndex, m_duration);
}

void FadeInCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "duration" <<
                m_duration;
    m_model.fadeIn(m_trackIndex, m_clipIndex, m_previous);
}

bool FadeInCommand::mergeWith(const QUndoCommand *other)
{
    const FadeInCommand *that = static_cast<const FadeInCommand *>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex
            || (!that->m_duration && m_duration != that->m_duration))
        return false;
    m_duration = static_cast<const FadeInCommand *>(other)->m_duration;
    return true;
}

FadeOutCommand::FadeOutCommand(MultitrackModel &model, int trackIndex, int clipIndex, int duration,
                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_duration(qMax(duration, 0))
{
    QModelIndex modelIndex = m_model.index(clipIndex, 0, m_model.index(trackIndex));
    m_previous = model.data(modelIndex, MultitrackModel::FadeOutRole).toInt();
    setText(QObject::tr("Adjust fade out"));
}

void FadeOutCommand::redo()
{
    m_model.fadeOut(m_trackIndex, m_clipIndex, m_duration);
}

void FadeOutCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "duration" <<
                m_duration;
    m_model.fadeOut(m_trackIndex, m_clipIndex, m_previous);
}

bool FadeOutCommand::mergeWith(const QUndoCommand *other)
{
    const FadeOutCommand *that = static_cast<const FadeOutCommand *>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex
            || (!that->m_duration && m_duration != that->m_duration))
        return false;
    m_duration = static_cast<const FadeOutCommand *>(other)->m_duration;
    return true;
}

AddTransitionCommand::AddTransitionCommand(TimelineDock &timeline, int trackIndex, int clipIndex,
                                           int position, bool ripple, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_timeline(timeline)
    , m_model(*m_timeline.model())
    , m_markersModel(*m_timeline.markersModel())
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_position(position)
    , m_transitionIndex(-1)
    , m_ripple(ripple)
    , m_undoHelper(*m_timeline.model())
    , m_rippleAllTracks(Settings.timelineRippleAllTracks())
    , m_rippleMarkers(Settings.timelineRippleMarkers())
    , m_markerOldStart(-1)
    , m_markerNewStart(-1)
{
    setText(QObject::tr("Add transition"));
}

void AddTransitionCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position" <<
                m_position;

    if (m_rippleMarkers) {
        // Calculate the marker delta before moving anything
        auto mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(mlt_index));
        if (track && track->is_valid()) {
            Mlt::Playlist playlist(*track);
            m_markerOldStart = playlist.clip_start(m_clipIndex);
            m_markerNewStart = m_position;
        }
    }

    m_undoHelper.recordBeforeState();
    m_transitionIndex = m_model.addTransition(m_trackIndex, m_clipIndex, m_position, m_ripple,
                                              m_rippleAllTracks);
    LOG_DEBUG() << "m_transitionIndex" << m_transitionIndex;
    m_undoHelper.recordAfterState();

    // Remove and shift markers as appropriate
    bool markersModified = false;
    if (m_transitionIndex >= 0 && m_rippleMarkers && m_markerOldStart >= 0) {
        m_markers = m_markersModel.getMarkers();
        QList<Markers::Marker> newMarkers = m_markers;
        int startDelta = m_markerNewStart - m_markerOldStart;
        for (int i = 0; i < newMarkers.size(); i++) {
            Markers::Marker &marker = newMarkers[i];
            if (marker.start <= m_markerOldStart &&
                    marker.start > m_markerNewStart) {
                // This marker is in the overwritten segment. Remove it
                newMarkers.removeAt(i);
                i--;
                markersModified = true;
            } else if (marker.start >= m_markerOldStart) {
                // This marker is after the start of the moved segment. Shift it with the move
                marker.start += startDelta;
                marker.end += startDelta;
                markersModified = true;
            }
        }
        if (markersModified) {
            m_markersModel.doReplace(newMarkers);
        }
    }
    if (!markersModified) {
        m_markerOldStart = -1;
        m_markers.clear();
    }
}

void AddTransitionCommand::undo()
{
    if (m_transitionIndex >= 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position" <<
                    m_position;
        m_timeline.blockSelection(false);
        m_timeline.setSelection();
        m_undoHelper.undoChanges();
        m_timeline.setSelection(QList<QPoint>() << QPoint(m_clipIndex, m_trackIndex));

        if (m_rippleMarkers && m_markerOldStart >= 0) {
            m_markersModel.doReplace(m_markers);
        }
    }
}

TrimTransitionInCommand::TrimTransitionInCommand(MultitrackModel &model, int trackIndex,
                                                 int clipIndex, int delta, bool redo, QUndoCommand *parent)
    : TrimCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_notify(false)
    , m_redo(redo)
{
    setText(QObject::tr("Trim transition in point"));
}

void TrimTransitionInCommand::redo()
{
    if (m_redo) {
        m_model.trimTransitionIn(m_trackIndex, m_clipIndex, m_delta);
        if (m_notify && m_clipIndex >= 0)
            m_model.notifyClipIn(m_trackIndex, m_clipIndex);
    } else {
        m_redo = true;
    }
}

void TrimTransitionInCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
    if (m_clipIndex >= 0) {
        m_model.trimTransitionIn(m_trackIndex, m_clipIndex, -m_delta);
        m_model.notifyClipIn(m_trackIndex, m_clipIndex);
        m_notify = true;
    } else LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

bool TrimTransitionInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimTransitionInCommand *that = static_cast<const TrimTransitionInCommand *>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    m_delta += static_cast<const TrimTransitionInCommand *>(other)->m_delta;
    return true;
}

TrimTransitionOutCommand::TrimTransitionOutCommand(MultitrackModel &model, int trackIndex,
                                                   int clipIndex, int delta, bool redo, QUndoCommand *parent)
    : TrimCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_notify(false)
    , m_redo(redo)
{
    setText(QObject::tr("Trim transition out point"));
}

void TrimTransitionOutCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
        m_model.trimTransitionOut(m_trackIndex, m_clipIndex, m_delta);
        if (m_notify && m_clipIndex >= 0)
            m_model.notifyClipOut(m_trackIndex, m_clipIndex);
    } else {
        m_redo = true;
    }
}

void TrimTransitionOutCommand::undo()
{
    if (m_clipIndex >= 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
        m_model.trimTransitionOut(m_trackIndex, m_clipIndex, -m_delta);
        m_model.notifyClipOut(m_trackIndex, m_clipIndex);
        m_notify = true;
    } else LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

bool TrimTransitionOutCommand::mergeWith(const QUndoCommand *other)
{
    const TrimTransitionOutCommand *that = static_cast<const TrimTransitionOutCommand *>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    m_delta += static_cast<const TrimTransitionOutCommand *>(other)->m_delta;
    return true;
}

AddTransitionByTrimInCommand::AddTransitionByTrimInCommand(MultitrackModel &model, int trackIndex,
                                                           int clipIndex, int duration, int trimDelta, bool redo, QUndoCommand *parent)
    : TrimCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_duration(duration)
    , m_trimDelta(trimDelta)
    , m_notify(false)
    , m_redo(redo)
{
    setText(QObject::tr("Add transition"));
}

void AddTransitionByTrimInCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_trimDelta
                    << "duration" << m_duration;
        if (m_trimDelta)
            m_model.trimClipIn(m_trackIndex, m_clipIndex + 1, m_trimDelta, false, false);
        m_model.addTransitionByTrimIn(m_trackIndex, m_clipIndex, m_duration);
        if (m_notify && m_clipIndex > 0)
            m_model.notifyClipOut(m_trackIndex, m_clipIndex - 1);
    } else {
        m_redo = true;
    }
}

void AddTransitionByTrimInCommand::undo()
{
    if (m_clipIndex > 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_trimDelta;
        m_model.removeTransitionByTrimIn(m_trackIndex, m_clipIndex, -m_trimDelta);
        m_notify = true;
    } else LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

bool AddTransitionByTrimInCommand::mergeWith(const QUndoCommand *other)
{
    const AddTransitionByTrimInCommand *that = static_cast<const AddTransitionByTrimInCommand *>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex ||
            (that->m_clipIndex != m_clipIndex && m_clipIndex != that->m_clipIndex - 1))
        return false;
    return true;
}

RemoveTransitionByTrimInCommand::RemoveTransitionByTrimInCommand(MultitrackModel &model,
                                                                 int trackIndex, int clipIndex, int delta, bool redo, QUndoCommand *parent)
    : TrimCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_redo(redo)
{
    setText(QObject::tr("Remove transition"));
}

void RemoveTransitionByTrimInCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
        QModelIndex modelIndex = m_model.makeIndex(m_trackIndex, m_clipIndex);
        int n = m_model.data(modelIndex, MultitrackModel::DurationRole).toInt();
        m_model.liftClip(m_trackIndex, m_clipIndex);
        m_model.trimClipIn(m_trackIndex, m_clipIndex + 1, -n, false, false);
        m_model.notifyClipIn(m_trackIndex, m_clipIndex + 1);
    } else {
        m_redo = true;
    }
}

void RemoveTransitionByTrimInCommand::undo()
{
    if (m_clipIndex > 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
        m_model.addTransitionByTrimOut(m_trackIndex, m_clipIndex - 1, m_delta);
        m_model.notifyClipIn(m_trackIndex, m_clipIndex + 1);
    } else LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

RemoveTransitionByTrimOutCommand::RemoveTransitionByTrimOutCommand(MultitrackModel &model,
                                                                   int trackIndex, int clipIndex, int delta, bool redo, QUndoCommand *parent)
    : TrimCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_redo(redo)
{
    setText(QObject::tr("Remove transition"));
}

void RemoveTransitionByTrimOutCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
        QModelIndex modelIndex = m_model.makeIndex(m_trackIndex, m_clipIndex);
        int n = m_model.data(modelIndex, MultitrackModel::DurationRole).toInt();
        m_model.liftClip(m_trackIndex, m_clipIndex);
        m_model.trimClipOut(m_trackIndex, m_clipIndex - 1, -n, false, false);
        m_model.notifyClipOut(m_trackIndex, m_clipIndex - 1);
    } else {
        m_redo = true;
    }
}

void RemoveTransitionByTrimOutCommand::undo()
{
    if (m_clipIndex > 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
        m_model.addTransitionByTrimIn(m_trackIndex, m_clipIndex, m_delta);
        m_model.notifyClipOut(m_trackIndex, m_clipIndex - 1);
    } else LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

AddTransitionByTrimOutCommand::AddTransitionByTrimOutCommand(MultitrackModel &model, int trackIndex,
                                                             int clipIndex, int duration, int trimDelta, bool redo, QUndoCommand *parent)
    : TrimCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_duration(duration)
    , m_trimDelta(trimDelta)
    , m_notify(false)
    , m_redo(redo)
{
    setText(QObject::tr("Add transition"));
}

void AddTransitionByTrimOutCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_trimDelta
                    << "duration" << m_duration;
        if (m_trimDelta)
            m_model.trimClipOut(m_trackIndex, m_clipIndex, m_trimDelta, false, false);
        m_model.addTransitionByTrimOut(m_trackIndex, m_clipIndex, m_duration);
        if (m_notify)
            m_model.notifyClipIn(m_trackIndex, m_clipIndex + 2);
    } else {
        m_redo = true;
    }
}

void AddTransitionByTrimOutCommand::undo()
{
    if (m_clipIndex + 2 < m_model.rowCount(m_model.index(m_trackIndex))) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_trimDelta;
        m_model.removeTransitionByTrimOut(m_trackIndex, m_clipIndex, -m_trimDelta);
        m_notify = true;
    } else LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

bool AddTransitionByTrimOutCommand::mergeWith(const QUndoCommand *other)
{
    const AddTransitionByTrimOutCommand *that = static_cast<const AddTransitionByTrimOutCommand *>
                                                (other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    return true;
}

AddTrackCommand::AddTrackCommand(MultitrackModel &model, bool isVideo, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_isVideo(isVideo)
{
    if (isVideo)
        setText(QObject::tr("Add video track"));
    else
        setText(QObject::tr("Add audio track"));
}

void AddTrackCommand::redo()
{
    LOG_DEBUG() << (m_isVideo ? "video" : "audio");
    if (m_isVideo)
        m_trackIndex = m_model.addVideoTrack();
    else
        m_trackIndex = m_model.addAudioTrack();
}

void AddTrackCommand::undo()
{
    LOG_DEBUG() << (m_isVideo ? "video" : "audio");
    m_model.removeTrack(m_trackIndex);
}

InsertTrackCommand::InsertTrackCommand(MultitrackModel &model, int trackIndex, TrackType trackType,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_trackType(trackType)
{
    if (trackType != AudioTrackType && trackType != VideoTrackType) {
        m_trackType = model.trackList().size() > 0 ? model.trackList().at(trackIndex).type : VideoTrackType;
    }
    if (m_trackType == AudioTrackType)
        setText(QObject::tr("Insert audio track"));
    else if (m_trackType == VideoTrackType)
        setText(QObject::tr("Insert video track"));
}

void InsertTrackCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "type" << (m_trackType == AudioTrackType ? "audio" :
                                                              "video");
    m_model.insertTrack(m_trackIndex, m_trackType);
}

void InsertTrackCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "type" << (m_trackType == AudioTrackType ? "audio" :
                                                              "video");
    m_model.removeTrack(m_trackIndex);
}

RemoveTrackCommand::RemoveTrackCommand(MultitrackModel &model, int trackIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_trackType(model.trackList().at(trackIndex).type)
    , m_undoHelper(model)
{
    if (m_trackType == AudioTrackType)
        setText(QObject::tr("Remove audio track"));
    else if (m_trackType == VideoTrackType)
        setText(QObject::tr("Remove video track"));

    // Get the track as MLT playlist.
    int mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> producer(m_model.tractor()->multitrack()->track(mlt_index));
    if (producer && producer->is_valid()) {
        // Save track name.
        m_trackName = QString::fromUtf8(producer->get(kTrackNameProperty));
        // Save the track filters.
        if (producer->filter_count() > 0) {
            m_filtersProducer.reset(new Mlt::Producer(MLT.profile(), "color"));
            if (m_filtersProducer->is_valid())
                MLT.copyFilters(*producer, *m_filtersProducer);
        }
    }
}

void RemoveTrackCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "type" << (m_trackType == AudioTrackType ? "audio" :
                                                              "video");
    m_undoHelper.recordBeforeState();
    int mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> producer(m_model.tractor()->multitrack()->track(mlt_index));
    Mlt::Playlist playlist(*producer);
    playlist.clear();
    m_undoHelper.recordAfterState();
    m_model.removeTrack(m_trackIndex);
}

void RemoveTrackCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "type" << (m_trackType == AudioTrackType ? "audio" :
                                                              "video");
    m_model.insertTrack(m_trackIndex, m_trackType);
    m_model.setTrackName(m_trackIndex, m_trackName);

    // Restore track contents from UndoHelper.
    m_undoHelper.undoChanges();

    // Re-attach filters.
    int mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> producer(m_model.tractor()->multitrack()->track(mlt_index));
    Mlt::Playlist playlist(*producer);
    if (playlist.is_valid() && m_filtersProducer &&  m_filtersProducer->is_valid()) {
        MLT.copyFilters(*m_filtersProducer, playlist);
        QModelIndex modelIndex = m_model.index(m_trackIndex);
        emit m_model.dataChanged(modelIndex, modelIndex, QVector<int>() << MultitrackModel::IsFilteredRole);
    }
}

MoveTrackCommand::MoveTrackCommand(MultitrackModel &model, int fromTrackIndex, int toTrackIndex,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_fromTrackIndex(qBound(0, fromTrackIndex, qMax(model.rowCount() - 1, 0)))
    , m_toTrackIndex(qBound(0, toTrackIndex, qMax(model.rowCount() - 1, 0)))
{
    if (m_toTrackIndex < m_fromTrackIndex)
        setText(QObject::tr("Move track down"));
    else
        setText(QObject::tr("Move track up"));
}

void MoveTrackCommand::redo()
{
    LOG_DEBUG() << "fromTrackIndex" << m_fromTrackIndex << "toTrackIndex" << m_toTrackIndex;
    m_model.moveTrack(m_fromTrackIndex, m_toTrackIndex);
}

void MoveTrackCommand::undo()
{
    LOG_DEBUG() << "fromTrackIndex" << m_fromTrackIndex << "toTrackIndex" << m_toTrackIndex;
    m_model.moveTrack(m_toTrackIndex, m_fromTrackIndex);
}

ChangeBlendModeCommand::ChangeBlendModeCommand(Mlt::Transition &transition,
                                               const QString &propertyName, const QString &mode, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_transition(transition)
    , m_propertyName(propertyName)
    , m_newMode(mode)
{
    setText(QObject::tr("Change track blend mode"));
    m_oldMode = m_transition.get(m_propertyName.toLatin1().constData());
}

void ChangeBlendModeCommand::redo()
{
    LOG_DEBUG() << "mode" << m_newMode;
    if (!m_newMode.isEmpty()) {
        m_transition.set("disable", 0);
        m_transition.set(m_propertyName.toLatin1().constData(), m_newMode.toUtf8().constData());
    } else {
        m_transition.set("disable", 1);
    }
    MLT.refreshConsumer();
    emit modeChanged(m_newMode);
}

void ChangeBlendModeCommand::undo()
{
    LOG_DEBUG() << "mode" << m_newMode;
    if (!m_oldMode.isEmpty()) {
        m_transition.set("disable", 0);
        m_transition.set(m_propertyName.toLatin1().constData(), m_oldMode.toUtf8().constData());
    } else {
        m_transition.set("disable", 1);
    }
    MLT.refreshConsumer();
    emit modeChanged(m_oldMode);
}

UpdateCommand::UpdateCommand(TimelineDock &timeline, int trackIndex, int clipIndex,
                             int position, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_timeline(timeline)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_position(position)
    , m_isFirstRedo(true)
    , m_undoHelper(*timeline.model())
    , m_ripple(Settings.timelineRipple())
{
    setText(QObject::tr("Change clip properties"));
    m_undoHelper.recordBeforeState();
}

void UpdateCommand::setXmlAfter(const QString &xml)
{
    m_xmlAfter = xml;
    m_ripple = Settings.timelineRipple();
}

void UpdateCommand::setPosition(int trackIndex, int clipIndex, int position)
{
    if (trackIndex >= 0)
        m_trackIndex = trackIndex;
    if (clipIndex >= 0)
        m_clipIndex = clipIndex;
    if (position >= 0)
        m_position = position;
    m_undoHelper.recordBeforeState();
}

void UpdateCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position" <<
                m_position;
    if (!m_isFirstRedo)
        m_undoHelper.recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xmlAfter.toUtf8().constData());
    if (m_ripple) {
        m_timeline.model()->removeClip(m_trackIndex, m_clipIndex, false);
        m_timeline.model()->insertClip(m_trackIndex, clip, m_position, false, false);
    } else {
        m_timeline.model()->liftClip(m_trackIndex, m_clipIndex);
        m_timeline.model()->overwrite(m_trackIndex, clip, m_position, false);
    }
    m_undoHelper.recordAfterState();
}

void UpdateCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position" <<
                m_position;
    m_undoHelper.undoChanges();
    m_timeline.emitSelectedFromSelection();
    m_isFirstRedo = false;
}

DetachAudioCommand::DetachAudioCommand(MultitrackModel &model, int trackIndex, int clipIndex,
                                       int position, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_position(position)
    , m_targetTrackIndex(-1)
    , m_xml(xml)
    , m_undoHelper(m_model)
    , m_trackAdded(false)
{
    setText(QObject::tr("Detach Audio"));
}

void DetachAudioCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position" <<
                m_position;
    Mlt::Producer audioClip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    Mlt::Producer videoClip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    if (audioClip.is_valid() && videoClip.is_valid()) {

        // Disable audio on the video clip.
        videoClip.set("audio_index", -1);
        // Remove audio filters from the video clip.
        for (int i = 0; i < videoClip.filter_count(); i++) {
            Mlt::Filter *filter = videoClip.filter(i);
            if (filter && filter->is_valid() && !filter->get_int("_loader")) {
                QmlMetadata *newMeta = MAIN.filterController()->metadataForService(filter);
                if (newMeta && newMeta->isAudio()) {
                    videoClip.detach(*filter);
                    i--;
                }
            }
            delete filter;
        }

        // Disable video on the audio clip.
        audioClip.set("video_index", -1);
        // Remove video filters from the audio clip.
        for (int i = 0; i < audioClip.filter_count(); i++) {
            Mlt::Filter *filter = audioClip.filter(i);
            if (filter && filter->is_valid() && !filter->get_int("_loader")) {
                QmlMetadata *newMeta = MAIN.filterController()->metadataForService(filter);
                if (newMeta && !newMeta->isAudio()) {
                    audioClip.detach(*filter);
                    i--;
                }
            }
            delete filter;
        }

        // Add an audio track if needed.
        int n = m_model.trackList().size();
        for (int i = 0; i < n; i++) {
            Mlt::Producer track(m_model.tractor()->track(m_model.trackList()[i].mlt_index));
            if (!track.is_valid())
                continue;
            if (track.get(kAudioTrackProperty)) {
                Mlt::Playlist playlist(track);
                int out = videoClip.get_playtime() - 1;
                // If the audio track is blank in the target region.
                if (playlist.is_blank_at(m_position) && playlist.is_blank_at(m_position + out)
                        && playlist.get_clip_index_at(m_position) == playlist.get_clip_index_at(m_position + out)) {
                    // Save the target track index.
                    m_targetTrackIndex = i;
                    break;
                }
            }
        }
        if (m_targetTrackIndex == -1) {
            // No target audio track
            m_targetTrackIndex = m_model.addAudioTrack();
            m_trackAdded = m_targetTrackIndex > -1;
        }

        if (m_targetTrackIndex > -1) {
            // Add the clip to the new audio track.
            m_undoHelper.recordBeforeState();
            m_model.overwrite(m_targetTrackIndex, audioClip, m_position, false);
            m_model.overwrite(m_trackIndex, videoClip, m_position, false);
            m_undoHelper.recordAfterState();
            QModelIndex modelIndex = m_model.makeIndex(m_trackIndex, m_clipIndex);
            emit m_model.dataChanged(modelIndex, modelIndex, QVector<int>() << MultitrackModel::AudioIndexRole);
        }
    }
}

void DetachAudioCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position" <<
                m_position;
    m_undoHelper.undoChanges();
    if (m_trackAdded) {
        // Remove the new audio track.
        m_model.removeTrack(m_targetTrackIndex);
        m_targetTrackIndex = -1;
    }
    Mlt::Producer originalClip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.overwrite(m_trackIndex, originalClip, m_position, true);
    QModelIndex modelIndex = m_model.makeIndex(m_trackIndex, m_clipIndex);
    emit m_model.dataChanged(modelIndex, modelIndex,
                             QVector<int>() << MultitrackModel::AudioIndexRole << MultitrackModel::AudioLevelsRole);
}

ReplaceCommand::ReplaceCommand(MultitrackModel &model, int trackIndex, int clipIndex,
                               const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_xml(xml)
    , m_isFirstRedo(true)
    , m_undoHelper(model)
{
    setText(QObject::tr("Replace timeline clip"));
    m_undoHelper.recordBeforeState();
}

void ReplaceCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    if (!m_isFirstRedo)
        m_undoHelper.recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.replace(m_trackIndex, m_clipIndex, clip);
    m_undoHelper.recordAfterState();
}

void ReplaceCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    m_undoHelper.undoChanges();
    m_isFirstRedo = false;
}

AlignClipsCommand::AlignClipsCommand(MultitrackModel &model, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_undoHelper(m_model)
    , m_redo(false)
{
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
    m_undoHelper.recordBeforeState();
    setText(QObject::tr("Align clips to reference track"));
}

void AlignClipsCommand::addAlignment(QUuid uuid, int offset, double speed)
{
    Alignment alignment;
    alignment.uuid = uuid;
    alignment.offset = offset;
    alignment.speed = speed;
    m_alignments.append(alignment);
}

void AlignClipsCommand::redo()
{
    LOG_DEBUG() << "Alignment Clips:" << m_alignments.size();
    struct ClipItem {
        Mlt::Producer *clip;
        int track;
        int start;
    };
    QVector<ClipItem> clipMemory;

    // Remove all the clips and remember them.
    for (auto &alignment : m_alignments) {
        int trackIndex, clipIndex;
        QScopedPointer<Mlt::ClipInfo> info(m_model.findClipByUuid(alignment.uuid, trackIndex, clipIndex));
        if (!info || !info->cut || !info->cut->is_valid()) {
            continue;
        }
        ClipItem item;
        if (alignment.speed != 1.0) {
            double warpspeed = Util::GetSpeedFromProducer(info->producer) * alignment.speed;
            QString filename = Util::GetFilenameFromProducer(info->producer, false);
            QString s = QString("%1:%2:%3").arg("timewarp").arg(warpspeed).arg(filename);
            item.clip = new Mlt::Producer(MLT.profile(), s.toUtf8().constData());
            if (!item.clip || !item.clip->is_valid()) {
                delete item.clip;
                continue;
            }
            Util::passProducerProperties(info->producer, item.clip);
            Util::updateCaption(item.clip);
            int length = qRound(info->producer->get_length() / alignment.speed);
            int in = qRound(info->cut->get_in() / alignment.speed);
            int out = qRound(info->cut->get_out() / alignment.speed);
            item.clip->set("length", item.clip->frames_to_time(length, mlt_time_clock));
            item.clip->set_in_and_out(in, out);
            MLT.copyFilters(*info->producer, *item.clip);
        } else {
            item.clip = new Mlt::Producer(info->cut);
        }
        item.track = trackIndex;
        item.start = alignment.offset;
        clipMemory.append(item);
        m_model.liftClip(trackIndex, clipIndex);
    }

    // Place all the clips back in the new spot.
    for (auto &item : clipMemory) {
        m_model.overwrite(item.track, *item.clip, item.start, false, false);
        delete item.clip;
    }

    if (!m_redo) {
        m_redo = true;
        m_undoHelper.recordAfterState();
    }
}

void AlignClipsCommand::undo()
{
    m_undoHelper.undoChanges();
}

} // namespace

#include "moc_timelinecommands.cpp"
