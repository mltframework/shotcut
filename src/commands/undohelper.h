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
#include <QFlags>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>

class UndoHelper
{
public:
    enum OptimizationHint { NoHints = 0x0, SkipXML = 0x1, RestoreTracks = 0x2 };
    Q_DECLARE_FLAGS(OptimizationHints, OptimizationHint)

    UndoHelper(MultitrackModel &model);

    void recordBeforeState();
    void recordAfterState();
    void undoChanges();
    void setHints(OptimizationHints hints);
    void storeXmlForClip(const QUuid &uid);
    // Limit snapshot/restore to these timeline track indexes.
    // restrictToTracks({}) is an empty restriction (no tracks), not "all tracks".
    void restrictToTrack(int trackIndex);
    void restrictToTracks(const QSet<int> &tracks);
    void clearRestriction();
    QSet<int> affectedTracks() const { return m_affectedTracks; }
    // Reconnect mix tractor tracks and mix_in/mix_out/mlt_mix after a cut was replaced.
    static void fixTransitions(Mlt::Playlist playlist, int clipIndex, Mlt::Producer clip);

private:
    void debugPrintState(const QString &title);
    void restoreAffectedTracks();

    // In-place producer edits (no in/out/move) are not detected. Commands that
    // mutate filters/XML without replacing the producer must storeXmlForClip()
    // or avoid SkipXML so the before-state XML can restore them.
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
    bool m_hasRestriction;
    bool m_undoFailed;

    bool includeTrack(int trackIndex) const;
    void failUndo(const QString &detail);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(UndoHelper::OptimizationHints)

#endif // UNDOHELPER_H
