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

#ifndef UNDOHELPER_H
#define UNDOHELPER_H

#include "models/multitrackmodel.h"

#include <MltPlaylist.h>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>

// Records the state of the affected tracks before a timeline edit and reverts them on undo.
// Two strategies, selected per command via setHints():
//
//  - RestoreTracks: each affected track's playlist XML is snapshotted whole and, on undo, the
//    live track is cleared and rebuilt verbatim from it. Clip content, in/out points, blanks,
//    groups, and transition wiring all restore together and always consistently. Use for
//    commands that mutate clip content in place, move clips across tracks, or otherwise cannot
//    be expressed as a per-clip diff.
//
//  - NoHints / SkipXML: a per-clip diff (by uuid) is replayed surgically, restoring only the
//    clips that actually moved, resized, were added, or were removed. This avoids clearing and
//    re-inserting an entire track (O(clips)) for a single-clip edit. Under NoHints the track
//    snapshot is also taken, so a removed or content-changed clip is rebuilt from it; under
//    SkipXML no snapshot is taken and the command promises its undo needs only in/out resizes
//    and moves (no clip rebuild). SkipXML and RestoreTracks are mutually exclusive.
//
// The fine-grained path cannot detect a pure in-place content change (same uuid, index, and
// in/out): any command that mutates clip content without a structural change must use
// RestoreTracks, or its undo will silently do nothing.
class UndoHelper
{
public:
    // Mutually exclusive; compare with ==, never bitwise.
    enum OptimizationHints { NoHints, SkipXML, RestoreTracks };

    UndoHelper(MultitrackModel &model);

    // trackScope: the set of track indexes that this command can possibly affect. When
    // non-empty, capture is restricted to just these tracks (and none are excluded as
    // locked, since the caller is asserting it will touch them), which avoids the
    // O(total clips in project) cost of scanning every track for commands that are known
    // in advance to touch only a few of them. Leave empty (the default) to fall back to
    // scanning every unlocked track, e.g. when the affected tracks are not known until
    // after the command has partially run (see MoveClipCommand). Locked tracks are
    // skipped in that fallback because every ripple path in MultitrackModel already
    // refuses to mutate a locked track.
    void recordBeforeState(const QSet<int> &trackScope = QSet<int>());
    void recordAfterState();
    void undoChanges();
    void setHints(OptimizationHints hints);

    // Capture this clip's own XML in recordBeforeState so it can be restored from that small
    // snapshot on undo, instead of parsing the whole track's snapshot to reach one clip. Call
    // before recordBeforeState() for each clip a command removes or rewrites in place.
    void storeXmlForClip(const QUuid &uid);

private:
    void debugPrintState(const QString &title);
    bool survivingClipsInOrder() const;
    void restoreAffectedTracks();
    void fixTransitions(Mlt::Playlist playlist, int clipIndex, Mlt::Producer clip);
    void promoteUuids(Mlt::Playlist &playlist);
    void demoteUuids(Mlt::Playlist &playlist);

    enum ChangeFlags {
        NoChange = 0x0,
        ClipInfoModified = 0x1,
        Moved = 0x2,
        Removed = 0x4,
    };

    struct Info
    {
        int oldTrackIndex = -1;
        int oldClipIndex = -1;
        int newTrackIndex = -1;
        int newClipIndex = -1;
        bool isBlank = false;
        int frame_in = -1;
        int frame_out = -1;
        int in_delta = 0;
        int out_delta = 0;
        int group = -1;
        int changes = NoChange;
    };

    QMap<int, QString> m_beforeXml;
    QMap<QUuid, Info> m_state;
    QList<QUuid> m_insertedOrder;
    QList<QUuid> m_clipsAdded;
    QSet<QUuid> m_xmlClips;
    QMap<QUuid, QString> m_clipXml;
    QSet<int> m_affectedTracks;
    QSet<int> m_scannedTracks;
    QSet<int> m_trackScope;
    MultitrackModel &m_model;
    OptimizationHints m_hints = NoHints;
};

#endif // UNDOHELPER_H
