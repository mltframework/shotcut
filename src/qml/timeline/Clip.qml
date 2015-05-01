/*
 * Copyright (c) 2013-2014 Meltytech, LLC
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

import QtQuick 2.2
import QtQuick.Controls 1.0
import QtGraphicalEffects 1.0

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
    property bool isTransition: false
    property var audioLevels
    property int fadeIn: 0
    property int fadeOut: 0
    property int trackIndex
    property int originalTrackIndex: trackIndex
    property int originalClipIndex: index
    property int originalX: x
    property bool selected: false

    signal clicked(var clip)
    signal moved(var clip)
    signal dragged(var clip, var mouse)
    signal dropped(var clip)
    signal draggedToTrack(var clip, int direction)
    signal trimmingIn(var clip, real delta, var mouse)
    signal trimmedIn(var clip)
    signal trimmingOut(var clip, real delta, var mouse)
    signal trimmedOut(var clip)

    SystemPalette { id: activePalette }
    gradient: Gradient {
        GradientStop {
            id: gradientStop
            position: 0.00
            color: Qt.lighter(getColor())
        }
        GradientStop {
            id: gradientStop2
            position: 1.0
            color: getColor()
        }
    }

    border.color: 'black'
    border.width: isBlank? 0 : 1
    clip: true
    Drag.active: mouseArea.drag.active
    Drag.proposedAction: Qt.MoveAction
    opacity: Drag.active? 0.5 : 1.0

    function getColor() {
        return isBlank? 'transparent' : isTransition? 'mediumpurple' : isAudio? 'darkseagreen' : root.shotcutBlue
    }

    function reparent(track) {
        parent = track
        isAudio = track.isAudio
        height = track.height
        generateWaveform()
    }

    function generateWaveform() {
        var width = waveform.parent.width - waveform.parent.border.width * 2;
        waveform.width = width;

        if (!waveform.visible) return;
        if (typeof audioLevels == 'undefined') return;

        var cx = waveform.getContext('2d');
        if (cx === null) return
        // TODO use project channel count
        var channels = 2;
        var height = waveform.height;
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
        source: (isAudio || isBlank || isTransition)? '' : 'image://thumbnail/' + mltService + '/' + clipResource + '#' + outPoint
    }

    Image {
        id: outThumbnail
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: parent.border.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 2
        fillMode: Image.PreserveAspectFit
        source: (isAudio || isBlank || isTransition)? '' : 'image://thumbnail/' + mltService + '/' + clipResource + '#' + inPoint
    }

    Canvas {
        id: transitionCanvas
        visible: isTransition
        anchors.fill: parent
        onPaint: {
            var cx = getContext('2d')
            if (cx === null) return
            cx.beginPath()
            cx.moveTo(0, 0)
            cx.lineTo(width, height)
            cx.lineTo(width, 0)
            cx.lineTo(0, height)
            cx.closePath()
            var grad = cx.createLinearGradient(0, 0, 0, height)
            var color = isAudio? 'darkseagreen' : root.shotcutBlue
            grad.addColorStop(0, clipRoot.selected ? Qt.darker(color) : Qt.lighter(color))
            grad.addColorStop(1, color)
            cx.fillStyle = grad
            cx.fill()
            cx.strokeStyle = 'black'
            cx.stroke()
        }
    }

    Canvas {
        id: waveform
        visible: !isBlank && settings.timelineShowWaveforms
        height: isAudio? parent.height : parent.height / 2
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: parent.border.width
        opacity: 0.7
    }

    Rectangle {
        // audio peak line
        width: parent.width - parent.border.width * 2
        visible: !isBlank && !isTransition
        height: 1
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: parent.border.width
        anchors.bottomMargin: waveform.height * 0.9
        color: Qt.darker(parent.color)
        opacity: 0.7
    }

    Rectangle {
        // text background
        color: 'lightgray'
        visible: !isBlank && !isTransition
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
        visible: !isBlank && !isTransition
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
            when: !clipRoot.selected
            PropertyChanges {
                target: clipRoot
                z: 0
            }
        },
        State {
            name: 'selectedBlank'
            when: clipRoot.selected && clipRoot.isBlank
            PropertyChanges {
                target: gradientStop2
                color: Qt.lighter(selectedTrackColor)
            }
            PropertyChanges {
                target: gradientStop
                color: Qt.darker(selectedTrackColor)
            }
        },
        State {
            name: 'selected'
            when: clipRoot.selected
            PropertyChanges {
                target: clipRoot
                z: 1
            }
            PropertyChanges {
                target: gradientStop
                color: Qt.darker(getColor())
            }
        }
    ]

    onStateChanged: if (isTransition) transitionCanvas.requestPaint()

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
            (fadeInMouseArea.drag.active || fadeOutMouseArea.drag.active)? Qt.PointingHandCursor :
            drag.active? Qt.ClosedHandCursor :
            isBlank? Qt.ArrowCursor : Qt.OpenHandCursor
        property int startX
        onPressed: {
            root.stopScrolling = true
            originalX = parent.x
            originalTrackIndex = trackIndex
            originalClipIndex = index
            startX = parent.x
            if (mouse.button === Qt.RightButton)
                menu.popup()
            else
                clipRoot.clicked(clipRoot)
        }
        onPositionChanged: {
            if (mouse.y < 0 && trackIndex > 0)
                parent.draggedToTrack(clipRoot, -1)
            else if (mouse.y > height && (trackIndex + 1) < root.trackCount)
                parent.draggedToTrack(clipRoot, 1)
            parent.dragged(clipRoot, mouse)
        }
        onReleased: {
            root.stopScrolling = false
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
        onDoubleClicked: timeline.openClip(trackIndex, index)
    }

    Canvas {
        id: fadeInCanvas
        visible: !isBlank && !isTransition
        width: parent.fadeIn * timeScale
        height: parent.height - parent.border.width * 2
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: parent.border.width
        opacity: 0.5
        onWidthChanged: requestPaint()
        onPaint: {
            var cx = getContext('2d')
            if (cx === null) return
            cx.beginPath()
            cx.moveTo(0, 0)
            cx.lineTo(width, 0)
            cx.lineTo(0, height)
            cx.closePath()
            cx.fillStyle = 'black'
            cx.fill()
        }
    }
    Rectangle {
        id: fadeInControl
        enabled: !isBlank && !isTransition
        anchors.left: fadeInCanvas.width > radius? undefined : fadeInCanvas.left
        anchors.horizontalCenter: fadeInCanvas.width > radius? fadeInCanvas.right : undefined
        anchors.top: fadeInCanvas.top
        anchors.topMargin: -3
        width: 15
        height: 15
        radius: 7.5
        color: 'black'
        border.width: 2
        border.color: 'white'
        opacity: 0
        Drag.active: fadeInMouseArea.drag.active
        MouseArea {
            id: fadeInMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            property int startX
            property int startFadeIn
            onEntered: parent.opacity = 0.7
            onExited: parent.opacity = 0
            onPressed: {
                root.stopScrolling = true
                startX = parent.x
                startFadeIn = fadeIn
                parent.anchors.left = undefined
                parent.anchors.horizontalCenter = undefined
                parent.opacity = 1
                // trackRoot.clipSelected(clipRoot, trackRoot) TODO
            }
            onReleased: {
                root.stopScrolling = false
                if (fadeInCanvas.width > parent.radius)
                    parent.anchors.horizontalCenter = fadeInCanvas.right
                else
                    parent.anchors.left = fadeInCanvas.left
                bubbleHelp.hide()
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((parent.x - startX) / timeScale)
                    var duration = startFadeIn + delta
                    timeline.fadeIn(trackIndex, index, duration)

                    // Show fade duration as time in a "bubble" help.
                    var s = timeline.timecode(Math.max(duration, 0))
                    bubbleHelp.show(clipRoot.x, trackRoot.y + clipRoot.height, s.substring(6))
                }
            }
        }
        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: fadeInMouseArea.containsMouse
            NumberAnimation {
                from: 1.0
                to: 0.5
                duration: 250
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                from: 0.5
                to: 1.0
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    Canvas {
        id: fadeOutCanvas
        visible: !isBlank && !isTransition
        width: parent.fadeOut * timeScale
        height: parent.height - parent.border.width * 2
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: parent.border.width
        opacity: 0.5
        onWidthChanged: requestPaint()
        onPaint: {
            var cx = getContext('2d')
            if (cx === null) return
            cx.beginPath()
            cx.moveTo(width, 0)
            cx.lineTo(0, 0)
            cx.lineTo(width, height)
            cx.closePath()
            cx.fillStyle = 'black'
            cx.fill()
        }
    }
    Rectangle {
        id: fadeOutControl
        enabled: !isBlank && !isTransition
        anchors.right: fadeOutCanvas.width > radius? undefined : fadeOutCanvas.right
        anchors.horizontalCenter: fadeOutCanvas.width > radius? fadeOutCanvas.left : undefined
        anchors.top: fadeOutCanvas.top
        anchors.topMargin: -3
        width: 15
        height: 15
        radius: 7.5
        color: 'black'
        border.width: 2
        border.color: 'white'
        opacity: 0
        Drag.active: fadeOutMouseArea.drag.active
        MouseArea {
            id: fadeOutMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            property int startX
            property int startFadeOut
            onEntered: parent.opacity = 0.7
            onExited: parent.opacity = 0
            onPressed: {
                root.stopScrolling = true
                startX = parent.x
                startFadeOut = fadeOut
                parent.anchors.right = undefined
                parent.anchors.horizontalCenter = undefined
                parent.opacity = 1
            }
            onReleased: {
                root.stopScrolling = false
                if (fadeOutCanvas.width > parent.radius)
                    parent.anchors.horizontalCenter = fadeOutCanvas.left
                else
                    parent.anchors.right = fadeOutCanvas.right
                bubbleHelp.hide()
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((startX - parent.x) / timeScale)
                    var duration = startFadeOut + delta
                    timeline.fadeOut(trackIndex, index, duration)

                    // Show fade duration as time in a "bubble" help.
                    var s = timeline.timecode(Math.max(duration, 0))
                    bubbleHelp.show(clipRoot.x + clipRoot.width, trackRoot.y + clipRoot.height, s.substring(6))
                }
            }
        }
        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: fadeOutMouseArea.containsMouse
            NumberAnimation {
                from: 1.0
                to: 0.5
                duration: 250
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                from: 0.5
                to: 1.0
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    Rectangle {
        id: trimIn
        enabled: !isBlank && !isTransition
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
                root.stopScrolling = true
                startX = parent.x
                originalX = 0 // reusing originalX to accumulate delta for bubble help
                parent.anchors.left = undefined
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.left = clipRoot.left
                clipRoot.trimmedIn(clipRoot)
                parent.opacity = 0
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((parent.x - startX) / timeScale)
                    if (Math.abs(delta) > 0) {
                        if (clipDuration + originalX + delta > 0)
                            originalX += delta
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
        enabled: !isBlank && !isTransition
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
                root.stopScrolling = true
                duration = clipDuration
                originalX = 0 // reusing originalX to accumulate delta for bubble help
                parent.anchors.right = undefined
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.right = clipRoot.right
                clipRoot.trimmedOut(clipRoot)
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var newDuration = Math.round((parent.x + parent.width) / timeScale)
                    var delta = duration - newDuration 
                    if (Math.abs(delta) > 0) {
                        if (clipDuration - originalX - delta > 0)
                            originalX += delta
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
        // XXX This is a workaround for menus appearing in wrong location in a Quick
        // view used in a DockWidget on OS X.
        Component.onCompleted: if (timeline.yoffset) __yOffset = timeline.yoffset
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
            visible: !isBlank && !isTransition
            text: qsTr('Open As Clip')
            onTriggered: timeline.openClip(trackIndex, index)
        }
        MenuItem {
            visible: !isBlank && !isTransition
            text: qsTr('Split At Playhead (S)')
            onTriggered: timeline.splitClip(trackIndex, index)
        }
    }
}
