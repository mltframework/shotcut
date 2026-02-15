/*
 * Copyright (c) 2013-2026 Meltytech, LLC
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
import QtQuick
import QtQuick.Controls
import Shotcut.Controls as Shotcut

Rectangle {
    id: clipRoot

    property string clipName: ''
    property string clipComment: ''
    property string clipResource: ''
    property string mltService: ''
    property int inPoint: 0
    property int outPoint: 0
    property int clipDuration: 0
    property bool isBlank: false
    property bool isAudio: false
    property bool isTransition: false
    property bool isFiltered: false
    property int fadeIn: 0
    property int fadeOut: 0
    property double gain: 0
    property bool adjustGainEnabled: true
    property int trackIndex
    property int originalTrackIndex: trackIndex
    property int originalClipIndex: index
    property int originalX: x
    property bool selected: false
    property string hash: ''
    property double speed: 1
    property string audioIndex: ''
    property int group: -1
    property bool isTrackMute: false
    property bool elided: (width < 15) || (x + width < tracksFlickable.contentX) || (x > tracksFlickable.contentX + tracksFlickable.width) || (y + height < 0) || (y > tracksFlickable.contentY + tracksFlickable.contentHeight)
    property color clipColor: isBlank ? 'transparent' : isTransition ? 'mediumpurple' : isAudio ? 'darkseagreen' : root.shotcutBlue

    signal clicked(var clip, var mouse)
    signal clipRightClicked(var clip, var mouse)
    signal moved(var clip)
    signal dragged(var clip, var mouse)
    signal dropped(var clip)
    signal draggedToTrack(var clip, int direction)
    signal trimmingIn(var clip, real delta, var mouse)
    signal trimmedIn(var clip)
    signal trimmingOut(var clip, real delta, var mouse)
    signal trimmedOut(var clip)

    function reparent(track) {
        parent = track;
        isAudio = track.isAudio;
        height = track.height;
        generateWaveform(false);
    }

    function generateWaveform(force) {
        if (!waveform.visible && !force)
            return;

        // This is needed to make the model have the correct count.
        // Model as a property expression is not working in all cases.
        waveformRepeater.model = Math.ceil(clipRoot.width / waveform.maxWidth);
        for (let i = 0; i < waveformRepeater.count; i++)
            waveformRepeater.itemAt(i).update();
    }

    function updateThumbnails() {
        var s = inThumbnail.source.toString();
        if (s.substring(s.length - 1) !== '!') {
            inThumbnail.source = s + '!';
            resetThumbnailsSourceTimer.restart();
        }
        s = outThumbnail.source.toString();
        if (s.substring(s.length - 1) !== '!') {
            outThumbnail.source = s + '!';
            resetThumbnailsSourceTimer.restart();
        }
    }

    function imagePath(time) {
        if (isAudio || isBlank || isTransition)
            return '';
        else
            return 'image://thumbnail/' + hash + '/' + mltService + '/' + clipResource + '#' + time;
    }

    border.color: (selected || Drag.active || trackIndex != originalTrackIndex) ? group < 0 ? 'red' : 'white' : 'black'
    border.width: isBlank && !selected ? 0 : 1
    clip: true
    Drag.active: mouseArea.drag.active
    Drag.proposedAction: Qt.MoveAction
    opacity: Drag.active ? 0.5 : 1
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
                color: Qt.darker(clipColor)
            }
        }
    ]

    MouseArea {
        id: clipNameHover

        anchors.fill: parent
        enabled: !isBlank && !mouseArea.drag.active && !trimInMouseArea.drag.active && !trimOutMouseArea.drag.active && !fadeInMouseArea.drag.active && !fadeOutMouseArea.drag.active
        propagateComposedEvents: true
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        onEntered: {
            nameHoverTimer.start();
        }
        onPositionChanged: {
            bubbleHelp.hide();
            nameHoverTimer.restart();
        }
        onExited: {
            nameHoverTimer.stop();
            bubbleHelp.hide();
        }

        Timer {
            id: nameHoverTimer

            interval: 1000
            onTriggered: {
                // Limit text to 200 pixels
                var text = fontMetrics.elidedText(clipName.trim(), Qt.ElideRight, 200);
                if (text.length > 0) {
                    var commentLines = clipComment.split('\n');
                    if (commentLines.length > 0) {
                        var comment = fontMetrics.elidedText(commentLines[0].trim(), Qt.ElideRight, 200);
                        if (comment.length > 0)
                            text += '<br>' + comment;
                    }
                    text += '<br>' + application.timeFromFrames(clipDuration);
                    bubbleHelp.show(text);
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: isBlank
        acceptedButtons: Qt.RightButton
        onClicked: {
            timeline.position = timeline.position; // pause
            clipRoot.clicked(clipRoot, mouse);
            clipRoot.clipRightClicked(clipRoot, mouse);
        }
    }

    MouseArea {
        id: mouseArea

        property int startX

        anchors.fill: parent
        enabled: !isBlank
        acceptedButtons: Qt.LeftButton
        drag.target: parent
        drag.axis: Drag.XAxis
        onPressed: mouse => {
            if (!doubleClickTimer.running) {
                doubleClickTimer.restart();
                doubleClickTimer.isFirstRelease = true;
            }
            root.stopScrolling = true;
            originalX = parent.x;
            originalTrackIndex = trackIndex;
            originalClipIndex = index;
            startX = parent.x;
            clipRoot.forceActiveFocus();
            clipRoot.clicked(clipRoot, mouse);
        }
        onPositionChanged: mouse => {
            if (mouse.y < 0 && trackIndex > 0)
                parent.draggedToTrack(clipRoot, -1);
            else if (mouse.y > height && (trackIndex + 1) < root.trackCount)
                parent.draggedToTrack(clipRoot, 1);
            parent.dragged(clipRoot, mouse);
        }
        onReleased: mouse => {
            root.stopScrolling = false;
            if (!doubleClickTimer.isFirstRelease && doubleClickTimer.running) {
                // double click
                timeline.position = Math.round(clipRoot.x / multitrack.scaleFactor);
                doubleClickTimer.stop();
            } else {
                // single click
                doubleClickTimer.isFirstRelease = false;
                parent.y = 0;
                var delta = parent.x - startX;
                if (Math.abs(delta) >= 1 || trackIndex !== originalTrackIndex) {
                    parent.moved(clipRoot);
                    originalX = parent.x;
                    originalTrackIndex = trackIndex;
                } else if (mouse.modifiers === Qt.NoModifier) {
                    parent.dropped(clipRoot);
                    clipRoot.clicked(clipRoot, mouse);
                }
            }
        }

        Timer {
            id: doubleClickTimer

            property bool isFirstRelease

            interval: 500 //ms
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            propagateComposedEvents: true
            cursorShape: (trimInMouseArea.drag.active || trimOutMouseArea.drag.active) ? Qt.SizeHorCursor : (fadeInMouseArea.drag.active || fadeOutMouseArea.drag.active) ? Qt.PointingHandCursor : mouseArea.drag.active ? Qt.ClosedHandCursor : isBlank ? Qt.ArrowCursor : Qt.OpenHandCursor
            onClicked: mouse => {
                timeline.position = timeline.position; // pause
                clipRoot.forceActiveFocus();
                clipRoot.clicked(clipRoot, mouse);
                clipRoot.clipRightClicked(clipRoot, mouse);
            }
        }
    }

    Image {
        id: outThumbnail

        visible: !elided && !isBlank && settings.timelineShowThumbnails && parent.height > 20 && x > inThumbnail.width
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.rightMargin: parent.border.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 2
        width: height * 16 / 9
        fillMode: Image.PreserveAspectFit
        source: imagePath(outPoint)
    }

    Image {
        id: inThumbnail

        visible: !elided && !isBlank && settings.timelineShowThumbnails && parent.height > 20
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.leftMargin: parent.border.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 2
        width: height * 16 / 9
        fillMode: Image.PreserveAspectFit
        source: imagePath(inPoint)
    }

    Shotcut.TimelineTransition {
        property var color: isAudio ? 'darkseagreen' : root.shotcutBlue

        visible: !elided && isTransition
        anchors.fill: parent
        colorA: color
        colorB: clipRoot.selected ? Qt.darker(color) : Qt.lighter(color)
    }

    Row {
        id: waveform

        readonly property int maxWidth: Math.max(application.maxTextureSize / 2, 2048)

        visible: !elided && !isBlank && settings.timelineShowWaveforms && (parseInt(audioIndex) > -1 || audioIndex === 'all')
        height: (isAudio || parent.height <= 20) ? parent.height : parent.height / 2
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: parent.border.width
        opacity: isTrackMute ? 0.2 : 0.7

        Repeater {
            id: waveformRepeater

            model: Math.ceil(clipRoot.width / waveform.maxWidth)

            Shotcut.TimelineWaveform {
                property int channels: 2

                trackIndex: clipRoot.trackIndex
                clipIndex: clipRoot.originalClipIndex
                width: Math.min(clipRoot.width, waveform.maxWidth)
                height: waveform.height
                fillColor: clipColor
                inPoint: Math.round((clipRoot.inPoint + index * waveform.maxWidth / timeScale) * speed) * channels
                outPoint: inPoint + Math.round(width / timeScale * speed) * channels
                active: ((clipRoot.x + x + width) > tracksFlickable.contentX) && ((clipRoot.x + x) < tracksFlickable.contentX + tracksFlickable.width) && ((trackRoot.y + y + height) > tracksFlickable.contentY) && ((trackRoot.y + y) < tracksFlickable.contentY + tracksFlickable.height) // top edge

                Component.onCompleted: connectToModel()
            }
        }
    }

    Rectangle {
        id: audioPeakLine

        width: parent.width - parent.border.width * 2
        visible: !elided && waveform.visible && !isTransition && parent.height > 25
        height: audioPeakMouseArea.drag.active ? 2 : 1
        anchors.left: parent.left
        anchors.leftMargin: parent.border.width
        property real yOffset: 0
        y: clipRoot.height - waveform.height * Math.min((gain + 72) / 80, 1) + yOffset
        color: audioPeakMouseArea.enabled ? audioPeakMouseArea.drag.active ? Qt.lighter(parent.color) : Qt.darker(clipColor) : Qt.darker(parent.color)
        opacity: waveform.opacity

        MouseArea {
            id: audioPeakMouseArea

            enabled: adjustGainEnabled && settings.timelineAdjustGain
            anchors.fill: parent
            anchors.topMargin: -2
            anchors.bottomMargin: -2
            cursorShape: enabled ? Qt.SizeVerCursor : mouseArea.cursorShape
            drag.axis: Drag.YAxis
            drag.target: parent
            drag.minimumY: clipRoot.height - waveform.height
            drag.maximumY: clipRoot.height
            onPositionChanged: {
                let y = parent.y - (clipRoot.height - waveform.height);
                let value = (waveform.height - Math.max(0, Math.min(y, waveform.height))) / waveform.height;
                let gain = -80 /* dB */ * (1 - value) + 10;
                if (!timeline.changeGain(trackIndex, index, gain)) {
                    // force y's expression to re-evaluate
                    parent.yOffset = 0.1;
                    parent.yOffset = 0;
                }
            }
            onDoubleClicked: timeline.changeGain(trackIndex, index, 0);
        }
    }

    Rectangle {
        id: leftLabelBackground

        color: 'lightgray'
        visible: !elided && !isBlank && !isTransition
        opacity: 0.7
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: parent.border.width
        anchors.leftMargin: parent.border.width + ((isAudio || !settings.timelineShowThumbnails) ? filtersIcon.enabledWidth : inThumbnail.width + filtersIcon.width)
        width: label.width + 2
        height: label.height
    }

    Text {
        id: label

        text: clipName
        visible: !elided && !isBlank && !isTransition
        font.pointSize: 8
        color: 'black'

        anchors {
            top: parent.top
            left: leftLabelBackground.left
            topMargin: parent.border.width + 1
            leftMargin: parent.border.width + 1
        }
    }

    Rectangle {
        id: rightLabelBackground

        color: 'lightgray'
        visible: labelRight.visible
        opacity: 0.7
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: parent.border.width
        anchors.rightMargin: parent.border.width + ((isAudio || !settings.timelineShowThumbnails) ? 0 : outThumbnail.width) + 2
        width: labelRight.width + 2
        height: labelRight.height
    }

    Text {
        id: labelRight

        text: clipName
        visible: !elided && !isBlank && !isTransition && parent.width > ((settings.timelineShowThumbnails ? 2 * outThumbnail.width : 0) + 3 * label.width)
        font.pointSize: 8
        color: 'black'

        anchors {
            top: parent.top
            right: rightLabelBackground.right
            topMargin: parent.border.width + 1
            rightMargin: parent.border.width
        }
    }

    ToolButton {
        id: filtersIcon

        property real enabledWidth: enabled ? 2 * width : width

        visible: !elided && !isBlank && isFiltered
        icon.source: 'qrc:///icons/light/32x32/view-filter.png'
        icon.width: 16
        icon.height: 16
        opacity: 0.7
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.leftMargin: parent.border.width + ((isAudio || !settings.timelineShowThumbnails) ? (enabled ? width : 0) : inThumbnail.width)
        width: visible ? label.height : 0
        height: label.height
        padding: 0
        focusPolicy: Qt.NoFocus
        enabled: false
        onClicked: {
            clipRoot.clicked(clipRoot, null);
            timeline.filteredClicked();
        }
        background: Rectangle {
            implicitWidth: 16
            implicitHeight: 16
            color: 'lightgray'
            opacity: 1
        }
    }

    Shotcut.TimelineTriangle {
        id: fadeInTriangle

        visible: !elided && !isBlank && !isTransition
        width: parent.fadeIn * timeScale
        height: parent.height - parent.border.width * 2
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: parent.border.width
        opacity: 0.5
    }

    Rectangle {
        id: fadeInControl

        enabled: !elided && !isBlank && !isTransition
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
            // trackRoot.clipSelected(clipRoot, trackRoot) TODO
            id: fadeInMouseArea

            property int startX
            property int startFadeIn

            anchors.fill: parent
            hoverEnabled: parent.enabled
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            drag.minimumX: 0
            drag.maximumX: clipRoot.width
            onEntered: parent.opacity = 0.7
            onExited: parent.opacity = 0
            onPressed: {
                root.stopScrolling = true;
                startX = parent.x;
                startFadeIn = fadeIn;
                parent.anchors.left = undefined;
                parent.opacity = 1;
            }
            onReleased: {
                root.stopScrolling = false;
                parent.anchors.left = fadeInTriangle.right;
                bubbleHelp.hide();
            }
            onPositionChanged: mouse => {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((parent.x - startX) / timeScale);
                    var duration = Math.min(Math.max(0, startFadeIn + delta), clipDuration);
                    timeline.fadeIn(trackIndex, index, duration);
                    // Show fade duration as time in a "bubble" help.
                    var s = application.timeFromFrames(duration);
                    bubbleHelp.show(s.substring(6));
                }
            }
            onDoubleClicked: timeline.fadeIn(trackIndex, index, (fadeIn > 0) ? 0 : Math.round(profile.fps))
        }

        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: fadeInMouseArea.containsMouse

            NumberAnimation {
                from: 1
                to: 1.5
                duration: 250
                easing.type: Easing.InOutQuad
            }

            NumberAnimation {
                from: 1.5
                to: 1
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    Shotcut.TimelineTriangle {
        id: fadeOutTriangle

        visible: !elided && !isBlank && !isTransition
        width: parent.fadeOut * timeScale
        height: parent.height - parent.border.width * 2
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: parent.border.width
        opacity: 0.5

        transform: Scale {
            xScale: -1
            origin.x: fadeOutTriangle.width / 2
        }
    }

    Rectangle {
        id: fadeOutControl

        enabled: !elided && !isBlank && !isTransition
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

            property int startX
            property int startFadeOut

            anchors.fill: parent
            hoverEnabled: parent.enabled
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            drag.minimumX: -width - 1
            drag.maximumX: clipRoot.width
            onEntered: parent.opacity = 0.7
            onExited: parent.opacity = 0
            onPressed: {
                root.stopScrolling = true;
                startX = parent.x;
                startFadeOut = fadeOut;
                parent.anchors.right = undefined;
                parent.opacity = 1;
            }
            onReleased: {
                root.stopScrolling = false;
                parent.anchors.right = fadeOutTriangle.left;
                bubbleHelp.hide();
            }
            onPositionChanged: mouse => {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((startX - parent.x) / timeScale);
                    var duration = Math.min(Math.max(0, startFadeOut + delta), clipDuration);
                    timeline.fadeOut(trackIndex, index, duration);
                    // Show fade duration as time in a "bubble" help.
                    var s = application.timeFromFrames(duration);
                    bubbleHelp.show(s.substring(6));
                }
            }
            onDoubleClicked: timeline.fadeOut(trackIndex, index, (fadeOut > 0) ? 0 : Math.round(profile.fps))
        }

        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: fadeOutMouseArea.containsMouse

            NumberAnimation {
                from: 1
                to: 1.5
                duration: 250
                easing.type: Easing.InOutQuad
            }

            NumberAnimation {
                from: 1.5
                to: 1
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    Rectangle {
        id: trimIn

        enabled: trimInMouseArea.drag.active || (!elided && !isBlank && !isTransition)
        anchors.left: parent.left
        anchors.leftMargin: 0
        height: parent.height
        width: 5
        color: isAudio ? 'green' : 'lawngreen'
        opacity: 0
        Drag.active: trimInMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction

        MouseArea {
            id: trimInMouseArea

            property double startX

            anchors.fill: parent
            hoverEnabled: parent.enabled
            cursorShape: enabled ? Qt.SizeHorCursor : Qt.ArrowCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            onPressed: {
                root.stopScrolling = true;
                startX = mapToItem(null, x, y).x;
                originalX = 0; // reusing originalX to accumulate delta for bubble help
                parent.anchors.left = undefined;
                originalClipIndex = index;
            }
            onReleased: {
                root.stopScrolling = false;
                parent.anchors.left = clipRoot.left;
                clipRoot.trimmedIn(clipRoot);
                parent.opacity = 0;
            }
            onPositionChanged: mouse => {
                if (mouse.buttons === Qt.LeftButton) {
                    var newX = mapToItem(null, x, y).x;
                    var delta = Math.round((newX - startX) / timeScale);
                    if (Math.abs(delta) > 0) {
                        originalX += delta;
                        clipRoot.trimmingIn(clipRoot, delta, mouse);
                        startX = newX;
                    }
                }
            }
            onEntered: parent.opacity = 0.5
            onExited: parent.opacity = 0
        }
    }

    Rectangle {
        id: trimOut

        enabled: trimOutMouseArea.drag.active || (!elided && !isBlank && !isTransition)
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

            property int duration

            anchors.fill: parent
            hoverEnabled: parent.enabled
            cursorShape: enabled ? Qt.SizeHorCursor : Qt.ArrowCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            onPressed: {
                root.stopScrolling = true;
                duration = clipDuration;
                originalX = 0; // reusing originalX to accumulate delta for bubble help
                parent.anchors.right = undefined;
            }
            onReleased: {
                root.stopScrolling = false;
                parent.anchors.right = clipRoot.right;
                clipRoot.trimmedOut(clipRoot);
            }
            onPositionChanged: mouse => {
                if (mouse.buttons === Qt.LeftButton) {
                    var newDuration = Math.round((parent.x + parent.width) / timeScale);
                    var delta = duration - newDuration;
                    if (Math.abs(delta) > 0) {
                        originalX += delta;
                        clipRoot.trimmingOut(clipRoot, delta, mouse);
                        duration = newDuration;
                    }
                }
            }
            onEntered: parent.opacity = 0.5
            onExited: parent.opacity = 0
        }
    }

    Timer {
        id: resetThumbnailsSourceTimer

        interval: 5000
        onTriggered: {
            var s = inThumbnail.source.toString();
            if (s.substring(s.length - 1) === '!')
                inThumbnail.source = s.substring(0, s.length - 1);
            s = inThumbnail.source.toString();
            if (s.substring(s.length - 1) === '!')
                inThumbnail.source = s.substring(0, s.length - 1);
        }
    }

    gradient: Gradient {
        GradientStop {
            id: gradientStop

            position: 0
            color: Qt.lighter(clipColor)
        }

        GradientStop {
            id: gradientStop2

            position: 1
            color: clipColor
        }
    }
}
