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
#include "dialogs/longuitask.h"
#include "mltcontroller.h"
#include "models/audiolevelstask.h"
#include "shotcut_mlt_properties.h"

#include <QScopedPointer>
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>

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

void UndoHelper::storeXmlForClip(const QUuid &uid)
{
    m_xmlClips.insert(uid);
}

void UndoHelper::recordBeforeState(const QSet<int> &trackScope)
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("Before state");
#endif
    m_trackScope = trackScope;
    m_beforeXml.clear();
    m_state.clear();
    m_clipsAdded.clear();
    m_insertedOrder.clear();
    m_clipXml.clear();
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
                // Snapshot just this clip so undo can restore it without parsing the whole track.
                if (!info.isBlank && m_xmlClips.contains(uid))
                    m_clipXml[uid] = MLT.XML(&clip->parent());
            }
        }
    }
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

    // A removed or rewritten clip is restored from its own small snapshot when one was captured,
    // which avoids parsing the whole track's snapshot just to reach a single clip.
    auto clipProducer = [&](const QUuid &uid, const Info &info) -> Mlt::Producer * {
        auto it = m_clipXml.constFind(uid);
        if (it != m_clipXml.constEnd())
            return new Mlt::Producer(MLT.profile(), "xml-string", it->toUtf8().constData());
        Mlt::Playlist *shadow = shadows.playlist(info.oldTrackIndex);
        return shadow ? shadow->get_clip(info.oldClipIndex) : nullptr;
    };

    // Undo structural edits by reversing them rather than by moving clips. First drop every clip
    // the edit added, which slides the surviving clips back toward their original positions, then
    // walk the original layout order reinserting removed clips and restoring modified ones in
    // place. The commands that use this path never genuinely reorder clips (real moves use
    // RestoreTracks), so each clip's index is fully determined by the reinsertions before it.

    // Verify that assumption before touching anything: the surviving (non-added) real clips must
    // still be in their original relative order. If not (some structural edits, or a preceding
    // whole-track restore, can break it), rebuild the affected tracks from the snapshot instead,
    // which is always correct.
    if (m_hints != SkipXML && !survivingClipsInOrder()) {
        LOG_WARNING() << "UndoHelper: clip order changed, using whole-track restore";
        m_affectedTracks = m_scannedTracks;
        restoreAffectedTracks();
        emit m_model.modified();
        return;
    }

    // 1. Remove the clips the edit added.
    for (const auto &i : std::as_const(m_scannedTracks)) {
        if (i < 0 || i >= m_model.trackList().size())
            continue;
        QScopedPointer<Mlt::Producer> trackProducer(
            m_model.tractor()->track(m_model.trackList()[i].mlt_index));
        if (!trackProducer || !trackProducer->is_valid())
            continue;
        Mlt::Playlist playlist(*trackProducer);
        for (int j = playlist.count() - 1; j >= 0; --j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            if (!clip || !clip->is_valid())
                continue;
            QUuid uid = clip->is_blank() ? MLT.uuid(*clip) : MLT.uuid(clip->parent());
            if (m_clipsAdded.contains(uid)) {
                UNDOLOG << "Removing added clip at" << j;
                if (clip->parent().get_data("mlt_mix"))
                    clip->parent().set("mlt_mix", NULL, 0);
                if (clip->get_data("mix_in"))
                    clip->set("mix_in", NULL, 0);
                if (clip->get_data("mix_out"))
                    clip->set("mix_out", NULL, 0);
                m_model.beginRemoveRows(m_model.index(i), j, j);
                playlist.remove(j);
                m_model.endRemoveRows();
            }
        }
    }

    // 2. Reinsert removed clips and restore modified ones, in the original layout order.
    QMap<int, int> nextIndex;
    foreach (QUuid uid, m_insertedOrder) {
        const Info &info = m_state[uid];
        int mltIndex = m_model.trackList()[info.oldTrackIndex].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);
        int currentIndex = nextIndex[info.oldTrackIndex];
        QModelIndex parentIndex = m_model.index(info.oldTrackIndex);
        QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);

        if (info.changes & Removed) {
            if (info.isBlank) {
                m_model.beginInsertRows(parentIndex, currentIndex, currentIndex);
                playlist.insert_blank(currentIndex, info.frame_out - info.frame_in);
                m_model.endInsertRows();
                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
                if (clip && clip->is_valid()) {
                    MLT.setUuid(*clip, uid);
                    clip->set(kUuidPropertyTemp, nullptr, 0);
                }
            } else {
                QScopedPointer<Mlt::Producer> entry(clipProducer(uid, info));
                insertRestoredClip(playlist, currentIndex, uid, info, entry.data());
            }
        } else if (info.changes & ClipInfoModified) {
            // Genuine reordering is unsupported here; the clip must already be at currentIndex.
            bool rebuild = !info.isBlank && (m_clipXml.contains(uid) || m_hints != SkipXML);
            if (rebuild) {
                // Rebuild the clip from its snapshot so keyframes and filters removed during the
                // edit are restored, not just its in/out points.
                m_model.beginRemoveRows(parentIndex, currentIndex, currentIndex);
                playlist.remove(currentIndex);
                m_model.endRemoveRows();

                QScopedPointer<Mlt::Producer> entry(clipProducer(uid, info));
                insertRestoredClip(playlist, currentIndex, uid, info, entry.data());
            } else {
                int filterIn = MLT.filterIn(playlist, currentIndex);
                int filterOut = MLT.filterOut(playlist, currentIndex);
                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
                if (clip && clip->is_valid()) {
                    if (clip->parent().get_data("mlt_mix"))
                        clip->parent().set("mlt_mix", nullptr, 0);
                    if (clip->get_data("mix_in"))
                        clip->set("mix_in", nullptr, 0);
                    if (clip->get_data("mix_out"))
                        clip->set("mix_out", nullptr, 0);
                    playlist.resize_clip(currentIndex, info.frame_in, info.frame_out);
                    if (!info.isBlank)
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
        nextIndex[info.oldTrackIndex] = currentIndex + 1;
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

// True if every surviving (non-added) real clip is still in its original relative order, which the
// fine-grained undo path requires; blanks are ignored because their identity churns as they merge.
bool UndoHelper::survivingClipsInOrder() const
{
    QMap<int, QList<QUuid>> expected;
    for (const QUuid &uid : std::as_const(m_insertedOrder)) {
        const Info &info = m_state[uid];
        if (!(info.changes & Removed) && !info.isBlank)
            expected[info.oldTrackIndex] << uid;
    }
    for (const auto &i : std::as_const(m_scannedTracks)) {
        if (i < 0 || i >= m_model.trackList().size())
            continue;
        QScopedPointer<Mlt::Producer> trackProducer(
            m_model.tractor()->track(m_model.trackList()[i].mlt_index));
        if (!trackProducer || !trackProducer->is_valid())
            continue;
        Mlt::Playlist playlist(*trackProducer);
        QList<QUuid> live;
        for (int j = 0; j < playlist.count(); ++j) {
            if (playlist.is_blank(j))
                continue;
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            if (!clip || !clip->is_valid())
                continue;
            QUuid uid = MLT.uuid(clip->parent());
            if (!m_clipsAdded.contains(uid))
                live << uid;
        }
        if (live != expected.value(i))
            return false;
    }
    return true;
}

void UndoHelper::restoreAffectedTracks()
{
    LongUiTask longTask(QObject::tr("Undo"));
    longTask.setMinimumDuration(1000);

    // Only parsing each track's "before" XML snapshot is worth moving off the GUI thread; the
    // model itself must be mutated on its own (GUI) thread since QAbstractItemModel row-change
    // notifications are not thread-safe and views expect them on their affinity thread.
    using ShadowPtr = std::shared_ptr<Mlt::Producer>;
    QSet<int> affectedTracks = m_affectedTracks;
    QMap<int, QString> beforeXml = m_beforeXml;
    QFuture<QMap<int, ShadowPtr>> future = QtConcurrent::run([affectedTracks, beforeXml]() {
        QMap<int, ShadowPtr> shadows;
        for (const auto &trackIndex : std::as_const(affectedTracks)) {
            if (!beforeXml.contains(trackIndex))
                continue;
            auto producer
                = std::make_shared<Mlt::Producer>(MLT.profile(),
                                                  "xml-string",
                                                  beforeXml.value(trackIndex).toUtf8().constData());
            if (producer->is_valid())
                shadows.insert(trackIndex, producer);
        }
        return shadows;
    });
    QMap<int, ShadowPtr> shadows
        = longTask.wait<QMap<int, ShadowPtr>>(QObject::tr("Undo %1").arg(m_text), future);

    for (const auto &trackIndex : std::as_const(m_affectedTracks)) {
        if (trackIndex < 0 || trackIndex >= m_model.trackList().size())
            continue;
        // Parsed into a self-contained shadow playlist so that any transitions among its
        // entries are wired to each other rather than to anything currently on the live track.
        auto shadowIt = shadows.constFind(trackIndex);
        if (shadowIt == shadows.constEnd())
            continue;
        int mltIndex = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        if (!trackProducer || !trackProducer->is_valid())
            continue;
        Mlt::Playlist playlist(*trackProducer);
        QModelIndex modelIndex = m_model.index(trackIndex);
        Mlt::Playlist restoredPlaylist(*shadowIt.value());

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
                    // insert_blank() makes a fresh blank, dropping the shadow blank's uuid; carry
                    // it over so a later command's undo can still recognize this blank.
                    QScopedPointer<Mlt::Producer> shadowBlank(restoredPlaylist.get_clip(j));
                    QScopedPointer<Mlt::Producer> newBlank(playlist.get_clip(j));
                    if (shadowBlank && newBlank && shadowBlank->get(kUuidPropertyTemp))
                        newBlank->set(kUuidPropertyTemp, shadowBlank->get(kUuidPropertyTemp));
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

void UndoHelper::insertRestoredClip(Mlt::Playlist &playlist,
                                    int clipIndex,
                                    const QUuid &uid,
                                    const Info &info,
                                    Mlt::Producer *entry)
{
    Q_ASSERT(entry && "Missing snapshot for restored clip");
    if (!entry)
        return;

    QModelIndex parentIndex = m_model.index(info.oldTrackIndex);
    m_model.beginInsertRows(parentIndex, clipIndex, clipIndex);
    if (entry->type() == mlt_service_tractor_type) // transition
        entry->set("mlt_type", "mlt_producer");
    playlist.insert(*entry, clipIndex, info.frame_in, info.frame_out);
    m_model.endInsertRows();

    QScopedPointer<Mlt::Producer> clip(playlist.get_clip(clipIndex));
    if (!clip || !clip->is_valid())
        return;
    m_model.relinkTransitions(info.oldTrackIndex, clipIndex);
    MLT.setUuid(clip->parent(), uid);
    clip->parent().set(kUuidPropertyTemp, nullptr, 0);
    m_model.setClipGroup(info.oldTrackIndex, playlist.clip_start(clipIndex), info.group);
    AudioLevelsTask::start(clip->parent(),
                           &m_model,
                           m_model.createIndex(clipIndex, 0, info.oldTrackIndex));
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
