/*
 * Copyright (c) 2015-2026 Meltytech, LLC
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

#include "Logger.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/audiolevelstask.h"
#include "shotcut_mlt_properties.h"

#include <MltTractor.h>
#include <QObject>
#include <QScopedPointer>
#include <QSet>
#include <QUuid>

#ifdef UNDOHELPER_DEBUG
#define UNDOLOG LOG_DEBUG()
#else
#define UNDOLOG \
    if (false) \
    LOG_DEBUG()
#endif

// Omit profile and avformat metadata. Restore does not need them; they dominate
// snapshot size when every clip on a large timeline is serialized.
static QString xmlForUndo(Mlt::Producer *producer)
{
    if (!producer || !producer->is_valid())
        return QString();
    return MLT.XML(producer, false, false);
}

// Prefer the expected index before a linear scan of a long playlist.
static bool clipHasUuid(Mlt::Playlist &playlist, int index, const QUuid &uid)
{
    if (index < 0 || index >= playlist.count())
        return false;
    QScopedPointer<Mlt::Producer> clip(playlist.get_clip(index));
    return clip && clip->is_valid() && (MLT.uuid(clip->parent()) == uid || MLT.uuid(*clip) == uid);
}

static int indexOfUuid(Mlt::Playlist &playlist, const QUuid &uid, int hint1 = -1, int hint2 = -1)
{
    if (clipHasUuid(playlist, hint1, uid))
        return hint1;
    if (hint2 != hint1 && clipHasUuid(playlist, hint2, uid))
        return hint2;
    for (int i = 0; i < playlist.count(); ++i) {
        if (i != hint1 && i != hint2 && clipHasUuid(playlist, i, uid))
            return i;
    }
    return -1;
}

UndoHelper::UndoHelper(MultitrackModel &model)
    : m_model(model)
    , m_hints(NoHints)
    , m_hasRestriction(false)
    , m_undoFailed(false)
{}

bool UndoHelper::includeTrack(int trackIndex) const
{
    return !m_hasRestriction || m_restrictedTracks.contains(trackIndex);
}

void UndoHelper::failUndo(const QString &detail)
{
    LOG_ERROR() << detail;
    m_undoFailed = true;
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
        if (!includeTrack(i))
            continue;
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        if (!trackProducer || !trackProducer->is_valid())
            continue;
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            if (!clip || !clip->is_valid())
                continue;
            QUuid uid = MLT.ensureHasUuid(clip->parent());
            if (clip->is_blank()) {
                uid = MLT.ensureHasUuid(*clip);
            }
            m_insertedOrder << uid;
            Info &info = m_state[uid];
            // Blanks are restored from in/out; skip their XML.
            // SkipXML: XML only for clips the command listed (storeXmlForClip).
            if (!clip->is_blank() && (!(m_hints & SkipXML) || m_xmlClips.contains(uid)))
                info.xml = xmlForUndo(&clip->parent());
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
    const auto stateKeys = m_state.keys();
    QSet<QUuid> clipsRemoved(stateKeys.begin(), stateKeys.end());
    m_clipsAdded.clear();
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        if (!includeTrack(i))
            continue;
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        if (!trackProducer || !trackProducer->is_valid())
            continue;
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            if (!clip || !clip->is_valid())
                continue;
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

                // SkipXML: only relocate clips the command stored. Blank-merge
                // shifts every later clip; moving those hangs large timelines.
                if (info.oldTrackIndex != info.newTrackIndex
                    || info.oldClipIndex != info.newClipIndex) {
                    UNDOLOG << "Clip" << uid << "moved from" << info.oldTrackIndex
                            << info.oldClipIndex << "to" << info.newTrackIndex << info.newClipIndex;
                    if (!(m_hints & SkipXML) || m_xmlClips.contains(uid))
                        info.changes |= Moved;
                    m_affectedTracks << info.oldTrackIndex;
                    m_affectedTracks << info.newTrackIndex;
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
            clipsRemoved.remove(uid);
        }
    }

    /* Clips that did not show up are removed from the timeline */
    for (const QUuid &uid : std::as_const(clipsRemoved)) {
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
    m_undoFailed = false;
    if (m_hints & RestoreTracks) {
        // restrictToTrack sets the scope before any clips look "affected".
        if (m_affectedTracks.isEmpty() && m_hasRestriction)
            m_affectedTracks = m_restrictedTracks;
        restoreAffectedTracks();
        emit m_model.modified();
#ifdef UNDOHELPER_DEBUG
        debugPrintState("After undo");
#endif
        if (m_undoFailed)
            MAIN.showStatusMessage(
                QObject::tr("Undo incomplete. Some timeline changes could not be restored."));
        return;
    }
    QMap<int, int> indexAdjustment;

    /* We're walking through the list in the order of uids, which is the order in which the
     * clips were laid out originally. As we go through the clips we make sure the clips behind
     * the current index are as they were originally before we move on to the next one */
    foreach (QUuid uid, m_insertedOrder) {
        const Info &info = m_state[uid];
        if (info.changes == NoChange)
            continue; // later clips only shifted index; insert/remove slides them
        UNDOLOG << "Handling uid" << uid << "on track" << info.oldTrackIndex << "index"
                << info.oldClipIndex;

        if (info.oldTrackIndex < 0 || info.oldTrackIndex >= m_model.trackList().count()) {
            failUndo(QStringLiteral("Invalid track index while undoing %1 %2")
                         .arg(uid.toString())
                         .arg(info.oldTrackIndex));
            continue;
        }

        int trackIndex = m_model.trackList()[info.oldTrackIndex].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(trackIndex));
        if (!trackProducer || !trackProducer->is_valid()) {
            failUndo(QStringLiteral("Invalid track producer while undoing %1").arg(uid.toString()));
            continue;
        }
        Mlt::Playlist playlist(*trackProducer);

        /* This is the index in the track we're currently restoring */
        int currentIndex = info.oldClipIndex + indexAdjustment[trackIndex];
        if (playlist.count() > 0)
            currentIndex = qMin(currentIndex, playlist.count() - 1);
        currentIndex = qMax(currentIndex, 0);

        /* Clips that were moved are simply searched for using the uid, and moved in place. We
         * do not use the indices directly because they become invalid once the playlist is
         * modified. */
        if (info.changes & Moved) {
            if (info.newTrackIndex != info.oldTrackIndex) {
                failUndo(QStringLiteral("Cross-track move is not supported by incremental undo %1")
                             .arg(uid.toString()));
                continue;
            }
            int clipCurrentlyAt = indexOfUuid(playlist, uid, info.oldClipIndex, info.newClipIndex);
            if (clipCurrentlyAt == -1) {
                failUndo(QStringLiteral("Moved clip could not be found %1").arg(uid.toString()));
                continue;
            }
            UNDOLOG << "Found clip with uid" << uid << "at index" << clipCurrentlyAt;

            if (clipCurrentlyAt != info.oldClipIndex
                && (currentIndex < clipCurrentlyAt || currentIndex > clipCurrentlyAt + 1)) {
                UNDOLOG << "moving from" << clipCurrentlyAt << "to" << currentIndex;
                QModelIndex modelIndex = m_model.createIndex(clipCurrentlyAt, 0, info.oldTrackIndex);
                m_model.beginMoveRows(modelIndex.parent(),
                                      clipCurrentlyAt,
                                      clipCurrentlyAt,
                                      modelIndex.parent(),
                                      currentIndex);
                playlist.move(clipCurrentlyAt, currentIndex);
                m_model.endMoveRows();
            }
        }

        /* Removed clips are reinserted using their stored XML */
        if (info.changes & Removed) {
            QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
            Mlt::Producer restoredClip;
            if (!info.isBlank) {
                UNDOLOG << "inserting clip at " << currentIndex << uid;
                if (info.xml.isEmpty()) {
                    failUndo(QStringLiteral("Cannot restore clip without stored XML %1")
                                 .arg(uid.toString()));
                    continue;
                }
                restoredClip = Mlt::Producer(MLT.profile(),
                                             "xml-string",
                                             info.xml.toUtf8().constData());
                if (!restoredClip.is_valid()) {
                    failUndo(QStringLiteral("Failed to parse clip XML %1").arg(uid.toString()));
                    continue;
                }
            }
            m_model.beginInsertRows(modelIndex.parent(), currentIndex, currentIndex);
            if (info.isBlank) {
                playlist.insert_blank(currentIndex, info.frame_out - info.frame_in);
                UNDOLOG << "inserting isBlank at " << currentIndex;
            } else {
                if (restoredClip.type() == mlt_service_tractor_type) { // transition
                    restoredClip.set("mlt_type", "mlt_producer");
                } else {
                    fixTransitions(playlist, currentIndex, restoredClip);
                }
                playlist.insert(restoredClip, currentIndex, info.frame_in, info.frame_out);
            }
            m_model.endInsertRows();

            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
            // Q_ASSERT here aborted release builds after a failed insert.
            if (!clip || !clip->is_valid()) {
                failUndo(QStringLiteral("Restored clip is missing at %1 %2")
                             .arg(currentIndex)
                             .arg(uid.toString()));
                continue;
            }
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
            QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);

            if (!info.isBlank && !info.xml.isEmpty()) {
                // Restore the clip fully from its stored XML (preserves deleted keyframes).
                UNDOLOG << "restoring clip from xml at" << currentIndex;
                Mlt::Producer restoredClip(MLT.profile(),
                                           "xml-string",
                                           info.xml.toUtf8().constData());
                if (!restoredClip.is_valid()) {
                    failUndo(QStringLiteral("Failed to parse clip XML for in/out restore %1")
                                 .arg(uid.toString()));
                    continue;
                }
                m_model.beginRemoveRows(modelIndex.parent(), currentIndex, currentIndex);
                playlist.remove(currentIndex);
                m_model.endRemoveRows();

                m_model.beginInsertRows(modelIndex.parent(), currentIndex, currentIndex);
                if (restoredClip.type() == mlt_service_tractor_type)
                    restoredClip.set("mlt_type", "mlt_producer");
                else
                    fixTransitions(playlist, currentIndex, restoredClip);
                playlist.insert(restoredClip, currentIndex, info.frame_in, info.frame_out);
                m_model.endInsertRows();

                // Re-apply timeline-specific properties that are not encoded in the producer XML.
                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
                if (clip && clip->is_valid()) {
                    // Restore UUID on the parent producer for this non-blank clip.
                    MLT.setUuid(clip->parent(), uid);
                    // Restore grouping metadata on the clip, if any was recorded.
                    if (info.group >= 0) {
                        clip->set(kShotcutGroupProperty, info.group);
                    }
                    AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
                }
            } else {
                int filterIn = MLT.filterIn(playlist, currentIndex);
                int filterOut = MLT.filterOut(playlist, currentIndex);

                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
                if (clip && clip->is_valid()) {
                    UNDOLOG << "resizing clip at" << currentIndex << "in" << info.frame_in << "out"
                            << info.frame_out;
                    if (clip->parent().get_data("mlt_mix"))
                        clip->parent().set("mlt_mix", nullptr, 0);
                    if (clip->get_data("mix_in"))
                        clip->set("mix_in", nullptr, 0);
                    if (clip->get_data("mix_out"))
                        clip->set("mix_out", nullptr, 0);
                    playlist.resize_clip(currentIndex, info.frame_in, info.frame_out);
                    MLT.adjustClipFilters(clip->parent(),
                                          filterIn,
                                          filterOut,
                                          info.in_delta,
                                          info.out_delta,
                                          info.in_delta);
                }

                QVector<int> roles;
                roles << MultitrackModel::InPointRole;
                roles << MultitrackModel::OutPointRole;
                roles << MultitrackModel::DurationRole;
                emit m_model.dataChanged(modelIndex, modelIndex, roles);
                if (!info.isBlank) {
                    QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
                    if (clip && clip->is_valid())
                        AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
                }
            }
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
            if (!clip || !clip->is_valid())
                continue;
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
    if (m_undoFailed)
        MAIN.showStatusMessage(
            QObject::tr("Undo incomplete. Some timeline changes could not be restored."));
}

void UndoHelper::setHints(OptimizationHints hints)
{
    m_hints = hints;
}

void UndoHelper::storeXmlForClip(const QUuid &uid)
{
    m_xmlClips.insert(uid);
}

void UndoHelper::restrictToTrack(int trackIndex)
{
    if (trackIndex >= 0) {
        m_hasRestriction = true;
        m_restrictedTracks.insert(trackIndex);
    }
}

void UndoHelper::restrictToTracks(const QSet<int> &tracks)
{
    m_hasRestriction = true;
    m_restrictedTracks = tracks;
}

void UndoHelper::clearRestriction()
{
    m_hasRestriction = false;
    m_restrictedTracks.clear();
}

void UndoHelper::debugPrintState(const QString &title)
{
    LOG_DEBUG() << "timeline state:" << title << "{";
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QString trackStr = QStringLiteral("   track %1 (mlt-idx %2):").arg(i).arg(mltIndex);
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            Mlt::ClipInfo info;
            playlist.clip_info(j, &info);
            QUuid uid = MLT.uuid(*info.producer);
            if (info.producer->is_blank() && info.cut) {
                uid = MLT.uuid(*info.cut);
            }
            trackStr += QStringLiteral(" [ %5 %1 -> %2 (%3 frames) %4]")
                            .arg(info.frame_in)
                            .arg(info.frame_out)
                            .arg(info.frame_count)
                            .arg(info.cut->is_blank() ? "blank " : "clip")
                            .arg(uid.toString());
        }
        LOG_DEBUG() << qPrintable(trackStr);
    }
    LOG_DEBUG() << "}";
}

void UndoHelper::restoreAffectedTracks()
{
    struct Planned
    {
        QUuid uid;
        Info info;
        Mlt::Producer clip;
    };
    QList<Planned> planned;
    for (const auto &uid : std::as_const(m_insertedOrder)) {
        const Info &info = m_state[uid];
        if (!m_affectedTracks.contains(info.oldTrackIndex))
            continue;
        if (info.oldTrackIndex < 0 || info.oldTrackIndex >= m_model.trackList().count()) {
            failUndo(QStringLiteral("Invalid track index while restoring %1 %2")
                         .arg(uid.toString())
                         .arg(info.oldTrackIndex));
            return;
        }
        Planned item;
        item.uid = uid;
        item.info = info;
        if (!info.isBlank) {
            if (info.xml.isEmpty()) {
                failUndo(
                    QStringLiteral("Cannot restore clip without stored XML %1").arg(uid.toString()));
                return;
            }
            item.clip = Mlt::Producer(MLT.profile(), "xml-string", info.xml.toUtf8().constData());
            if (!item.clip.is_valid()) {
                failUndo(QStringLiteral("Failed to parse clip XML %1").arg(uid.toString()));
                return;
            }
        }
        planned.append(item);
    }
    if (m_undoFailed)
        return;

    // Remove everything in the affected tracks.
    for (const auto &trackIndex : std::as_const(m_affectedTracks)) {
        if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
            auto mlt_index = m_model.trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> producer(m_model.tractor()->track(mlt_index));
            if (producer && producer->is_valid()) {
                Mlt::Playlist playlist(*producer.data());
                if (playlist.count() > 0) {
                    m_model.beginRemoveRows(m_model.index(trackIndex), 0, playlist.count() - 1);
                    UNDOLOG << "clearing track" << trackIndex;
                    playlist.clear();
                    m_model.endRemoveRows();
                }
            }
        }
    }

    for (auto &item : planned) {
        const Info &info = item.info;
        const QUuid &uid = item.uid;
        UNDOLOG << "Handling uid" << uid << "on track" << info.oldTrackIndex << "index"
                << info.oldClipIndex;
        int mltIndex = m_model.trackList()[info.oldTrackIndex].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        if (!trackProducer || !trackProducer->is_valid()) {
            failUndo(
                QStringLiteral("Invalid track producer while restoring %1").arg(uid.toString()));
            continue;
        }
        Mlt::Playlist playlist(*trackProducer);
        auto currentIndex = playlist.count();
        QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
        m_model.beginInsertRows(modelIndex.parent(), currentIndex, currentIndex);
        if (info.isBlank) {
            playlist.blank(info.frame_out - info.frame_in);
            UNDOLOG << "appending blank at" << currentIndex << info.frame_out << info.frame_in;
        } else {
            UNDOLOG << "appending clip at" << currentIndex;
            if (item.clip.type() == mlt_service_tractor_type) { // transition
                item.clip.set("mlt_type", "mlt_producer");
            }
            playlist.append(item.clip, info.frame_in, info.frame_out);
            if (info.group >= 0) {
                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
                if (clip && clip->is_valid())
                    clip->set(kShotcutGroupProperty, info.group);
            }
        }
        m_model.endInsertRows();

        QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
        if (!clip || !clip->is_valid()) {
            failUndo(QStringLiteral("Restored clip is missing at %1 %2")
                         .arg(currentIndex)
                         .arg(uid.toString()));
            continue;
        }
        if (info.isBlank) {
            MLT.setUuid(*clip, uid);
        } else {
            MLT.setUuid(clip->parent(), uid);
        }
        AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
    }
    for (const auto &trackIndex : std::as_const(m_affectedTracks)) {
        if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
            auto mlt_index = m_model.trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> producer(m_model.tractor()->track(mlt_index));
            if (producer && producer->is_valid()) {
                Mlt::Playlist playlist(*producer.data());
                for (auto currentIndex = 0; currentIndex < playlist.count(); currentIndex++) {
                    // Own the get_clip() pointer; skip if that cut is gone.
                    QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
                    if (clip && clip->is_valid())
                        fixTransitions(playlist, currentIndex, *clip);
                }
            }
        }
    }
}

// overwrite()/lift clear mix_in/mix_out/mlt_mix. Those are live pointers,
// not XML, so a restored mix tractor still plays as a hard cut until we
// point the neighbors and the mix at each other again (same graph
// mlt_playlist_mix writes).
static void relinkMixReferences(Mlt::Playlist &playlist, int clipIndex, Mlt::Producer &clip)
{
    Mlt::Producer parent = clip.parent();
    if (!parent.get(kShotcutTransitionProperty))
        return;
    Mlt::Tractor tractor(parent);
    if (!tractor.is_valid())
        return;
    mlt_tractor mix = tractor.get_tractor();
    parent.set("mlt_mix", mix, 0);
    if (clipIndex > 0 && !playlist.is_blank(clipIndex - 1)) {
        QScopedPointer<Mlt::Producer> left(playlist.get_clip(clipIndex - 1));
        if (left && left->is_valid() && !left->parent().get(kShotcutTransitionProperty)) {
            left->set("mix_out", mix, 0);
            parent.set("mix_in", left->get_producer(), 0);
        }
    }
    if (clipIndex + 1 < playlist.count() && !playlist.is_blank(clipIndex + 1)) {
        QScopedPointer<Mlt::Producer> right(playlist.get_clip(clipIndex + 1));
        if (right && right->is_valid() && !right->parent().get(kShotcutTransitionProperty)) {
            right->set("mix_in", mix, 0);
            parent.set("mix_out", right->get_producer(), 0);
        }
    }
}

void UndoHelper::fixTransitions(Mlt::Playlist playlist, int clipIndex, Mlt::Producer clip)
{
    if (clip.is_blank()) {
        return;
    }
    int transitionIndex = 0;
    for (auto currentIndex : {clipIndex + 1, clipIndex - 1}) {
        // Connect a transition on the right/left to the new producer.
        Mlt::Producer producer(playlist.get_clip(currentIndex));
        if (producer.is_valid() && producer.parent().get(kShotcutTransitionProperty)) {
            Mlt::Tractor transition(producer.parent());
            if (transition.is_valid()) {
                QScopedPointer<Mlt::Producer> transitionClip(transition.track(transitionIndex));
                if (transitionClip->is_valid()
                    && transitionClip->parent().get_service() != clip.parent().get_service()) {
                    UNDOLOG << "Fixing transition at clip index" << currentIndex
                            << "transition index" << transitionIndex;
                    transitionClip.reset(
                        clip.cut(transitionClip->get_in(), transitionClip->get_out()));
                    transition.set_track(*transitionClip.data(), transitionIndex);
                }
            }
        }
        transitionIndex++;
    }
    relinkMixReferences(playlist, clipIndex, clip);
}
