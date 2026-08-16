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

class UndoHelper
{
public:
    // Flags so SkipXML and RestoreTracks can be combined if needed.
    enum OptimizationHints { NoHints = 0x0, SkipXML = 0x1, RestoreTracks = 0x2 };
    UndoHelper(MultitrackModel &model);

    void recordBeforeState();
    void recordAfterState();
    void undoChanges();
    void setHints(OptimizationHints hints);
    void storeXmlForClip(const QUuid &uid);
    // Limit snapshot/restore to these timeline track indexes. Empty means all tracks.
    void restrictToTrack(int trackIndex);
    void restrictToTracks(const QSet<int> &tracks);
    QSet<int> affectedTracks() const { return m_affectedTracks; }

private:
    void debugPrintState(const QString &title);
    void restoreAffectedTracks();
    void fixTransitions(Mlt::Playlist playlist, int clipIndex, Mlt::Producer clip);

    // XMLModified was unused; keep bits as flags for the remaining kinds.
    enum ChangeFlags { NoChange = 0x0, ClipInfoModified = 0x1, Moved = 0x2, Removed = 0x4 };

    struct Info
    {
        int oldTrackIndex;
        int oldClipIndex;
        int newTrackIndex;
        int newClipIndex;
        bool isBlank;
        QString xml;
        int frame_in;
        int frame_out;
        int in_delta;
        int out_delta;
        int group;

        int changes;
        Info()
            : oldTrackIndex(-1)
            , oldClipIndex(-1)
            , newTrackIndex(-1)
            , newClipIndex(-1)
            , isBlank(false)
            , frame_in(-1)
            , frame_out(-1)
            , in_delta(0)
            , out_delta(0)
            , changes(NoChange)
            , group(-1)
        {}
    };
    QMap<QUuid, Info> m_state;
    QList<QUuid> m_clipsAdded;
    QList<QUuid> m_insertedOrder;
    QSet<int> m_affectedTracks;
    QSet<int> m_restrictedTracks;
    QSet<QUuid> m_xmlClips;
    MultitrackModel &m_model;
    OptimizationHints m_hints;

    bool includeTrack(int trackIndex) const;
};

#endif // UNDOHELPER_H
