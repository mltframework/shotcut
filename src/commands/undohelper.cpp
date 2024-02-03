/*
 * Copyright (c) 2015-2023 Meltytech, LLC
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

#include "undohelper.h"
#include "mltcontroller.h"
#include "models/audiolevelstask.h"
#include "shotcut_mlt_properties.h"
#include <Logger.h>
#include <QScopedPointer>
#include <QUuid>

#ifdef UNDOHELPER_DEBUG
#define UNDOLOG LOG_DEBUG()
#else
#define UNDOLOG if (false) LOG_DEBUG()
#endif

UndoHelper::UndoHelper(MultitrackModel &model)
    : m_model(model)
    , m_hints(NoHints)
{
}

void UndoHelper::recordBeforeState()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("Before state");
#endif
    m_state.clear();
    m_clipsAdded.clear();
    m_insertedOrder.clear();
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            QUuid uid = MLT.ensureHasUuid(clip->parent());
            if (clip->is_blank()) {
                uid = MLT.ensureHasUuid(*clip);
            }
            m_insertedOrder << uid;
            Info &info = m_state[uid];
            if (!(m_hints & SkipXML))
                info.xml = MLT.XML(&clip->parent());
            Mlt::ClipInfo clipInfo;
            playlist.clip_info(j, &clipInfo);
            info.frame_in = clipInfo.frame_in;
            info.frame_out = clipInfo.frame_out;
            info.oldTrackIndex = i;
            info.oldClipIndex = j;
            info.isBlank = playlist.is_blank(j);
            if (clipInfo.cut && clipInfo.cut->property_exists(kShotcutGroupProperty)) {
                info.group = clipInfo.cut->get_int(kShotcutGroupProperty);
            }
        }
    }
}

void UndoHelper::recordAfterState()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("After state");
#endif
    QList<QUuid> clipsRemoved = m_state.keys();
    m_clipsAdded.clear();
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            QUuid uid = MLT.ensureHasUuid(clip->parent());
            if (clip->is_blank()) {
                uid = MLT.ensureHasUuid(*clip);
            }

            /* Clips not previously in m_state are new */
            if (!m_state.contains(uid)) {
                UNDOLOG << "New clip at" << i << j;
                m_clipsAdded << uid;
                m_affectedTracks << i;
            } else {
                Info &info = m_state[uid];
                info.changes = 0;
                info.newTrackIndex = i;
                info.newClipIndex = j;

                /* Indices have changed; these are moved */
                if (info.oldTrackIndex != info.newTrackIndex || info.oldClipIndex != info.newClipIndex) {
                    UNDOLOG << "Clip" << uid << "moved from"
                            << info.oldTrackIndex << info.oldClipIndex
                            << "to"
                            << info.newTrackIndex << info.newClipIndex;
                    info.changes |= Moved;
                    m_affectedTracks << info.oldTrackIndex;
                    m_affectedTracks << info.newTrackIndex;
                }

                if (!(m_hints & SkipXML) && !info.isBlank) {
                    QString newXml = MLT.XML(&clip->parent());
                    if (info.xml != newXml) {
                        UNDOLOG << "Modified xml:" << uid;
                        info.changes |= XMLModified;
                        m_affectedTracks << i;
                    }
                }

                Mlt::ClipInfo newInfo;
                playlist.clip_info(j, &newInfo);
                /* Only in/out point changes are handled at this time. */
                if (info.frame_in != newInfo.frame_in || info.frame_out != newInfo.frame_out) {
                    UNDOLOG << "In/out changed:" << uid;
                    info.changes |= ClipInfoModified;
                    info.in_delta = info.frame_in - newInfo.frame_in;
                    info.out_delta = newInfo.frame_out - info.frame_out;
                    m_affectedTracks << i;
                }
            }
            clipsRemoved.removeOne(uid);
        }
    }

    /* Clips that did not show up are removed from the timeline */
    foreach (QUuid uid, clipsRemoved) {
        UNDOLOG << "Clip removed:" << uid;
        auto &info = m_state[uid];
        info.changes = Removed;
        m_affectedTracks << info.oldTrackIndex;
    }
}

void UndoHelper::undoChanges()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("Before undo");
#endif
    if (m_hints & RestoreTracks) {
        restoreAffectedTracks();
        emit m_model.modified();
#ifdef UNDOHELPER_DEBUG
        debugPrintState("After undo");
#endif
        return;
    }
    QMap<int, int> indexAdjustment;

    /* We're walking through the list in the order of uids, which is the order in which the
     * clips were laid out originally. As we go through the clips we make sure the clips behind
     * the current index are as they were originally before we move on to the next one */
    foreach (QUuid uid, m_insertedOrder) {
        const Info &info = m_state[uid];
        UNDOLOG << "Handling uid" << uid << "on track" << info.oldTrackIndex << "index" <<
                info.oldClipIndex;

        int trackIndex = m_model.trackList()[info.oldTrackIndex].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(trackIndex));
        Mlt::Playlist playlist(*trackProducer);

        /* This is the index in the track we're currently restoring */
        int currentIndex = qMin(info.oldClipIndex + indexAdjustment[trackIndex], playlist.count() - 1);

        /* Clips that were moved are simply searched for using the uid, and moved in place. We
         * do not use the indices directly because they become invalid once the playlist is
         * modified. */
        if (info.changes & Moved) {
            Q_ASSERT(info.newTrackIndex == info.oldTrackIndex && "cross-track moves are unsupported so far");
            int clipCurrentlyAt = -1;
            for (int i = 0; i < playlist.count(); ++i) {
                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(i));
                if (MLT.uuid(clip->parent()) == uid || MLT.uuid(*clip) == uid) {
                    clipCurrentlyAt = i;
                    break;
                }
            }
            Q_ASSERT(clipCurrentlyAt != -1 && "Moved clip could not be found");
            UNDOLOG << "Found clip with uid" << uid << "at index" << clipCurrentlyAt;

            if (clipCurrentlyAt != info.oldClipIndex &&
                    (currentIndex < clipCurrentlyAt || currentIndex > clipCurrentlyAt + 1)) {
                UNDOLOG << "moving from" << clipCurrentlyAt << "to" << currentIndex;
                QModelIndex modelIndex = m_model.createIndex(clipCurrentlyAt, 0, info.oldTrackIndex);
                m_model.beginMoveRows(modelIndex.parent(), clipCurrentlyAt, clipCurrentlyAt, modelIndex.parent(),
                                      currentIndex);
                playlist.move(clipCurrentlyAt, currentIndex);
                m_model.endMoveRows();
            }
        }

        /* Removed clips are reinserted using their stored XML */
        if (info.changes & Removed) {
            QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
            m_model.beginInsertRows(modelIndex.parent(), currentIndex, currentIndex);
            if (info.isBlank) {
                playlist.insert_blank(currentIndex, info.frame_out - info.frame_in);
                UNDOLOG << "inserting isBlank at " << currentIndex;
            } else {
                UNDOLOG << "inserting clip at " << currentIndex << uid;
                Q_ASSERT(!(m_hints & SkipXML) && "Cannot restore clip without stored XML");
                Q_ASSERT(!info.xml.isEmpty());
                Mlt::Producer restoredClip(MLT.profile(), "xml-string", info.xml.toUtf8().constData());
                if (restoredClip.type() == mlt_service_tractor_type) { // transition
                    restoredClip.set("mlt_type", "mlt_producer");
                } else {
                    fixTransitions(playlist, currentIndex, restoredClip);
                }
                playlist.insert(restoredClip, currentIndex, info.frame_in, info.frame_out);
            }
            m_model.endInsertRows();

            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
            Q_ASSERT(currentIndex < playlist.count());
            Q_ASSERT(!clip.isNull());
            if (info.isBlank) {
                MLT.setUuid(*clip, uid);
            } else {
                MLT.setUuid(clip->parent(), uid);
            }
            if (info.group >= 0) {
                clip->set(kShotcutGroupProperty, info.group);
            }
            AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
            indexAdjustment[trackIndex]++;
        }

        /* Only in/out points handled so far */
        if (info.changes & ClipInfoModified) {
            int filterIn = MLT.filterIn(playlist, currentIndex);
            int filterOut = MLT.filterOut(playlist, currentIndex);

            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
            if (clip && clip->is_valid()) {
                UNDOLOG << "resizing clip at" << currentIndex << "in" << info.frame_in << "out" << info.frame_out;
                if (clip->parent().get_data("mlt_mix"))
                    clip->parent().set("mlt_mix", nullptr, 0);
                if (clip->get_data("mix_in"))
                    clip->set("mix_in", nullptr, 0);
                if (clip->get_data("mix_out"))
                    clip->set("mix_out", nullptr, 0);
                playlist.resize_clip(currentIndex, info.frame_in, info.frame_out);
                MLT.adjustClipFilters(clip->parent(), filterIn, filterOut, info.in_delta, info.out_delta,
                                      info.in_delta);
            }

            QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
            QVector<int> roles;
            roles << MultitrackModel::InPointRole;
            roles << MultitrackModel::OutPointRole;
            roles << MultitrackModel::DurationRole;
            emit m_model.dataChanged(modelIndex, modelIndex, roles);
            if (clip && clip->is_valid())
                AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
        }
    }

    /* Finally we walk through the tracks once more, removing clips that
     * were added, and clearing the temporarily used uid property */
    int trackIndex = 0;
    foreach (const Track &track, m_model.trackList()) {
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(track.mlt_index));
        Mlt::Playlist playlist(*trackProducer);
        for (int i = playlist.count() - 1; i >= 0; --i) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(i));
            QUuid uid = MLT.uuid(clip->parent());
            if (clip->is_blank()) {
                uid = MLT.uuid(*clip);
            }
            if (m_clipsAdded.removeOne(uid)) {
                UNDOLOG << "Removing clip at" << i;
                m_model.beginRemoveRows(m_model.index(trackIndex), i, i);
                if (clip->parent().get_data("mlt_mix"))
                    clip->parent().set("mlt_mix", NULL, 0);
                if (clip->get_data("mix_in"))
                    clip->set("mix_in", NULL, 0);
                if (clip->get_data("mix_out"))
                    clip->set("mix_out", NULL, 0);
                playlist.remove(i);
                m_model.endRemoveRows();
            }
        }
        trackIndex++;
    }

    emit m_model.modified();
#ifdef UNDOHELPER_DEBUG
    debugPrintState("After undo");
#endif
}

void UndoHelper::setHints(OptimizationHints hints)
{
    m_hints = hints;
}

void UndoHelper::debugPrintState(const QString &title)
{
    LOG_DEBUG() << "timeline state:" << title << "{";
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QString trackStr = QString("   track %1 (mlt-idx %2):").arg(i).arg(mltIndex);
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            Mlt::ClipInfo info;
            playlist.clip_info(j, &info);
            QUuid uid = MLT.uuid(*info.producer);
            if (info.producer->is_blank() && info.cut) {
                uid = MLT.uuid(*info.cut);
            }
            trackStr += QString(" [ %5 %1 -> %2 (%3 frames) %4]").arg(info.frame_in).arg(info.frame_out).arg(
                            info.frame_count).arg(info.cut->is_blank() ? "blank " : "clip").arg(uid.toString());
        }
        LOG_DEBUG() << qPrintable(trackStr);
    }
    LOG_DEBUG() << "}";
}

void UndoHelper::restoreAffectedTracks()
{
    // Remove everything in the affected tracks.
    for (const auto &trackIndex : qAsConst(m_affectedTracks)) {
        if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
            auto mlt_index = m_model.trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> producer(m_model.tractor()->track(mlt_index));
            if (producer->is_valid()) {
                Mlt::Playlist playlist(*producer.data());
                m_model.beginRemoveRows(m_model.index(trackIndex), 0, playlist.count() - 1);
                UNDOLOG << "clearing track" << trackIndex;
                playlist.clear();
                m_model.endRemoveRows();
            }
        }
    }

    for (const auto &uid : qAsConst(m_insertedOrder)) {
        const Info &info = m_state[uid];
        if (m_affectedTracks.contains(info.oldTrackIndex)) {
            UNDOLOG << "Handling uid" << uid << "on track" << info.oldTrackIndex << "index" <<
                    info.oldClipIndex;
            // Clips are restored using their stored XML.
            int mltIndex = m_model.trackList()[info.oldTrackIndex].mlt_index;
            QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
            Mlt::Playlist playlist(*trackProducer);
            auto currentIndex = playlist.count();
            QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
            m_model.beginInsertRows(modelIndex.parent(), currentIndex, currentIndex);
            if (info.isBlank) {
                playlist.blank(info.frame_out - info.frame_in);
                UNDOLOG << "appending blank at" << currentIndex << info.frame_out << info.frame_in;
            } else {
                UNDOLOG << "appending clip at" << currentIndex;
                Q_ASSERT(!(m_hints & SkipXML) && "Cannot restore clip without stored XML");
                Q_ASSERT(!info.xml.isEmpty());
                Mlt::Producer restoredClip(MLT.profile(), "xml-string", info.xml.toUtf8().constData());
                if (restoredClip.type() == mlt_service_tractor_type) { // transition
                    restoredClip.set("mlt_type", "mlt_producer");
                }
                playlist.append(restoredClip, info.frame_in, info.frame_out);
                if (info.group >= 0) {
                    QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
                    clip->set(kShotcutGroupProperty, info.group);
                }
            }
            m_model.endInsertRows();

            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
            Q_ASSERT(currentIndex < playlist.count());
            Q_ASSERT(!clip.isNull());
            if (info.isBlank) {
                MLT.setUuid(*clip, uid);
            } else {
                MLT.setUuid(clip->parent(), uid);
            }
            AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
        }
    }
    for (const auto &trackIndex : qAsConst(m_affectedTracks)) {
        if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
            auto mlt_index = m_model.trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> producer(m_model.tractor()->track(mlt_index));
            if (producer->is_valid()) {
                Mlt::Playlist playlist(*producer.data());
                for (auto currentIndex = 0; currentIndex < playlist.count(); currentIndex++) {
                    Mlt::Producer clip = playlist.get_clip(currentIndex);
                    fixTransitions(playlist, currentIndex, clip);
                }
            }
        }
    }
}

void UndoHelper::fixTransitions(Mlt::Playlist playlist, int clipIndex, Mlt::Producer clip)
{
    if (clip.is_blank()) {
        return;
    }
    int transitionIndex = 0;
    for (auto currentIndex : {
                clipIndex + 1, clipIndex - 1
            }) {
        // Connect a transition on the right/left to the new producer.
        Mlt::Producer producer(playlist.get_clip(currentIndex));
        if (producer.is_valid() && producer.parent().get(kShotcutTransitionProperty)) {
            Mlt::Tractor transition(producer.parent());
            if (transition.is_valid()) {
                QScopedPointer<Mlt::Producer> transitionClip(transition.track(transitionIndex));
                if (transitionClip->is_valid()
                        && transitionClip->parent().get_service() != clip.parent().get_service()) {
                    UNDOLOG << "Fixing transition at clip index" << currentIndex << "transition index" <<
                            transitionIndex;
                    transitionClip.reset(clip.cut(transitionClip->get_in(), transitionClip->get_out()));
                    transition.set_track(*transitionClip.data(), transitionIndex);
                }
            }
        }
        transitionIndex++;
    }
}
