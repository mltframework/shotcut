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
import QtQuick.Controls 1.0

Rectangle {
    id: clipRoot
    property string clipName: ''
    property string clipResource: ''
    property string mltService: ''
    property int inPoint: 0
    property int outPoint: 0
    property int clipDuration: 0
    property bool isBlank: false
    property bool isAudio: false
    property var audioLevels
    property int trackIndex
    property int originalTrackIndex: trackIndex
    property int originalClipIndex: index
    property int originalX: x
    property color shotcutBlue: Qt.rgba(23/255, 92/255, 118/255, 1.0)

    signal selected(var clip)
    signal moved(var clip)
    signal dragged(var clip, var mouse)
    signal dropped(var clip)
    signal draggedToTrack(var clip, int direction)
    signal trimmingIn(var clip, real delta, var mouse)
    signal trimmedIn(var clip)
    signal trimmingOut(var clip, real delta, var mouse)
    signal trimmedOut(var clip)

    SystemPalette { id: activePalette }

    color: getColor()
    border.color: 'black'
    border.width: isBlank? 0 : 1
    clip: true
    state: 'normal'
    Drag.active: mouseArea.drag.active
    Drag.proposedAction: Qt.MoveAction

    function getColor() {
        return isBlank? 'transparent' : (isAudio? 'darkseagreen' : shotcutBlue)
    }

    function reparent(track) {
        parent = track
        isAudio = track.isAudio
        height = track.height
        generateWaveform()
    }

    function generateWaveform() {
        if (typeof audioLevels == 'undefined') return;
        var cx = waveform.getContext('2d');
        // TODO use project channel count
        var channels = 2;
        var height = waveform.height;
        var width = waveform.width;
        var color = getColor();
        cx.beginPath();
        cx.moveTo(-1, height);
        for (var i = 0; i < width; i++) {
            var j = Math.round(i / timeScale);
            var level = Math.max(audioLevels[j * channels], audioLevels[j * channels + 1]) / 256;
            cx.lineTo(i, height - level * height);
        }
        cx.lineTo(width, height);
        cx.closePath();
        cx.fillStyle = Qt.lighter(color);
        cx.fill();
        cx.strokeStyle = Qt.darker(color);
        cx.stroke();
        waveform.requestPaint();
    }

    onAudioLevelsChanged: generateWaveform()

    Image {
        id: inThumbnail
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: parent.border.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 2
        fillMode: Image.PreserveAspectFit
        source: (isAudio || isBlank)? '' : 'image://thumbnail/' + mltService + '/' + clipResource + '#' + outPoint
    }

    Image {
        id: outThumbnail
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: parent.border.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 2
        fillMode: Image.PreserveAspectFit
        source: (isAudio || isBlank)? '' : 'image://thumbnail/' + mltService + '/' + clipResource + '#' + inPoint
    }

    Canvas {
        id: waveform
        visible: !isBlank
        width: parent.width - parent.border.width * 2
        height: isAudio? parent.height : parent.height / 2
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: parent.border.width
        opacity: 0.7
    }

    Rectangle {
        width: parent.width - parent.border.width * 2
        visible: !isBlank
        height: 1
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: parent.border.width
        anchors.bottomMargin: waveform.height * 0.9
        color: Qt.darker(parent.color)
        opacity: 0.7
    }

    Rectangle {
        color: 'lightgray'
        visible: !isBlank
        opacity: 0.7
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: parent.border.width
        anchors.leftMargin: parent.border.width + (isAudio? 0 : inThumbnail.width)
        width: label.width + 2
        height: label.height
    }

    Text {
        id: label
        text: clipName
        visible: !isBlank
        font.pointSize: 8
        anchors {
            top: parent.top
            left: parent.left
            topMargin: parent.border.width + 1
            leftMargin: parent.border.width + (isAudio? 0 : inThumbnail.width) + 1
        }
        color: 'black'
    }

    states: [
        State {
            name: 'normal'
            PropertyChanges {
                target: clipRoot
                z: 0
            }
        },
        State {
            name: 'selected'
            PropertyChanges {
                target: clipRoot
                color: isBlank? 'transparent' : Qt.darker(getColor())
                z: 1
            }
        }
    ]

    transitions: [
        Transition {
            to: '*'
            ColorAnimation { target: clipRoot; duration: 50 }
        }
    ]

    MouseArea {
        anchors.fill: parent
        enabled: isBlank
        acceptedButtons: Qt.RightButton
        onClicked: menu.popup()
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: !isBlank
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        propagateComposedEvents: true
        drag.target: parent
        drag.axis: Drag.XAxis
        cursorShape: (trimInMouseArea.drag.active || trimOutMouseArea.drag.active)? Qt.SizeHorCursor :
            drag.active? Qt.ClosedHandCursor :
            isBlank? Qt.ArrowCursor : Qt.OpenHandCursor
        property int startX
        onPressed: {
            originalX = parent.x
            originalTrackIndex = trackIndex
            originalClipIndex = index
            startX = parent.x
            parent.state = 'selected'
            parent.selected(clipRoot)
            if (mouse.button === Qt.RightButton)
                menu.popup()
        }
        onPositionChanged: {
            if (mouse.y < 0 && trackIndex > 0)
                parent.draggedToTrack(clipRoot, -1)
            else if (mouse.y > height && (trackIndex + 1) < root.trackCount)
                parent.draggedToTrack(clipRoot, 1)
            parent.dragged(clipRoot, mouse)
        }
        onReleased: {
            parent.y = 0
            var delta = parent.x - startX
            if (Math.abs(delta) >= 1.0 || trackIndex !== originalTrackIndex) {
                parent.moved(clipRoot)
                originalX = parent.x
                originalTrackIndex = trackIndex
            } else {
                parent.dropped(clipRoot)
            }
        }
    }

    Rectangle {
        id: trimIn
        enabled: !isBlank
        anchors.left: parent.left
        anchors.leftMargin: 0
        height: parent.height
        width: 5
        color: isAudio? 'green' : 'lawngreen'
        opacity: 0
        Drag.active: trimInMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction

        MouseArea {
            id: trimInMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            property int startX

            onPressed: {
                startX = parent.x
                parent.anchors.left = undefined
            }
            onReleased: {
                parent.anchors.left = clipRoot.left
                clipRoot.trimmedIn(clipRoot)
                parent.opacity = 0
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((parent.x - startX) / timeScale)
                    if (Math.abs(delta) > 0) {
                        clipRoot.trimmingIn(clipRoot, delta, mouse)
                    }
                }
            }
            onEntered: parent.opacity = 0.5
            onExited: parent.opacity = 0
        }
    }
    Rectangle {
        id: trimOut
        enabled: !isBlank
        anchors.right: parent.right
        anchors.rightMargin: 0
        height: parent.height
        width: 5
        color: 'red'
        opacity: 0
        Drag.active: trimOutMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction

        MouseArea {
            id: trimOutMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            property int duration

            onPressed: {
                duration = clipDuration
                parent.anchors.right = undefined
            }
            onReleased: {
                parent.anchors.right = clipRoot.right
                clipRoot.trimmedOut(clipRoot)
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var newDuration = Math.round((parent.x + parent.width) / timeScale)
                    var delta = duration - newDuration 
                    if (Math.abs(delta) > 0) {
                        clipRoot.trimmingOut(clipRoot, delta, mouse)
                        duration = newDuration
                    }
                }
            }
            onEntered: parent.opacity = 0.5
            onExited: parent.opacity = 0
        }
    }
    Menu {
        id: menu
        MenuItem {
            text: qsTr('Remove')
            onTriggered: timeline.remove(trackIndex, index)
        }
        MenuItem {
            visible: !isBlank
            text: qsTr('Lift')
            onTriggered: timeline.lift(trackIndex, index)
        }
        MenuItem {
            visible: !isBlank
            text: qsTr('Open As Clip')
            onTriggered: timeline.openClip(trackIndex, index)
        }
        MenuItem {
            visible: !isBlank
            text: qsTr('Split At Playhead (S)')
            onTriggered: timeline.splitClip(trackIndex, index)
        }
    }
}
