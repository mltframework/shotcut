/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

import QtQuick 2.0
import QtQml.Models 2.1
import 'Track.js' as Logic

Rectangle {
    id: trackRoot
    property alias model: trackModel.model
    property alias rootIndex: trackModel.rootIndex
    property bool isAudio
    property real timeScale: 1.0

    signal clipSelected(var clip, var track)
    signal clipDragged(var clip, int x, int y)
    signal clipDropped(var clip)
    signal clipDraggedToTrack(var clip, int direction)

    function resetStates(index) {
        for (var i = 0; i < repeater.count; i++)
            if (i !== index) repeater.itemAt(i).state = ''
    }

    function redrawWaveforms() {
        for (var i = 0; i < repeater.count; i++)
            repeater.itemAt(i).generateWaveform()
    }

    color: 'transparent'

    DelegateModel {
        id: trackModel
        Clip {
            clipName: model.name
            clipResource: model.resource
            clipDuration: model.duration
            mltService: model.mlt_service
            inPoint: model.in
            outPoint: model.out
            isBlank: model.blank
            isAudio: model.audio
            audioLevels: model.audioLevels
            width: model.duration * timeScale
            height: trackRoot.height
            trackIndex: trackRoot.DelegateModel.itemsIndex
            onSelected: {
                resetStates(clip.DelegateModel.itemsIndex);
                trackRoot.clipSelected(clip, trackRoot);
            }
            onMoved: {
                if (!multitrack.moveClip(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex, Math.round(clip.x / timeScale)))
                    clip.x = clip.originalX
            }
            onDragged: {
                // Snap if Alt key is not down.
                if (!(mouse.modifiers & Qt.AltModifier))
                    Logic.snapClip(clip)
                var mapped = trackRoot.mapFromItem(clip, mouse.x, mouse.y)
                trackRoot.clipDragged(clip, mapped.x, mapped.y)
            }
            onTrimmingIn: {
                if (!(mouse.modifiers & Qt.AltModifier))
                    delta = Logic.snapTrimIn(clip, delta)
                if (delta != 0)
                    multitrack.trimClipIn(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex, delta)
            }
            onTrimmedIn:multitrack.notifyClipIn(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex)
            onTrimmingOut: {
                if (!(mouse.modifiers & Qt.AltModifier))
                    delta = Logic.snapTrimOut(clip, delta)
                if (delta != 0)
                    multitrack.trimClipOut(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex, delta)
            }
            onTrimmedOut: multitrack.notifyClipOut(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex)

            Component.onCompleted: {
                moved.connect(trackRoot.clipDropped)
                dropped.connect(trackRoot.clipDropped)
                draggedToTrack.connect(trackRoot.clipDraggedToTrack)
            }

        }
    }

    Row {
        Repeater { id: repeater; model: trackModel }
    }
}
