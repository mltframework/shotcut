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
#include <QMap>
#include <QSet>
#include <QString>

// Captures the XML of the affected tracks' playlists before a timeline edit, and can restore
// them verbatim on undo. Each affected track is snapshotted/restored as a whole (not clip by
// clip), so clip content, in/out points, blanks, groups, and transition wiring are all restored
// together and always consistently, without needing any special-case patch-up code.
class UndoHelper
{
public:
    UndoHelper(MultitrackModel &model);

    // trackScope: the set of track indexes that this command can possibly affect. When
    // non-empty, before/after capture and diffing are restricted to just these tracks,
    // which avoids the O(total clips in project) cost of scanning every track for
    // commands that are known in advance to touch only a few of them. Leave empty (the
    // default) to fall back to scanning every track, e.g. when the affected tracks are
    // not known until after the command has partially run (see MoveClipCommand).
    void recordBeforeState(const QSet<int> &trackScope = QSet<int>());
    void recordAfterState();
    void undoChanges();
    QSet<int> affectedTracks() const { return m_affectedTracks; }

private:
    void debugPrintState(const QString &title);
    void promoteUuids(Mlt::Playlist &playlist);
    void demoteUuids(Mlt::Playlist &playlist);

    QMap<int, QString> m_beforeXml;
    QSet<int> m_affectedTracks;
    QSet<int> m_trackScope;
    MultitrackModel &m_model;
};

#endif // UNDOHELPER_H
