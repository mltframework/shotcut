/*
 * Copyright (c) 2013-2020 Meltytech, LLC
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

import QtQml.Models 2.12
import QtQuick 2.12
import "Track.js" as Logic

Rectangle {
    id: trackRoot

    property alias model: trackModel.model
    property alias rootIndex: trackModel.rootIndex
    property bool isAudio
    property real timeScale: 1
    property bool placeHolderAdded: false
    property bool isCurrentTrack: false
    property bool isLocked: false
    property alias clipCount: repeater.count
    property bool isMute: false

    signal clipClicked(var clip, var track, var mouse)
    signal clipRightClicked(var clip, var track, var mouse)
    signal clipDragged(var clip, int x, int y)
    signal clipDropped(var clip)
    signal clipDraggedToTrack(var clip, int direction)
    signal checkSnap(var clip)

    function redrawWaveforms(force) {
        for (var i = 0; i < repeater.count; i++) repeater.itemAt(i).generateWaveform(force)
    }

    function remakeWaveforms(force) {
        for (var i = 0; i < repeater.count; i++) timeline.remakeAudioLevels(trackRoot.DelegateModel.itemsIndex, i, force)
    }

    function updateThumbnails(clipIndex) {
        if (clipIndex >= 0 && clipIndex < repeater.count)
            repeater.itemAt(clipIndex).updateThumbnails();

    }

    function snapClip(clip) {
        Logic.snapClip(clip, repeater);
    }

    function snapDrop(clip) {
        Logic.snapDrop(clip, repeater);
    }

    function clipAt(index) {
        return repeater.itemAt(index);
    }

    color: 'transparent'
    width: clipRow.width
    onIsMuteChanged: {
        if (!isMute)
            redrawWaveforms(true);

    }

    DelegateModel {
        id: trackModel

        Clip {
            clipName: typeof model.name !== 'undefined' ? model.name : ""
            clipComment: typeof model.comment !== 'undefined' ? model.comment : ""
            clipResource: typeof model.resource !== 'undefined' ? model.resource : ""
            clipDuration: typeof model.duration !== 'undefined' ? model.duration : 0
            mltService: typeof model.mlt_service !== 'undefined' ? model.mlt_service : ""
            inPoint: typeof model.in !== 'undefined' ? model.in : 0
            outPoint: typeof model.out !== 'undefined' ? model.out : 0
            isBlank: typeof model.blank !== 'undefined' ? model.blank : false
            isAudio: typeof model.audio !== 'undefined' ? model.audio : false
            isTransition: typeof model.isTransition !== 'undefined' ? model.isTransition : false
            audioLevels: typeof model.audioLevels !== 'undefined' ? model.audioLevels : ""
            width: typeof model.duration !== 'undefined' ? model.duration * timeScale : 0
            height: trackRoot.height
            trackIndex: trackRoot.DelegateModel.itemsIndex
            fadeIn: typeof model.fadeIn !== 'undefined' ? model.fadeIn : 0
            fadeOut: typeof model.fadeOut !== 'undefined' ? model.fadeOut : 0
            hash: typeof model.hash !== 'undefined' ? model.hash : 0
            speed: typeof model.speed !== 'undefined' ? model.speed : 1
            audioIndex: typeof model.audioindex !== 'undefined' ? model.audioIndex : 0
            selected: Logic.selectionContains(timeline.selection, trackIndex, index)
            isTrackMute: trackRoot.isMute
            onClicked: trackRoot.clipClicked(clip, trackRoot, mouse)
            onClipRightClicked: trackRoot.clipRightClicked(clip, trackRoot, mouse)
            onMoved: {
                var fromTrack = clip.originalTrackIndex;
                var toTrack = clip.trackIndex;
                var clipIndex = clip.originalClipIndex;
                var selection = timeline.selection;
                var frame = Math.round(clip.x / timeScale);
                // Workaround moving multiple clips on the same track with ripple on
                if (fromTrack === toTrack && settings.timelineRipple && selection.length > 1) {
                    // Use the left-most clip
                    var clipIndexChanged = false;
                    for (var i = 0; i < selection.length; i++) {
                        if (selection[i].y === fromTrack && selection[i].x < clipIndex) {
                            clipIndex = selection[i].x;
                            clipIndexChanged = true;
                        }
                    }
                    if (clipIndexChanged)
                        frame = Math.round((clipAt(clipIndex).x + clip.x - clip.originalX) / timeScale);

                }
                // Remove the placeholder inserted in onDraggedToTrack
                if (placeHolderAdded) {
                    placeHolderAdded = false;
                    root.resetDrag();
                    multitrack.reload(true);
                }
                if (!timeline.moveClip(fromTrack, toTrack, clipIndex, frame, settings.timelineRipple)) {
                    clip.x = clip.originalX;
                    clip.trackIndex = clip.originalTrackIndex;
                }
            }
            onDragged: {
                if (settings.timelineDragScrub) {
                    root.stopScrolling = false;
                    timeline.position = Math.round(clip.x / timeScale);
                }
                // Snap if Alt key is not down.
                if (!(mouse.modifiers & Qt.AltModifier) && settings.timelineSnap)
                    trackRoot.checkSnap(clip);

                // Prevent dragging left of multitracks origin.
                clip.x = Math.max(0, clip.x);
                var mapped = trackRoot.mapFromItem(clip, mouse.x, mouse.y);
                trackRoot.clipDragged(clip, mapped.x, mapped.y);
                // Show distance moved as time in a "bubble" help.
                var delta = Math.round(clip.x / timeScale) - model.start;
                var s = application.timecode(Math.abs(delta));
                // remove leading zeroes
                if (s.substring(0, 3) === '00:')
                    s = s.substring(3);

                s = ((delta < 0) ? '-' : (delta > 0) ? '+' : '') + s;
                bubbleHelp.show(s);
            }
            onTrimmingIn: {
                var originalDelta = delta;
                if (!(mouse.modifiers & Qt.AltModifier) && settings.timelineSnap && !settings.timelineRipple)
                    delta = Logic.snapTrimIn(clip, delta, root, trackRoot.DelegateModel.itemsIndex);

                if (delta != 0) {
                    if (timeline.trimClipIn(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex, clip.originalClipIndex, delta, settings.timelineRipple)) {
                        // Show amount trimmed as a time in a "bubble" help.
                        var s = application.timecode(Math.abs(clip.originalX));
                        s = '%1%2 = %3'.arg((clip.originalX < 0) ? '-' : (clip.originalX > 0) ? '+' : '').arg(s.substring(3)).arg(application.timecode(clipDuration));
                        bubbleHelp.show(s);
                    } else {
                        clip.originalX -= originalDelta;
                    }
                } else {
                    clip.originalX -= originalDelta;
                }
            }
            onTrimmedIn: {
                multitrack.notifyClipIn(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex);
                // Notify out point of clip A changed when trimming to add a transition.
                if (clip.DelegateModel.itemsIndex > 1 && repeater.itemAt(clip.DelegateModel.itemsIndex - 1).isTransition)
                    multitrack.notifyClipOut(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex - 2);

                bubbleHelp.hide();
                timeline.commitTrimCommand();
            }
            onTrimmingOut: {
                var originalDelta = delta;
                if (!(mouse.modifiers & Qt.AltModifier) && settings.timelineSnap && !settings.timelineRipple)
                    delta = Logic.snapTrimOut(clip, delta, root, trackRoot.DelegateModel.itemsIndex);

                if (delta != 0) {
                    if (timeline.trimClipOut(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex, delta, settings.timelineRipple)) {
                        // Show amount trimmed as a time in a "bubble" help.
                        var s = application.timecode(Math.abs(clip.originalX));
                        s = '%1%2 = %3'.arg((clip.originalX < 0) ? '+' : (clip.originalX > 0) ? '-' : '').arg(s.substring(3)).arg(application.timecode(clipDuration));
                        bubbleHelp.show(s);
                    } else {
                        clip.originalX -= originalDelta;
                    }
                } else {
                    clip.originalX -= originalDelta;
                }
            }
            onTrimmedOut: {
                multitrack.notifyClipOut(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex);
                // Notify in point of clip B changed when trimming to add a transition.
                if (clip.DelegateModel.itemsIndex + 2 < repeater.count && repeater.itemAt(clip.DelegateModel.itemsIndex + 1).isTransition)
                    multitrack.notifyClipIn(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex + 2);

                bubbleHelp.hide();
                timeline.commitTrimCommand();
            }
            onDraggedToTrack: {
                if (!placeHolderAdded) {
                    placeHolderAdded = true;
                    trackModel.items.insert(clip.DelegateModel.itemsIndex, {
                        "name": '',
                        "resource": '',
                        "duration": clip.clipDuration,
                        "mlt_service": '<producer',
                        "in": 0,
                        "out": clip.clipDuration - 1,
                        "blank": true,
                        "audio": false,
                        "isTransition": false,
                        "fadeIn": 0,
                        "fadeOut": 0,
                        "hash": '',
                        "speed": 1
                    });
                }
            }
            onDropped: {
                if (placeHolderAdded) {
                    timeline.selection = [];
                    multitrack.reload(true);
                    placeHolderAdded = false;
                }
            }
            Component.onCompleted: {
                moved.connect(trackRoot.clipDropped);
                dropped.connect(trackRoot.clipDropped);
                draggedToTrack.connect(trackRoot.clipDraggedToTrack);
            }
        }

    }

    Row {
        id: clipRow

        Repeater {
            id: repeater

            model: trackModel
        }

    }

}
