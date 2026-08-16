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
#include "mltcontroller.h"
#include "models/audiolevelstask.h"
#include "shotcut_mlt_properties.h"

#include <QScopedPointer>
#include <QUuid>

#include <memory>

// #define UNDOHELPER_DEBUG
#ifdef UNDOHELPER_DEBUG
#define UNDOLOG LOG_DEBUG()
#else
#define UNDOLOG \
    if (false) \
    LOG_DEBUG()
#endif

namespace {
// Lazily parses each affected track's "before" snapshot into a self-contained shadow playlist,
// once, so restored clip content can be pulled from it by original index. Transitions among a
// shadow's entries are wired to each other, so an entry can be reinserted into the live track
// without patch-up, and a track whose snapshot is never needed is never parsed. Instances are
// meant to be local: the live playlists it holds must not outlive a single undo.
class ShadowTracks
{
public:
    explicit ShadowTracks(const QMap<int, QString> &beforeXml)
        : m_beforeXml(beforeXml)
    {}

    Mlt::Playlist *playlist(int trackIndex)
    {
        auto it = m_shadows.constFind(trackIndex);
        if (it != m_shadows.constEnd())
            return it->playlist.get();
        if (!m_beforeXml.contains(trackIndex))
            return nullptr;
        Shadow s;
        s.producer
            = std::make_shared<Mlt::Producer>(MLT.profile(),
                                              "xml-string",
                                              m_beforeXml.value(trackIndex).toUtf8().constData());
        if (!s.producer->is_valid())
            return nullptr;
        s.playlist = std::make_shared<Mlt::Playlist>(*s.producer);
        m_shadows.insert(trackIndex, s);
        return m_shadows[trackIndex].playlist.get();
    }

private:
    struct Shadow
    {
        std::shared_ptr<Mlt::Producer> producer;
        std::shared_ptr<Mlt::Playlist> playlist;
    };
    const QMap<int, QString> &m_beforeXml;
    QMap<int, Shadow> m_shadows;
};
} // namespace

UndoHelper::UndoHelper(MultitrackModel &model)
    : m_model(model)
{}

void UndoHelper::setHints(OptimizationHints hints)
{
    m_hints = hints;
}

void UndoHelper::recordBeforeState(const QSet<int> &trackScope)
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("Before state");
#endif
    qint64 xmlCallCountBefore = MLT.xmlCallCount();
    m_trackScope = trackScope;
    m_beforeXml.clear();
    m_state.clear();
    m_clipsAdded.clear();
    m_insertedOrder.clear();
    m_affectedTracks.clear();
    m_scannedTracks.clear();
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        if (!m_trackScope.isEmpty() && !m_trackScope.contains(i))
            continue;
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        if (!trackProducer || !trackProducer->is_valid())
            continue;
        // An implicit (empty) scope never snapshots a locked track, since every ripple path
        // in MultitrackModel already refuses to mutate one.
        if (m_trackScope.isEmpty() && trackProducer->get_int(kTrackLockProperty))
            continue;
        m_scannedTracks << i;

        Mlt::Playlist playlist(*trackProducer);
        if (m_hints != SkipXML) {
            promoteUuids(playlist);
            m_beforeXml[i] = MLT.XML(trackProducer.data());
            demoteUuids(playlist);
        }
        // The whole-track restore path works purely from m_beforeXml; only the fine-grained
        // paths need the per-clip diff state.
        if (m_hints != RestoreTracks) {
            for (int j = 0; j < playlist.count(); ++j) {
                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
                QUuid uid = clip->is_blank() ? MLT.ensureHasUuid(*clip)
                                             : MLT.ensureHasUuid(clip->parent());
                m_insertedOrder << uid;
                Info &info = m_state[uid];
                Mlt::ClipInfo clipInfo;
                playlist.clip_info(j, &clipInfo);
                info.frame_in = clipInfo.frame_in;
                info.frame_out = clipInfo.frame_out;
                info.oldTrackIndex = i;
                info.oldClipIndex = j;
                info.isBlank = playlist.is_blank(j);
                if (clipInfo.cut && clipInfo.cut->property_exists(kShotcutGroupProperty))
                    info.group = clipInfo.cut->get_int(kShotcutGroupProperty);
            }
        }
    }
    UNDOLOG << "recordBeforeState() called Controller::XML()"
            << (MLT.xmlCallCount() - xmlCallCountBefore) << "times";
}

void UndoHelper::recordAfterState()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("After state");
#endif
    if (m_hints == RestoreTracks) {
        // An edit can only ever have touched tracks that were captured above, so reuse that set
        // instead of re-serializing and diffing every in-scope track a second time.
        m_affectedTracks = m_scannedTracks;
        return;
    }

    QList<QUuid> clipsRemoved = m_state.keys();
    m_clipsAdded.clear();
    // Only the tracks captured before the edit can have changed; scanning others would
    // misclassify their untouched clips as newly added.
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        if (!m_scannedTracks.contains(i))
            continue;
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        if (!trackProducer || !trackProducer->is_valid())
            continue;
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            QUuid uid = clip->is_blank() ? MLT.ensureHasUuid(*clip)
                                         : MLT.ensureHasUuid(clip->parent());

            /* Clips not previously in m_state are new */
            if (!m_state.contains(uid)) {
                UNDOLOG << "New clip at" << i << j;
                m_clipsAdded << uid;
            } else {
                Info &info = m_state[uid];
                info.changes = NoChange;
                info.newTrackIndex = i;
                info.newClipIndex = j;

                /* Indices have changed; these are moved */
                if (info.oldTrackIndex != info.newTrackIndex
                    || info.oldClipIndex != info.newClipIndex) {
                    UNDOLOG << "Clip" << uid << "moved from" << info.oldTrackIndex
                            << info.oldClipIndex << "to" << info.newTrackIndex << info.newClipIndex;
                    info.changes |= Moved;
                }

                Mlt::ClipInfo newInfo;
                playlist.clip_info(j, &newInfo);
                /* Only in/out point changes are handled at this time. */
                if (info.frame_in != newInfo.frame_in || info.frame_out != newInfo.frame_out) {
                    UNDOLOG << "In/out changed:" << uid;
                    info.changes |= ClipInfoModified;
                    info.in_delta = info.frame_in - newInfo.frame_in;
                    info.out_delta = newInfo.frame_out - info.frame_out;
                }
            }
            clipsRemoved.removeOne(uid);
        }
    }

    /* Clips that did not show up are removed from the timeline */
    foreach (QUuid uid, clipsRemoved) {
        UNDOLOG << "Clip removed:" << uid;
        m_state[uid].changes = Removed;
    }
}

void UndoHelper::undoChanges()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("Before undo");
#endif
    if (m_hints == RestoreTracks) {
        restoreAffectedTracks();
        emit m_model.modified();
#ifdef UNDOHELPER_DEBUG
        debugPrintState("After undo");
#endif
        return;
    }

    // Restored clip content is pulled from each affected track's parsed "before" snapshot on
    // demand; nothing is parsed for a track no clip needs restored on.
    ShadowTracks shadows(m_beforeXml);
    QMap<int, int> indexAdjustment;

    /* We're walking through the list in the order of uids, which is the order in which the
     * clips were laid out originally. As we go through the clips we make sure the clips behind
     * the current index are as they were originally before we move on to the next one */
    foreach (QUuid uid, m_insertedOrder) {
        const Info &info = m_state[uid];
        UNDOLOG << "Handling uid" << uid << "on track" << info.oldTrackIndex << "index"
                << info.oldClipIndex;

        int trackIndex = m_model.trackList()[info.oldTrackIndex].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(trackIndex));
        Mlt::Playlist playlist(*trackProducer);

        /* This is the index in the track we're currently restoring */
        int currentIndex = qMin(info.oldClipIndex + indexAdjustment[trackIndex],
                                playlist.count() - 1);

        /* Clips that were moved are simply searched for using the uid, and moved in place. We
         * do not use the indices directly because they become invalid once the playlist is
         * modified. */
        if (info.changes & Moved) {
            Q_ASSERT(info.newTrackIndex == info.oldTrackIndex
                     && "cross-track moves are unsupported so far");
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

        /* Removed clips are reinserted from the track snapshot */
        if (info.changes & Removed) {
            QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
            m_model.beginInsertRows(modelIndex.parent(), currentIndex, currentIndex);
            if (info.isBlank) {
                playlist.insert_blank(currentIndex, info.frame_out - info.frame_in);
                UNDOLOG << "inserting blank at " << currentIndex;
            } else {
                UNDOLOG << "inserting clip at " << currentIndex << uid;
                Q_ASSERT(m_hints != SkipXML && "Cannot restore clip without a track snapshot");
                Mlt::Playlist *shadow = shadows.playlist(info.oldTrackIndex);
                Q_ASSERT(shadow && "Missing track snapshot for removed clip");
                QScopedPointer<Mlt::Producer> entry(shadow->get_clip(info.oldClipIndex));
                if (entry->type() == mlt_service_tractor_type) { // transition
                    entry->set("mlt_type", "mlt_producer");
                } else {
                    fixTransitions(playlist, currentIndex, *entry);
                }
                playlist.insert(*entry, currentIndex, info.frame_in, info.frame_out);
            }
            m_model.endInsertRows();

            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
            Q_ASSERT(currentIndex < playlist.count());
            Q_ASSERT(!clip.isNull());
            if (info.isBlank) {
                MLT.setUuid(*clip, uid);
                clip->set(kUuidPropertyTemp, nullptr, 0);
            } else {
                MLT.setUuid(clip->parent(), uid);
                clip->parent().set(kUuidPropertyTemp, nullptr, 0);
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
            Mlt::Playlist *shadow = info.isBlank ? nullptr : shadows.playlist(info.oldTrackIndex);

            if (shadow) {
                // Rebuild the clip from the snapshot so keyframes deleted during the edit are
                // restored, not just its in/out points.
                UNDOLOG << "restoring clip from snapshot at" << currentIndex;
                m_model.beginRemoveRows(modelIndex.parent(), currentIndex, currentIndex);
                playlist.remove(currentIndex);
                m_model.endRemoveRows();

                m_model.beginInsertRows(modelIndex.parent(), currentIndex, currentIndex);
                QScopedPointer<Mlt::Producer> entry(shadow->get_clip(info.oldClipIndex));
                if (entry->type() == mlt_service_tractor_type) { // transition
                    entry->set("mlt_type", "mlt_producer");
                } else {
                    fixTransitions(playlist, currentIndex, *entry);
                }
                playlist.insert(*entry, currentIndex, info.frame_in, info.frame_out);
                m_model.endInsertRows();

                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
                if (clip && clip->is_valid()) {
                    MLT.setUuid(clip->parent(), uid);
                    clip->parent().set(kUuidPropertyTemp, nullptr, 0);
                    if (info.group >= 0)
                        clip->set(kShotcutGroupProperty, info.group);
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
                    QScopedPointer<Mlt::Producer> clip2(playlist.get_clip(currentIndex));
                    if (clip2 && clip2->is_valid())
                        AudioLevelsTask::start(clip2->parent(), &m_model, modelIndex);
                }
            }
        }
    }

    /* Finally we walk through the tracks once more, removing clips that were added */
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
    for (const auto &trackIndex : std::as_const(m_affectedTracks)) {
        if (trackIndex < 0 || trackIndex >= m_model.trackList().size()
            || !m_beforeXml.contains(trackIndex))
            continue;
        int mltIndex = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        if (!trackProducer || !trackProducer->is_valid())
            continue;
        Mlt::Playlist playlist(*trackProducer);
        QModelIndex modelIndex = m_model.index(trackIndex);

        // Parse the "before" snapshot into a self-contained shadow playlist so that any
        // transitions among its entries are wired to each other rather than to anything
        // currently on the live track.
        Mlt::Producer restoredTrack(MLT.profile(),
                                    "xml-string",
                                    m_beforeXml.value(trackIndex).toUtf8().constData());
        Mlt::Playlist restoredPlaylist(restoredTrack);

        int oldCount = playlist.count();
        if (oldCount > 0) {
            m_model.beginRemoveRows(modelIndex, 0, oldCount - 1);
            UNDOLOG << "clearing track" << trackIndex;
            playlist.clear();
            m_model.endRemoveRows();
        }

        int newCount = restoredPlaylist.count();
        if (newCount > 0) {
            m_model.beginInsertRows(modelIndex, 0, newCount - 1);
            for (int j = 0; j < newCount; ++j) {
                Mlt::ClipInfo clipInfo;
                restoredPlaylist.clip_info(j, &clipInfo);
                if (restoredPlaylist.is_blank(j)) {
                    playlist.insert_blank(j, clipInfo.frame_out - clipInfo.frame_in);
                } else {
                    QScopedPointer<Mlt::Producer> entry(restoredPlaylist.get_clip(j));
                    playlist.insert(*entry, j, clipInfo.frame_in, clipInfo.frame_out);
                }
            }
            m_model.endInsertRows();
        }

        // Restore uuids that were demoted to a temporary serializable property before the
        // snapshot was taken, and kick off audio levels rebuilds for the restored clips.
        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            if (!clip || !clip->is_valid())
                continue;
            if (playlist.is_blank(j)) {
                if (clip->get(kUuidPropertyTemp)) {
                    clip->set(kUuidProperty, clip->get(kUuidPropertyTemp));
                    clip->set(kUuidPropertyTemp, nullptr, 0);
                }
            } else {
                Mlt::Producer &parent = clip->parent();
                if (parent.get(kUuidPropertyTemp)) {
                    parent.set(kUuidProperty, parent.get(kUuidPropertyTemp));
                    parent.set(kUuidPropertyTemp, nullptr, 0);
                }
                AudioLevelsTask::start(parent, &m_model, m_model.createIndex(j, 0, trackIndex));
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
    for (auto currentIndex : {clipIndex + 1, clipIndex - 1}) {
        // Reconnect a transition on the right/left to the freshly restored producer, which is a
        // new instance the adjacent transition's track slot does not yet reference.
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
}

void UndoHelper::promoteUuids(Mlt::Playlist &playlist)
{
    // Copy the private, non-serialized uuid property to a temporary serializable property so
    // that it survives a round trip through MLT.XML().
    for (int j = 0; j < playlist.count(); ++j) {
        QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
        if (!clip || !clip->is_valid())
            continue;
        if (playlist.is_blank(j)) {
            QUuid uid = MLT.ensureHasUuid(*clip);
            clip->set(kUuidPropertyTemp, uid.toString().toUtf8().constData());
        } else {
            Mlt::Producer &parent = clip->parent();
            QUuid uid = MLT.ensureHasUuid(parent);
            parent.set(kUuidPropertyTemp, uid.toString().toUtf8().constData());
        }
    }
}

void UndoHelper::demoteUuids(Mlt::Playlist &playlist)
{
    // Clear the temporary serializable uuid property from the live objects so that it is
    // never left behind on anything other than a stored snapshot.
    for (int j = 0; j < playlist.count(); ++j) {
        QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
        if (!clip || !clip->is_valid())
            continue;
        if (playlist.is_blank(j)) {
            clip->set(kUuidPropertyTemp, nullptr, 0);
        } else {
            clip->parent().set(kUuidPropertyTemp, nullptr, 0);
        }
    }
}
