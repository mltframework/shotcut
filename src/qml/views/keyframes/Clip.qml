/*
 * Copyright (c) 2016-2020 Meltytech, LLC
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

import QtQuick 2.12
import QtQuick.Controls 2.12
import Shotcut.Controls 1.0 as Shotcut

Rectangle {
    id: clipRoot
    property string clipName: ''
    property string clipResource: ''
    property string mltService: ''
    property int inPoint: 0
    property int outPoint: 0
    property int clipDuration: outPoint - inPoint + 1
    property bool isBlank: false
    property bool isAudio: false
    property var audioLevels
    property int animateIn: 0
    property int animateOut: 0
    property int trackIndex: 0
    property int index: 0
    property int originalTrackIndex: trackIndex
    property int originalClipIndex: index
    property int originalX: x
    property bool selected: false
    property string hash: ''
    property double speed: 1.0
    property bool inThumbnailVisible: true
    property bool outThumbnailVisible: true

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

    width: clipDuration * timeScale
    border.color: selected? 'red' : 'black'
    border.width: 1
    clip: true
    opacity: isBlank? 0.5 : 1.0

    function getColor() {
        return isAudio? 'darkseagreen' : root.shotcutBlue
    }

    function generateWaveform() {
        // This is needed to make the model have the correct count.
        // Model as a property expression is not working in all cases.
        waveformRepeater.model = Math.ceil(waveform.innerWidth / waveform.maxWidth)
        for (var i = 0; i < waveformRepeater.count; i++)
            waveformRepeater.itemAt(0).update()
    }

    function imagePath(time) {
        return 'image://thumbnail/' + hash + '/' + mltService + '/' + clipResource + '#' + time
    }

    onAudioLevelsChanged: generateWaveform()

    Image {
        id: outThumbnail
        visible: settings.timelineShowThumbnails && outThumbnailVisible && metadata !== null &&
                 parent.height > 20 && x > inThumbnail.width
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.rightMargin: parent.border.width + 1
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 2
        width: height * 16.0/9.0
        fillMode: Image.PreserveAspectFit
        source: visible? imagePath(outPoint) : ''
    }

    Image {
        id: inThumbnail
        visible: settings.timelineShowThumbnails && inThumbnailVisible && metadata !== null && parent.height > 20
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 2
        width: height * 16.0/9.0
        fillMode: Image.PreserveAspectFit
        source: visible? imagePath(inPoint) : ''
    }

    Row {
        id: waveform
        visible: settings.timelineShowWaveforms
        height: (isAudio || parent.height <= 20)? parent.height : parent.height / 2
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: parent.border.width
        opacity: 0.7
        property int maxWidth: Math.max(application.maxTextureSize / 2, 2048)
        property int innerWidth: clipRoot.width - clipRoot.border.width * 2

        Repeater {
            id: waveformRepeater
            model: Math.ceil(waveform.innerWidth / waveform.maxWidth)
            Shotcut.TimelineWaveform {
                width: Math.min(waveform.innerWidth, waveform.maxWidth)
                height: waveform.height
                fillColor: getColor()
                property int channels: 2
                inPoint: Math.round((clipRoot.inPoint + index * waveform.maxWidth / timeScale) * speed) * channels
                outPoint: inPoint + Math.round(width / timeScale * speed) * channels
                levels: audioLevels
                active: ((clipRoot.x + x + width)   > tracksFlickable.contentX) && // right edge
                        ((clipRoot.x + x)           < tracksFlickable.contentX + tracksFlickable.width) && // left edge
                        ((trackRoot.y + y + height) > tracksFlickable.contentY) && // bottom edge
                        ((trackRoot.y + y)          < tracksFlickable.contentY + tracksFlickable.height) // top edge
            }
        }
    }

    Rectangle {
        // audio peak line
        width: parent.width - parent.border.width * 2
        visible: waveform.visible
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
        visible: !isBlank
        opacity: 0.7
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: parent.border.width
        anchors.leftMargin: parent.border.width +
            ((isAudio || !settings.timelineShowThumbnails) ? 0 : inThumbnail.width)
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
            leftMargin: parent.border.width +
                ((isAudio || !settings.timelineShowThumbnails) ? 0 : inThumbnail.width) + 1
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

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        propagateComposedEvents: true
        cursorShape: (trimInMouseArea.drag.active || trimOutMouseArea.drag.active)? Qt.SizeHorCursor :
            (animateInMouseArea.drag.active || animateOutMouseArea.drag.active)? Qt.PointingHandCursor :
            Qt.ArrowCursor
        onClicked: menu.popup()
    }

    Shotcut.TimelineTriangle {
        id: animateInTriangle
        visible: !isBlank
        width: parent.animateIn * timeScale
        height: parent.height - parent.border.width * 2
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: parent.border.width
        opacity: 0.5
    }
    Rectangle {
        id: animateInControl
        visible: metadata !== null && metadata.keyframes.allowAnimateIn
        enabled: !isBlank
        anchors.left: animateInTriangle.right
        anchors.top: animateInTriangle.top
        anchors.leftMargin: Math.min(clipRoot.width - animateInTriangle.width - width, 0)
        anchors.topMargin: -3
        width: 14
        height: 14
        radius: 7
        z: 1
        color: 'black'
        border.width: 2
        border.color: 'white'
        opacity: enabled? 0.7 : 0
        Drag.active: animateInMouseArea.drag.active
        MouseArea {
            id: animateInMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            drag.minimumX: 0
            drag.maximumX: clipRoot.width
            property int startX
            property int startFadeIn
            onPressed: {
                root.stopScrolling = true
                startX = parent.x
                startFadeIn = animateIn
                parent.anchors.left = undefined
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.left = animateInTriangle.right
                bubbleHelp.hide()
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((parent.x - startX) / timeScale)
                    var duration = Math.min(Math.max(0, startFadeIn + delta), clipDuration)
                    filter.animateIn = duration

                    // Show fade duration as time in a "bubble" help.
                    var s = application.timecode(Math.max(duration, 0))
                    bubbleHelp.show(clipRoot.x, trackRoot.y + clipRoot.height, s.substring(6))
                }
            }
            onDoubleClicked: filter.animateIn = (filter.animateIn > 0) ? 0 : Math.round(profile.fps)
            onExited: animateInControl.scale = 1.0
        }
        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: animateInMouseArea.containsMouse
            NumberAnimation {
                from: 1.0
                to: 1.5
                duration: 250
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                from: 1.5
                to: 1.0
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    Shotcut.TimelineTriangle {
        id: animateOutTriangle
        visible: !isBlank
        width: parent.animateOut * timeScale
        height: parent.height - parent.border.width * 2
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: parent.border.width
        opacity: 0.5
        transform: Scale { xScale: -1; origin.x: animateOutTriangle.width / 2}
    }
    Rectangle {
        id: animateOutControl
        visible: metadata !== null && metadata.keyframes.allowAnimateOut
        enabled: !isBlank
        anchors.right: animateOutTriangle.left
        anchors.top: animateOutTriangle.top
        anchors.rightMargin: Math.min(clipRoot.width - animateOutTriangle.width - width, 0)
        anchors.topMargin: -3
        width: 14
        height: 14
        radius: 7
        color: 'black'
        border.width: 2
        border.color: 'white'
        opacity: enabled? 0.7 : 0
        Drag.active: animateOutMouseArea.drag.active
        MouseArea {
            id: animateOutMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            drag.minimumX: -width - 1
            drag.maximumX: clipRoot.width
            property int startX
            property int startFadeOut
            onPressed: {
                root.stopScrolling = true
                startX = parent.x
                startFadeOut = animateOut
                parent.anchors.right = undefined
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.right = animateOutTriangle.left
                bubbleHelp.hide()
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((startX - parent.x) / timeScale)
                    var duration = Math.min(Math.max(0, startFadeOut + delta), clipDuration)
                    filter.animateOut = duration

                    // Show fade duration as time in a "bubble" help.
                    var s = application.timecode(duration, 0)
                    bubbleHelp.show(clipRoot.x + clipRoot.width, trackRoot.y + clipRoot.height, s.substring(6))
                }
            }
            onDoubleClicked: filter.animateOut = (filter.animateOut > 0) ?  0 : Math.round(profile.fps)
            onExited: animateOutControl.scale = 1.0
        }
        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: animateOutMouseArea.containsMouse
            NumberAnimation {
                from: 1.0
                to: 1.5
                duration: 250
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                from: 1.5
                to: 1.0
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    Rectangle {
        id: trimIn
        visible: metadata !== null && metadata.keyframes.allowTrim
        enabled: !isBlank
        anchors.left: parent.left
        anchors.leftMargin: 0
        height: parent.height
        width: 5
        color: isAudio? 'green' : 'lawngreen'
        opacity: enabled? 0.5 : 0
        Drag.active: trimInMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction

        MouseArea {
            id: trimInMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            property double startX

            onPressed: {
                root.stopScrolling = true
                startX = mapToItem(null, x, y).x
                originalX = 0 // reusing originalX to accumulate delta for bubble help
                parent.anchors.left = undefined
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.left = clipRoot.left
                clipRoot.trimmedIn(clipRoot)
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var newX = mapToItem(null, x, y).x
                    var delta = Math.round((newX - startX) / timeScale)
                    if (Math.abs(delta) > 0) {
                        originalX += delta
                        clipRoot.trimmingIn(clipRoot, delta, mouse)
                        startX = newX
                    }
                }
            }
        }
    }
    Rectangle {
        id: trimOut
        visible: metadata !== null && metadata.keyframes.allowTrim
        enabled: !isBlank
        anchors.right: parent.right
        anchors.rightMargin: 0
        height: parent.height
        width: 5
        color: 'red'
        opacity: enabled? 0.5 : 0
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
                        originalX += delta
                        clipRoot.trimmingOut(clipRoot, delta, mouse)
                        duration = newDuration
                    }
                }
            }
        }
    }
    Menu {
        id: menu
        MenuItem {
            visible: !isBlank && settings.timelineShowWaveforms
            text: qsTr('Rebuild Audio Waveform')
            onTriggered: producer.remakeAudioLevels()
        }
    }
}
