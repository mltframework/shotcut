/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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
    property string hash: ''
    property double speed: 1.0
    property string audioIndex: ''
    property bool isTrackMute: false

    signal clicked(var clip, var mouse)
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

    border.color: (selected || Drag.active || trackIndex != originalTrackIndex)? 'red' : 'black'
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
        generateWaveform(false)
    }

    function generateWaveform(force) {
        if (!waveform.visible && !force) return
        // This is needed to make the model have the correct count.
        // Model as a property expression is not working in all cases.
        waveformRepeater.model = Math.ceil(waveform.innerWidth / waveform.maxWidth)
        for (var i = 0; i < waveformRepeater.count; i++)
            waveformRepeater.itemAt(0).update()
    }

    function imagePath(time) {
        if (isAudio || isBlank || isTransition) {
            return ''
        } else {
            return 'image://thumbnail/' + hash + '/' + mltService + '/' + clipResource + '#' + time
        }
    }

    onAudioLevelsChanged: generateWaveform(false)

    Image {
        id: outThumbnail
        visible: !isBlank && settings.timelineShowThumbnails && parent.height > 20 && x > inThumbnail.width
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.rightMargin: parent.border.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 2
        width: height * 16.0/9.0
        fillMode: Image.PreserveAspectFit
        source: imagePath(outPoint)
    }

    Image {
        id: inThumbnail
        visible: !isBlank && settings.timelineShowThumbnails && parent.height > 20
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.leftMargin: parent.border.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 2
        width: height * 16.0/9.0
        fillMode: Image.PreserveAspectFit
        source: imagePath(inPoint)
    }

    Shotcut.TimelineTransition {
        visible: isTransition
        anchors.fill: parent
        property var color: isAudio? 'darkseagreen' : root.shotcutBlue
        colorA: color
        colorB: clipRoot.selected ? Qt.darker(color) : Qt.lighter(color)
    }

    Row {
        id: waveform
        visible: !isBlank && settings.timelineShowWaveforms && (parseInt(audioIndex) > -1 || audioIndex === 'all')
        height: (isAudio || parent.height <= 20)? parent.height : parent.height / 2
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: parent.border.width
        opacity: isTrackMute ? 0.2 : 0.7
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
        visible: waveform.visible && !isTransition
        height: 1
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: parent.border.width
        anchors.bottomMargin: waveform.height * 0.9
        color: Qt.darker(parent.color)
        opacity: waveform.opacity
    }

    Rectangle {
        // text background
        color: 'lightgray'
        visible: !isBlank && !isTransition
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
        visible: !isBlank && !isTransition
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

    Rectangle {
        // text background
        color: 'lightgray'
        visible: labelRight.visible
        opacity: 0.7
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: parent.border.width
        anchors.rightMargin: parent.border.width +
            ((isAudio || !settings.timelineShowThumbnails) ? 0 : outThumbnail.width) + 2
        width: labelRight.width + 2
        height: labelRight.height
    }

    Text {
        id: labelRight
        text: clipName
        visible: !isBlank && !isTransition && parent.width > ((settings.timelineShowThumbnails? 2 * outThumbnail.width : 0) + 3 * label.width)
        font.pointSize: 8
        anchors {
            top: parent.top
            right: parent.right
            topMargin: parent.border.width + 1
            rightMargin: parent.border.width +
                ((isAudio || !settings.timelineShowThumbnails) ? 0 : outThumbnail.width) + 3
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
        enabled: isBlank
        acceptedButtons: Qt.RightButton
        onClicked: {
            timeline.position = timeline.position // pause
            menu.show()
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: !isBlank
        acceptedButtons: Qt.LeftButton
        drag.target: parent
        drag.axis: Drag.XAxis
        property int startX
        onPressed: {
            if (!doubleClickTimer.running) {
                doubleClickTimer.restart()
                doubleClickTimer.isFirstRelease = true
            }
            root.stopScrolling = true
            originalX = parent.x
            originalTrackIndex = trackIndex
            originalClipIndex = index
            startX = parent.x
            clipRoot.forceActiveFocus();
            clipRoot.clicked(clipRoot, mouse)
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
            if (!doubleClickTimer.isFirstRelease && doubleClickTimer.running) {
                // double click
                timeline.position = Math.round(clipRoot.x / multitrack.scaleFactor)
                doubleClickTimer.stop()
            } else {
                // single click
                doubleClickTimer.isFirstRelease = false

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
        Timer {
            id: doubleClickTimer
            interval: 500 //ms
            property bool isFirstRelease
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            propagateComposedEvents: true
            cursorShape: (trimInMouseArea.drag.active || trimOutMouseArea.drag.active)? Qt.SizeHorCursor :
                (fadeInMouseArea.drag.active || fadeOutMouseArea.drag.active)? Qt.PointingHandCursor :
                drag.active? Qt.ClosedHandCursor :
                isBlank? Qt.ArrowCursor : Qt.OpenHandCursor
            onClicked: {
                timeline.position = timeline.position // pause
                clipRoot.forceActiveFocus();
                clipRoot.clicked(clipRoot, mouse)
                menu.show()
            }
        }
    }

    Shotcut.TimelineTriangle {
        id: fadeInTriangle
        visible: !isBlank && !isTransition
        width: parent.fadeIn * timeScale
        height: parent.height - parent.border.width * 2
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: parent.border.width
        opacity: 0.5
    }
    Rectangle {
        id: fadeInControl
        enabled: !isBlank && !isTransition
        anchors.left: fadeInTriangle.right
        anchors.top: fadeInTriangle.top
        anchors.leftMargin: Math.min(clipRoot.width - fadeInTriangle.width - width, 0)
        anchors.topMargin: -3
        width: 14
        height: 14
        radius: 7
        z: 1
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
            drag.minimumX: 0
            drag.maximumX: clipRoot.width
            property int startX
            property int startFadeIn
            onEntered: parent.opacity = 0.7
            onExited: parent.opacity = 0
            onPressed: {
                root.stopScrolling = true
                startX = parent.x
                startFadeIn = fadeIn
                parent.anchors.left = undefined
                parent.opacity = 1
                // trackRoot.clipSelected(clipRoot, trackRoot) TODO
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.left = fadeInTriangle.right
                bubbleHelp.hide()
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((parent.x - startX) / timeScale)
                    var duration = Math.min(Math.max(0, startFadeIn + delta), clipDuration)
                    timeline.fadeIn(trackIndex, index, duration)

                    // Show fade duration as time in a "bubble" help.
                    var s = application.timecode(duration)
                    bubbleHelp.show(clipRoot.x, trackRoot.y + clipRoot.height, s.substring(6))
                }
            }
            onDoubleClicked: timeline.fadeIn(trackIndex, index, (fadeIn > 0) ? 0 : Math.round(profile.fps))
        }
        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: fadeInMouseArea.containsMouse
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
        id: fadeOutTriangle
        visible: !isBlank && !isTransition
        width: parent.fadeOut * timeScale
        height: parent.height - parent.border.width * 2
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: parent.border.width
        opacity: 0.5
        transform: Scale { xScale: -1; origin.x: fadeOutTriangle.width / 2}
    }
    Rectangle {
        id: fadeOutControl
        enabled: !isBlank && !isTransition
        anchors.right: fadeOutTriangle.left
        anchors.top: fadeOutTriangle.top
        anchors.rightMargin: Math.min(clipRoot.width - fadeOutTriangle.width - width, 0)
        anchors.topMargin: -3
        width: 14
        height: 14
        radius: 7
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
            drag.minimumX: -width - 1
            drag.maximumX: clipRoot.width
            property int startX
            property int startFadeOut
            onEntered: parent.opacity = 0.7
            onExited: parent.opacity = 0
            onPressed: {
                root.stopScrolling = true
                startX = parent.x
                startFadeOut = fadeOut
                parent.anchors.right = undefined
                parent.opacity = 1
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.right = fadeOutTriangle.left
                bubbleHelp.hide()
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((startX - parent.x) / timeScale)
                    var duration = Math.min(Math.max(0, startFadeOut + delta), clipDuration)
                    timeline.fadeOut(trackIndex, index, duration)

                    // Show fade duration as time in a "bubble" help.
                    var s = application.timecode(duration)
                    bubbleHelp.show(clipRoot.x + clipRoot.width, trackRoot.y + clipRoot.height, s.substring(6))
                }
            }
            onDoubleClicked: timeline.fadeOut(trackIndex, index, (fadeOut > 0) ? 0 : Math.round(profile.fps))
        }
        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: fadeOutMouseArea.containsMouse
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
            property double startX

            onPressed: {
                root.stopScrolling = true
                startX = mapToItem(null, x, y).x
                originalX = 0 // reusing originalX to accumulate delta for bubble help
                parent.anchors.left = undefined
                originalClipIndex = index
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.left = clipRoot.left
                clipRoot.trimmedIn(clipRoot)
                parent.opacity = 0
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
        function show() {
            mergeItem.enabled = timeline.mergeClipWithNext(trackIndex, index, true)
            popup()
        }
        MenuItem {
            enabled: !isBlank && !isTransition
            text: qsTr('Cut') + (application.OS === 'OS X'? '    ⌘X' : ' (Ctrl+X)')
            onTriggered: {
                if (!trackRoot.isLocked) {
                    timeline.removeSelection(true)
                } else {
                    root.pulseLockButtonOnTrack(currentTrack)
                }
            }
        }
        MenuItem {
            enabled: !isBlank && !isTransition
            text: qsTr('Copy') + (application.OS === 'OS X'? '    ⌘C' : ' (Ctrl+C)')
            onTriggered: timeline.copy(trackIndex, index)
        }
        MenuItem {
            text: qsTr('Remove') + (isBlank? '' : (application.OS === 'OS X'? '    X' : ' (X)'))
            onTriggered: isBlank? timeline.remove(trackIndex, index) : timeline.removeSelection(false)
        }
        MenuItem {
            enabled: !isBlank && !isTransition
            text: qsTr('Split At Playhead') + (application.OS === 'OS X'? '    S' : ' (S)')
            onTriggered: timeline.splitClip(trackIndex, index)
        }
        Menu {
            title: qsTr('More')
            MenuItem {
                enabled: !isBlank
                text: qsTr('Lift') + (application.OS === 'OS X'? '    Z' : ' (Z)')
                onTriggered: timeline.liftSelection()
            }
            MenuItem {
                enabled: !isTransition
                text: qsTr('Replace') + (application.OS === 'OS X'? '    R' : ' (R)')
                onTriggered: timeline.replace(trackIndex, index)
            }
            MenuItem {
                id: mergeItem
                text: qsTr('Merge with next clip')
                onTriggered: timeline.mergeClipWithNext(trackIndex, index, false)
            }
            MenuItem {
                enabled: !isBlank && !isTransition && !isAudio && (parseInt(audioIndex) > -1 || audioIndex === 'all')
                text: qsTr('Detach Audio')
                onTriggered: timeline.detachAudio(trackIndex, index)
            }
            MenuItem {
                enabled: !isBlank && !isTransition && settings.timelineShowThumbnails && !isAudio
                text: qsTr('Update Thumbnails')
                onTriggered: {
                    var s = inThumbnail.source.toString()
                    if (s.substring(s.length - 1) !== '!') {
                        inThumbnail.source = s + '!'
                        resetThumbnailsSourceTimer.restart()
                    }
                    s = outThumbnail.source.toString()
                    if (s.substring(s.length - 1) !== '!') {
                        outThumbnail.source = s + '!'
                        resetThumbnailsSourceTimer.restart()
                    }
                }
            }
            MenuItem {
                enabled: !isBlank && !isTransition && settings.timelineShowWaveforms
                text: qsTr('Rebuild Audio Waveform')
                onTriggered: timeline.remakeAudioLevels(trackIndex, index)
            }
        }
        MenuItem {
            enabled: !isBlank
            text: qsTr('Properties')
            onTriggered: {
                clipRoot.forceActiveFocus()
                clipRoot.clicked(clipRoot, null)
                timeline.openProperties()
            }
        }
        MenuItem {
            text: qsTr('Cancel')
            onTriggered: menu.dismiss()
        }
    }

    Timer {
        id: resetThumbnailsSourceTimer
        interval: 5000
        onTriggered: {
            var s = inThumbnail.source.toString()
            if (s.substring(s.length - 1) === '!') {
                inThumbnail.source = s.substring(0, s.length - 1)
            }
            s = inThumbnail.source.toString()
            if (s.substring(s.length - 1) === '!') {
                inThumbnail.source = s.substring(0, s.length - 1)
            }
        }
    }
}
