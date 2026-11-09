/*
 * Copyright (c) 2026 Meltytech, LLC
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

#ifndef SHOTCUT_API_APITIMELINE_H
#define SHOTCUT_API_APITIMELINE_H

#include "api/apiobject.h"

#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class MultitrackModel;

namespace Shotcut {
namespace Api {

class ApiTimeline : public ApiObject
{
    Q_OBJECT
    Q_CLASSINFO("doc",
                "The multitrack timeline. Track 0 is the top track. Clip indexes count blank gaps "
                "and change after edits; clip ids are stable for the session.")
    Q_CLASSINFO("doc.exists", "Whether the timeline has any tracks.")
    Q_CLASSINFO("doc.trackCount", "Number of tracks.")
    Q_CLASSINFO("doc.duration", "Timeline length in frames.")
    Q_CLASSINFO("doc.position", "Timeline playhead position in frames.")
    Q_CLASSINFO("doc.currentTrack", "Index of the track with keyboard focus.")
    Q_CLASSINFO("doc.selectedTrack", "Index of the selected track header, or -1.")
    Q_PROPERTY(bool exists READ exists NOTIFY changed)
    Q_PROPERTY(int trackCount READ trackCount NOTIFY changed)
    Q_PROPERTY(int duration READ duration NOTIFY changed)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(int currentTrack READ currentTrack NOTIFY currentTrackChanged)
    Q_PROPERTY(int selectedTrack READ selectedTrack NOTIFY selectionChanged)

public:
    explicit ApiTimeline(QObject *parent = nullptr);

    bool exists() const;
    int trackCount() const;
    int duration() const;
    int position() const;
    int currentTrack() const;
    int selectedTrack() const;

    Q_CLASSINFO("mcp.tracks", "read")
    Q_CLASSINFO("doc.tracks",
                "Lists the timeline tracks with name, type (video/audio), duration, clip count, "
                "and mute/hidden/locked state.")
    Q_INVOKABLE QVariantList tracks() const;

    Q_CLASSINFO("mcp.track", "read")
    Q_CLASSINFO("doc.track", "Returns one track.")
    Q_CLASSINFO("arg.track.trackIndex", "Track index; 0 is the top track.")
    Q_INVOKABLE QVariant track(int trackIndex) const;

    Q_CLASSINFO("mcp.clips", "read")
    Q_CLASSINFO("doc.clips",
                "Lists timeline clips ordered by track, then position. Each clip has id, "
                "trackIndex, clipIndex, start, duration, in, out, name, resource, speed, fades, "
                "and more. All times are in frames.")
    Q_CLASSINFO("arg.clips.trackIndex", "integer|Only clips on this track.")
    Q_CLASSINFO("arg.clips.start", "integer|Only clips that end after this timeline frame.")
    Q_CLASSINFO("arg.clips.end", "integer|Only clips that start before this timeline frame.")
    Q_CLASSINFO("arg.clips.includeBlanks", "boolean|Include blank gaps. Defaults to false.")
    Q_CLASSINFO("arg.clips.selectedOnly",
                "boolean|Restrict to currently selected clips. Defaults to false; combines with "
                "track and range criteria.")
    Q_CLASSINFO("arg.clips.includeTransitions",
                "boolean|Include transition clips. Defaults to true for compatibility.")
    Q_INVOKABLE QVariantList clips(const QVariantMap &options = {}) const;

    Q_CLASSINFO("mcp.clipIds", "read")
    Q_CLASSINFO("doc.clipIds",
                "Returns matching clip IDs ordered by track and timeline position. With no "
                "criteria, returns all ordinary clips. Criteria are intersected. Never includes "
                "blanks or transitions. Does not change selection, playhead, or undo history. "
                "Use IDs with existing edit methods; locks are checked by the edits.")
    Q_CLASSINFO("arg.clipIds.trackIndex", "integer|Only clips on this track.")
    Q_CLASSINFO("arg.clipIds.start",
                "integer|Only clips that end after this nonnegative timeline frame.")
    Q_CLASSINFO("arg.clipIds.end",
                "integer|Only clips that start before this nonnegative timeline frame. Must exceed "
                "start when both are supplied.")
    Q_CLASSINFO("arg.clipIds.selectedOnly",
                "boolean|Only currently selected clips. Defaults to false.")
    Q_INVOKABLE QStringList clipIds(const QVariantMap &options = {}) const;

    Q_CLASSINFO("mcp.clip", "read")
    Q_CLASSINFO("doc.clip", "Returns the clip at a track and clip index.")
    Q_CLASSINFO("arg.clip.trackIndex", "Track index.")
    Q_CLASSINFO("arg.clip.clipIndex", "Clip index within the track, counting blank gaps.")
    Q_INVOKABLE QVariant clip(int trackIndex, int clipIndex) const;

    Q_CLASSINFO("mcp.clipAt", "read")
    Q_CLASSINFO("doc.clipAt",
                "Returns the clip or blank gap at a timeline frame on a track, or null if the "
                "frame is past the end of the track.")
    Q_CLASSINFO("arg.clipAt.trackIndex", "Track index.")
    Q_CLASSINFO("arg.clipAt.position", "Timeline frame.")
    Q_INVOKABLE QVariant clipAt(int trackIndex, int position) const;

    Q_CLASSINFO("mcp.findClip", "read")
    Q_CLASSINFO("doc.findClip", "Finds a timeline clip by id and returns its current location.")
    Q_CLASSINFO("arg.findClip.id", "Clip id from a previous clip result.")
    Q_INVOKABLE QVariant findClip(const QString &id) const;

    Q_CLASSINFO("mcp.selection", "read")
    Q_CLASSINFO("doc.selection", "Lists the clips selected in the timeline.")
    Q_INVOKABLE QVariantList selection() const;

    Q_CLASSINFO("mcp.appendClip", "edit")
    Q_CLASSINFO("doc.appendClip",
                "Appends a new independent clip to the end of the specified track and returns "
                "its snapshot. Give exactly one local resource or playlistIndex. Playlist "
                "sources retain their filters and source range unless in/out overrides it. "
                "Does not select or seek the clip, alter the playlist, or shift other tracks "
                "or markers. Import/proxy settings match playlist.addItem.")
    Q_CLASSINFO("arg.appendClip.trackIndex", "integer!|Destination track index; must be unlocked.")
    Q_CLASSINFO("arg.appendClip.resource", "string|Local media file or MLT virtual clip.")
    Q_CLASSINFO("arg.appendClip.playlistIndex", "integer|Source index in the complete playlist.")
    Q_CLASSINFO("arg.appendClip.in", "integer|Inclusive source in frame; defaults to source in.")
    Q_CLASSINFO("arg.appendClip.out", "integer|Inclusive source out frame; defaults to source out.")
    Q_INVOKABLE QVariant appendClip(const QVariantMap &options);

    Q_CLASSINFO("mcp.insertClip", "edit")
    Q_CLASSINFO("doc.insertClip",
                "Inserts a new independent clip at a timeline frame and returns its snapshot. "
                "Splits destination footage if needed and shifts later footage on that track. "
                "Positions beyond the track end create a gap. Give exactly one local resource "
                "or playlistIndex, with optional inclusive source in/out. Other tracks and "
                "markers stay put unless explicitly rippled; locked other tracks are skipped. "
                "Edits that split transitions or clips joined to them are unavailable. "
                "Does not select or seek. Import/proxy settings match playlist.addItem.")
    Q_CLASSINFO("arg.insertClip.trackIndex", "integer!|Destination track index; must be unlocked.")
    Q_CLASSINFO("arg.insertClip.position", "integer!|Nonnegative timeline frame for the new start.")
    Q_CLASSINFO("arg.insertClip.resource", "string|Local media file or MLT virtual clip.")
    Q_CLASSINFO("arg.insertClip.playlistIndex", "integer|Source index in the complete playlist.")
    Q_CLASSINFO("arg.insertClip.in", "integer|Inclusive source in frame; defaults to source in.")
    Q_CLASSINFO("arg.insertClip.out", "integer|Inclusive source out frame; defaults to source out.")
    Q_CLASSINFO("arg.insertClip.rippleAllTracks",
                "boolean|Shift footage on other unlocked tracks; defaults to false.")
    Q_CLASSINFO("arg.insertClip.rippleMarkers",
                "boolean|Shift markers starting at or after position; defaults to false.")
    Q_INVOKABLE QVariant insertClip(const QVariantMap &options);

    Q_CLASSINFO("mcp.overwriteClip", "edit")
    Q_CLASSINFO("doc.overwriteClip",
                "Overwrites a timeline range with a new independent clip and returns its "
                "snapshot. Later footage outside the range stays at its original time. "
                "May split or remove destination clips, extend the track, or create a gap "
                "beyond its end. Give exactly one local resource or playlistIndex, with optional "
                "inclusive source in/out. Overwriting transitions or clips joined to them is "
                "unavailable. Other tracks and markers stay put; does not select or seek. "
                "Import/proxy settings match playlist.addItem.")
    Q_CLASSINFO("arg.overwriteClip.trackIndex",
                "integer!|Destination track index; must be unlocked.")
    Q_CLASSINFO("arg.overwriteClip.position",
                "integer!|Nonnegative timeline frame for the new start.")
    Q_CLASSINFO("arg.overwriteClip.resource", "string|Local media file or MLT virtual clip.")
    Q_CLASSINFO("arg.overwriteClip.playlistIndex", "integer|Source index in the complete playlist.")
    Q_CLASSINFO("arg.overwriteClip.in", "integer|Inclusive source in frame; defaults to source in.")
    Q_CLASSINFO("arg.overwriteClip.out",
                "integer|Inclusive source out frame; defaults to source out.")
    Q_INVOKABLE QVariant overwriteClip(const QVariantMap &options);

    Q_CLASSINFO("mcp.removeClip", "edit")
    Q_CLASSINFO("doc.removeClip",
                "Removes a clip and returns what it was. By default this lifts the clip, leaving "
                "a gap; with ripple the clips after it move left to close the gap. Other tracks "
                "and markers stay put, regardless of GUI ripple settings.")
    Q_CLASSINFO("arg.removeClip.clipId", "Clip id.")
    Q_CLASSINFO("arg.removeClip.ripple", "boolean|Close the gap. Defaults to false.")
    Q_INVOKABLE QVariant removeClip(const QString &clipId, const QVariantMap &options = {});

    Q_CLASSINFO("mcp.splitClip", "edit")
    Q_CLASSINFO("doc.splitClip",
                "Splits a clip at a timeline frame and returns {left, right}. The right part keeps "
                "the clip id; the left part gets a new id.")
    Q_CLASSINFO("arg.splitClip.clipId", "Clip id.")
    Q_CLASSINFO("arg.splitClip.position",
                "Timeline frame where the right part starts; must be inside the clip.")
    Q_INVOKABLE QVariant splitClip(const QString &clipId, int position);

    Q_CLASSINFO("mcp.moveClip", "edit")
    Q_CLASSINFO("doc.moveClip",
                "Moves a clip to another position or track of the same type and returns it. "
                "Without ripple, the destination must be empty (a gap) apart from the clip itself. "
                "Ripple affects only the source and destination tracks; other tracks and markers "
                "stay put, regardless of GUI ripple settings.")
    Q_CLASSINFO("arg.moveClip.clipId", "Clip id.")
    Q_CLASSINFO("arg.moveClip.position", "integer|New start frame; defaults to the current start.")
    Q_CLASSINFO("arg.moveClip.trackIndex", "integer|Destination track; defaults to the current one.")
    Q_CLASSINFO("arg.moveClip.ripple",
                "boolean|Close the gap left behind and push later clips at the destination. "
                "Defaults to false.")
    Q_INVOKABLE QVariant moveClip(const QString &clipId, const QVariantMap &options);

    Q_CLASSINFO("mcp.trimClip", "edit")
    Q_CLASSINFO("doc.trimClip",
                "Shortens or lengthens a clip at either end and returns it. Lengthening needs "
                "unused source media and, without ripple, a gap next to the clip. Other tracks "
                "and markers stay put, regardless of GUI ripple settings.")
    Q_CLASSINFO("arg.trimClip.clipId", "Clip id.")
    Q_CLASSINFO("arg.trimClip.start",
                "integer|Frames to remove from the beginning; negative adds frames.")
    Q_CLASSINFO("arg.trimClip.end", "integer|Frames to remove from the end; negative adds frames.")
    Q_CLASSINFO("arg.trimClip.ripple",
                "boolean|Move later clips to follow the change instead of leaving or using a gap. "
                "Defaults to false.")
    Q_INVOKABLE QVariant trimClip(const QString &clipId, const QVariantMap &options);

    Q_CLASSINFO("mcp.updateClip", "edit")
    Q_CLASSINFO("doc.updateClip", "Changes clip settings and returns the clip.")
    Q_CLASSINFO("arg.updateClip.clipId", "Clip id.")
    Q_CLASSINFO("arg.updateClip.fadeIn", "integer|Fade in length in frames; 0 removes it.")
    Q_CLASSINFO("arg.updateClip.fadeOut", "integer|Fade out length in frames; 0 removes it.")
    Q_INVOKABLE QVariant updateClip(const QString &clipId, const QVariantMap &options);

    Q_CLASSINFO("mcp.addTrack", "edit")
    Q_CLASSINFO("doc.addTrack",
                "Adds a track and returns it. Video tracks are added at the top (index 0), audio "
                "tracks at the bottom.")
    Q_CLASSINFO("arg.addTrack.type", "string!|\"video\" or \"audio\".")
    Q_CLASSINFO("arg.addTrack.name", "string|Track name.")
    Q_INVOKABLE QVariant addTrack(const QVariantMap &options);

    Q_CLASSINFO("mcp.updateTrack", "edit")
    Q_CLASSINFO("doc.updateTrack", "Changes track settings and returns the track.")
    Q_CLASSINFO("arg.updateTrack.trackIndex", "Track index.")
    Q_CLASSINFO("arg.updateTrack.name", "string|Track name.")
    Q_CLASSINFO("arg.updateTrack.mute", "boolean|Mute the track's audio.")
    Q_CLASSINFO("arg.updateTrack.hidden", "boolean|Hide a video track.")
    Q_CLASSINFO("arg.updateTrack.locked", "boolean|Lock the track against edits.")
    Q_CLASSINFO("arg.updateTrack.composite", "boolean|Blend a video track with the tracks below it.")
    Q_INVOKABLE QVariant updateTrack(int trackIndex, const QVariantMap &options);

    Q_CLASSINFO("mcp.removeTrack", "edit")
    Q_CLASSINFO("doc.removeTrack",
                "Removes a track and all its clips, and returns what the track was.")
    Q_CLASSINFO("arg.removeTrack.trackIndex", "Track index.")
    Q_INVOKABLE QVariant removeTrack(int trackIndex);

    Q_CLASSINFO("mcp.addTransitions", "edit")
    Q_CLASSINFO("doc.addTransitions",
                "Adds default dissolve/audio crossfade transitions between consecutive supplied "
                "clips on each track, allowing only blank gaps between them. Returns {created, "
                "skipped}; invalid pairs are skipped with reasons. Overlap uses existing footage; "
                "preserve uses spare frames before the right clip's in point and consumes the "
                "left clip's tail without shortening the combined duration. Gaps are closed only "
                "when closeGaps is true. With ripple off, downstream clips stay in place and "
                "vacated space becomes a gap; ripple shifts them, optionally on all unlocked "
                "tracks. Markers stay in place. Successful pairs form one undo transaction.")
    Q_CLASSINFO("arg.addTransitions.clipIds", "Clip ids to pair in timeline order.")
    Q_CLASSINFO("arg.addTransitions.duration", "integer!|Transition duration in frames; positive.")
    Q_CLASSINFO("arg.addTransitions.mode", "string|overlap or preserve; defaults to overlap.")
    Q_CLASSINFO("arg.addTransitions.closeGaps",
                "boolean|Include pairs separated by a blank gap and close it; defaults to true.")
    Q_CLASSINFO("arg.addTransitions.ripple",
                "boolean|Shift later clips to close vacated space; defaults to false.")
    Q_CLASSINFO("arg.addTransitions.rippleAllTracks",
                "boolean|Ripple all unlocked tracks; requires ripple=true; defaults to false.")
    Q_INVOKABLE QVariant addTransitions(const QStringList &clipIds, const QVariantMap &options);

    Q_CLASSINFO("mcp.seek", "action")
    Q_CLASSINFO("doc.seek",
                "Shows the timeline in the player, moves the playhead to a frame, and returns "
                "the frame.")
    Q_CLASSINFO("arg.seek.position", "Frame; clamped to the timeline duration.")
    Q_INVOKABLE QVariant seek(int position);

    Q_CLASSINFO("mcp.setSelection", "action")
    Q_CLASSINFO("doc.setSelection",
                "Selects clips in the timeline, replacing the selection, and returns the new "
                "selection. An empty list clears it.")
    Q_CLASSINFO("arg.setSelection.clipIds", "Clip ids.")
    Q_INVOKABLE QVariant setSelection(const QStringList &clipIds);

    Q_CLASSINFO("mcp.setCurrentTrack", "action")
    Q_CLASSINFO("doc.setCurrentTrack",
                "Makes a track current; the GUI uses it as the target for adding clips.")
    Q_CLASSINFO("arg.setCurrentTrack.trackIndex", "Track index.")
    Q_INVOKABLE QVariant setCurrentTrack(int trackIndex);

signals:
    void changed();
    void positionChanged();
    void currentTrackChanged();
    void selectionChanged();

private:
    QVariant addClip(const char *method, const QVariantMap &options);
    MultitrackModel *model() const;
    bool isValidTrack(int trackIndex) const;
    QVariantMap trackSnapshot(int trackIndex) const;
    QVariantMap clipSnapshot(int trackIndex, int clipIndex) const;
    int clipCount(int trackIndex) const;
    bool findEditableClip(const QString &clipId, int *trackIndex, int *clipIndex) const;
    bool checkTrack(int trackIndex) const;
    bool checkUnlocked(int trackIndex) const;
    bool checkClipFits(int trackIndex, int position, int duration, int ignoreClipIndex) const;
    int blankDuration(int trackIndex, int clipIndex) const;
    QVariant clipSnapshotById(const QString &clipId) const;
};

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APITIMELINE_H
