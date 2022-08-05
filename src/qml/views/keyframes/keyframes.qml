/*
 * Copyright (c) 2017-2022 Meltytech, LLC
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
import QtQml.Models 2.12
import QtQuick.Controls 2.12
import Shotcut.Controls 1.0 as Shotcut
import QtGraphicalEffects 1.12
import 'Keyframes.js' as Logic

Rectangle {
    id: root
    width: 400
    SystemPalette { id: activePalette }
    color: activePalette.window

    property int selectedIndex: -1
    property int headerWidth: 140
    property int currentTrack: 0
    property color selectedTrackColor: Qt.rgba(0.8, 0.8, 0, 0.3)
    property bool stopScrolling: false
    property color shotcutBlue: Qt.rgba(23/255, 92/255, 118/255, 1.0)
    property double timeScale: 1.0
    property var selection: []
    property alias paramRepeater: parametersRepeater

    signal rightClicked()
    signal keyframeRightClicked()
    signal clipRightClicked()

    onTimeScaleChanged: redrawWaveforms()

    function redrawWaveforms() {
        Logic.scrollIfNeeded()
        beforeClip.generateWaveform()
        activeClip.generateWaveform()
        afterClip.generateWaveform()
    }

    function setZoom(value, targetX) {
        if (!targetX)
            targetX = tracksFlickable.contentX + tracksFlickable.width / 2
        var offset = targetX - tracksFlickable.contentX
        var before = timeScale
        timeScale = Math.pow(value, 3) + 0.01
        tracksFlickable.contentX = Logic.clamp((targetX * timeScale / before) - offset, 0, Logic.scrollMax().x)
    }

    Timer {
        id: scrollZoomTimer
        interval: 100
        onTriggered: {
            Logic.scrollIfNeeded(true)
        }
    }

    function adjustZoom(by, targetX) {
        var value = Math.pow(timeScale - 0.01, 1.0 / 3.0)
        setZoom(value + by, targetX)
        if (settings.timelineScrollZoom)
            scrollZoomTimer.restart()
    }

    function zoomIn() {
        adjustZoom(0.0625)
    }

    function zoomOut() {
        adjustZoom(-0.0625)
    }

    function zoomToFit() {
        setZoom(Math.pow((tracksFlickable.width - 50) * timeScale / tracksContainer.width - 0.01, 1/3))
        tracksFlickable.contentX = 0
    }

    function resetZoom() {
        setZoom(1.0)
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: root.rightClicked()
    }

    Row {
        anchors.fill: parent
        Column {
            z: 1

            Rectangle {
                // Padding between toolbar and track headers.
                width: headerWidth
                height: ruler.height
                color: activePalette.window
                z: 1
            }
            Flickable {
                // Non-slider scroll area for the track headers.
                contentY: tracksFlickable.contentY
                width: headerWidth
                height: trackHeaders.height
                interactive: false

                Column {
                    id: trackHeaders

                    ParameterHead {
                        id: clipHead
                        visible: metadata !== null
                        trackName: metadata !== null? metadata.name : ''
                        width: headerWidth
                        height: Logic.trackHeight(true)
                        onRightClicked: root.rightClicked()
                    }
                    Repeater {
                        id: trackHeaderRepeater
                        model: parameters
                        ParameterHead {
                            trackName: model.name
                            delegateIndex: index
                            isCurve: model.isCurve
                            width: headerWidth
                            height: Logic.trackHeight(isCurve)
                            current: index === currentTrack
                            onClicked: currentTrack = index
                            onRightClicked: root.rightClicked()
                        }
                    }
                }
                Rectangle {
                    // thin dividing line between headers and tracks
                    visible: metadata !== null
                    color: activePalette.windowText
                    width: 1
                    x: parent.x + parent.width
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                }
            }
        }
        MouseArea {
            id: tracksArea
            width: root.width - headerWidth
            height: root.height

            // This provides continuous scrubbing and scimming at the left/right edges.
            focus: true
            hoverEnabled: true
            onClicked: {
                producer.position = (tracksFlickable.contentX + mouse.x) / timeScale
                bubbleHelp.hide()
            }
            onWheel: Logic.onMouseWheel(wheel)
            onDoubleClicked: {
                // Figure out which parameter row that is in.
                for (var i = 0; i < parametersRepeater.count; i++) {
                    var parameter = parametersRepeater.itemAt(i)
                    // Only for parameters with curves.
                    if (parameter.isCurve) {
                        var point = tracksArea.mapToItem(parameter, mouse.x, mouse.y)
                        var position = Math.round(point.x / timeScale) - (filter.in - producer.in)
                        var trackHeight = parameter.height
                        var interpolation = 1
                        // Get the interpolation from the previous keyframe if any.
                        for (var j = 0; j < parameter.getKeyframeCount(); j++) {
                            var k = parameter.getKeyframe(j)
                            if (k.position - (filter.in - producer.in) < position)
                                interpolation = k.interpolation
                            else
                                break
                        }
                        // If click position is within range.
                        if (position >= 0 && position < filter.duration
                            && point.y > 0 && point.y < trackHeight) {
                            // Determine the value to set.
                            var keyframeHeight = 10
                            var trackValue = Math.min(Math.max(0, 1.0 - (point.y - keyframeHeight/2) / (trackHeight - keyframeHeight)), 1.0)
                            var value = parameter.minimum + trackValue * (parameter.maximum - parameter.minimum)
                            //console.log('clicked parameter ' + i + ' frame ' + position + ' trackValue ' + trackValue + ' value ' + value)
                            parameters.addKeyframe(i, value, position, interpolation)
                            break
                        }
                    }
                }
            }

            property bool scim: false
            onReleased: scim = false
            onExited: scim = false
            onPositionChanged: {
                if (mouse.modifiers === (Qt.ShiftModifier | Qt.AltModifier) || mouse.buttons === Qt.LeftButton) {
                    producer.position = (tracksFlickable.contentX + mouse.x) / timeScale
                    bubbleHelp.hide()
                    scim = true
                }
                else
                    scim = false
            }
            Timer {
                id: scrubTimer
                interval: 25
                repeat: true
                running: parent.scim && parent.containsMouse
                         && (parent.mouseX < 50 || parent.mouseX > parent.width - 50)
                         && (producer.position * timeScale >= 50)
                onTriggered: {
                    if (parent.mouseX < 50)
                        producer.position -= 10
                    else
                        producer.position += 10
                }
            }

            Item {
                Flickable {
                    // Non-slider scroll area for the Ruler.
                    id: rulerFlickable
                    contentX: tracksFlickable.contentX
                    width: root.width - headerWidth
                    height: ruler.height
                    interactive: false
                    // workaround to fix https://github.com/mltframework/shotcut/issues/777
                    onContentXChanged: if (contentX === 0) contentX = tracksFlickable.contentX

                    Ruler {
                        id: ruler
                        width: producer.duration * timeScale
                    }
                }
                Flickable {
                    id: tracksFlickable
                    y: ruler.height
                    width: root.width - headerWidth - 16
                    height: root.height - ruler.height - 16
                    clip: true
                    // workaround to fix https://github.com/mltframework/shotcut/issues/777
                    onContentXChanged: rulerFlickable.contentX = contentX
                    interactive: false
                    contentWidth: tracksContainer.width + headerWidth
                    contentHeight: trackHeaders.height + 30 // 30 is padding
                    ScrollBar.horizontal: ScrollBar {
                        id: horizontalScrollBar
                        height: 16
                        policy: ScrollBar.AlwaysOn
                        visible: tracksFlickable.contentWidth > tracksFlickable.width
                        parent: tracksFlickable.parent
                        anchors.top: tracksFlickable.bottom
                        anchors.left: tracksFlickable.left
                        anchors.right: tracksFlickable.right
                        background: Rectangle { color: parent.palette.alternateBase }
                    }
                    ScrollBar.vertical: ScrollBar {
                        width: 16
                        policy: ScrollBar.AlwaysOn
                        visible: tracksFlickable.contentHeight > tracksFlickable.height
                        parent: tracksFlickable.parent
                        anchors.top: tracksFlickable.top
                        anchors.left: tracksFlickable.right
                        anchors.bottom: tracksFlickable.bottom
                        anchors.bottomMargin: -16
                        background: Rectangle { color: parent.palette.alternateBase }
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        onWheel: Logic.onMouseWheel(wheel)

                        Column {
                            Rectangle {
                                width: 1
                                visible: metadata !== null
                                height: clipHead.height
                            }
                            // These make the striped background for the tracks.
                            // It is important that these are not part of the track visual hierarchy;
                            // otherwise, the clips will be obscured by the Track's background.
                            Repeater {
                                model: parameters
                                Rectangle {
                                    width: tracksContainer.width
                                    color: (index === currentTrack)? selectedTrackColor : (index % 2)? activePalette.alternateBase : activePalette.base
                                    height: Logic.trackHeight(model.isCurve)
                                }
                            }
                        }
                        Column {
                            id: tracksContainer
                            Rectangle {
                                id: trackRoot
                                width: clipRow.width
                                height: Logic.trackHeight(true)
                                color: 'transparent'
                                Row {
                                    id: clipRow
                                    Clip {
                                        id: beforeClip
                                        visible: metadata !== null && filter !== null && filter.out > 0 && filter.in > 0
                                        isBlank: true
                                        clipResource: producer.resource
                                        mltService: producer.mlt_service
                                        inPoint: producer.in
                                        outPoint: filter !== null? filter.in - 1 : 0
                                        audioLevels: producer.audioLevels
                                        height: trackRoot.height
                                        hash: producer.hash
                                        speed: producer.speed
                                        outThumbnailVisible: false
                                        onRightClicked: root.clipRightClicked()
                                    }
                                    Clip {
                                        id: activeClip
                                        visible: metadata !== null && filter !== null && filter.out > 0
                                        clipName: producer.name
                                        clipResource: producer.resource
                                        mltService: producer.mlt_service
                                        inPoint: filter !== null? filter.in : 0
                                        outPoint: filter !== null? filter.out : 0
                                        animateIn: filter !== null? filter.animateIn : 0
                                        animateOut: filter !== null? filter.animateOut : 0
                                        audioLevels: producer.audioLevels
                                        height: trackRoot.height
                                        hash: producer.hash
                                        speed: producer.speed
                                        onTrimmingIn: {
                                            var n = filter.in + delta
                                            if (delta != 0 && n >= producer.in && n <= filter.out) {
                                                parameters.trimFilterIn(n)
                                                // Show amount trimmed as a time in a "bubble" help.
                                                var s = application.timecode(Math.abs(clip.originalX))
                                                s = '%1%2 = %3'.arg((clip.originalX < 0)? '-' : (clip.originalX > 0)? '+' : '')
                                                               .arg(s.substring(3))
                                                               .arg(application.timecode(n))
                                                bubbleHelp.show(clip.x, trackRoot.y + trackRoot.height, s)
                                            } else {
                                                clip.originalX -= delta
                                            }
                                        }
                                        onTrimmedIn: bubbleHelp.hide()
                                        onTrimmingOut: {
                                            var n = filter.out - delta
                                            if (delta != 0 && n >= filter.in && n <= producer.out) {
                                                parameters.trimFilterOut(n)
                                                // Show amount trimmed as a time in a "bubble" help.
                                                var s = application.timecode(Math.abs(clip.originalX))
                                                s = '%1%2 = %3'.arg((clip.originalX < 0)? '+' : (clip.originalX > 0)? '-' : '')
                                                               .arg(s.substring(3))
                                                               .arg(application.timecode(n))
                                                bubbleHelp.show(clip.x + clip.width, trackRoot.y + trackRoot.height, s)
                                            } else {
                                                clip.originalX -= delta
                                            }
                                        }
                                        onTrimmedOut: bubbleHelp.hide()
                                        onRightClicked: root.clipRightClicked()
                                    }
                                    Clip {
                                        id: afterClip
                                        visible: metadata !== null && filter !== null && filter.out > 0
                                        isBlank: true
                                        clipResource: producer.resource
                                        mltService: producer.mlt_service
                                        inPoint: filter !== null? filter.out + 1 : 0
                                        outPoint: producer.out
                                        audioLevels: producer.audioLevels
                                        height: trackRoot.height
                                        hash: producer.hash
                                        speed: producer.speed
                                        inThumbnailVisible: false
                                        onRightClicked: root.clipRightClicked()
                                    }
                                }
                            }

                            Repeater { id: parametersRepeater; model: parameterDelegateModel }
                        }
                    }
                }
            }

            Rectangle {
                id: cursor
                visible: producer.position > -1 && metadata !== null
                color: activePalette.text
                width: 1
                height: root.height - horizontalScrollBar.height
                x: producer.position * timeScale - tracksFlickable.contentX
                y: 0
            }
            Shotcut.TimelinePlayhead {
                id: playhead
                visible: producer.position > -1 && metadata !== null
                x: producer.position * timeScale - tracksFlickable.contentX - width/2
                y: 0
                width: 16
                height: 8
            }
        }
    }

    Rectangle {
        id: bubbleHelp
        property alias text: bubbleHelpLabel.text
        color: application.toolTipBaseColor
        width: bubbleHelpLabel.width + 8
        height: bubbleHelpLabel.height + 8
        radius: 4
        states: [
            State { name: 'invisible'; PropertyChanges { target: bubbleHelp; opacity: 0} },
            State { name: 'visible'; PropertyChanges { target: bubbleHelp; opacity: 1} }
        ]
        state: 'invisible'
        transitions: [
            Transition {
                from: 'invisible'
                to: 'visible'
                OpacityAnimator { target: bubbleHelp; duration: 200; easing.type: Easing.InOutQuad }
            },
            Transition {
                from: 'visible'
                to: 'invisible'
                OpacityAnimator { target: bubbleHelp; duration: 200; easing.type: Easing.InOutQuad }
            }
        ]
        Label {
            id: bubbleHelpLabel
            color: application.toolTipTextColor
            anchors.centerIn: parent
        }
        function show(x, y, text) {
            bubbleHelp.x = x + tracksArea.x - tracksFlickable.contentX - bubbleHelpLabel.width
            bubbleHelp.y = Math.max(0, y + tracksArea.y - tracksFlickable.contentY - bubbleHelpLabel.height)
            bubbleHelp.text = text
            if (bubbleHelp.state !== 'visible')
                bubbleHelp.state = 'visible'
        }
        function hide() {
            bubbleHelp.state = 'invisible'
            bubbleHelp.opacity = 0
        }
    }
    DropShadow {
        source: bubbleHelp
        anchors.fill: bubbleHelp
        opacity: bubbleHelp.opacity
        horizontalOffset: 3
        verticalOffset: 3
        radius: 8
        color: '#80000000'
        transparentBorder: true
        fast: true
    }

    DelegateModel {
        id: parameterDelegateModel
        model: parameters
        Parameter {
            rootIndex: parameterDelegateModel.modelIndex(index)
            width: producer.duration * timeScale
            isCurve: model.isCurve
            minimum: model.minimum
            maximum: model.maximum
            height: Logic.trackHeight(model.isCurve)
            onClicked: {
                currentTrack = parameter.DelegateModel.itemsIndex
                root.selection = [keyframe.DelegateModel.itemsIndex]
            }
            onRightClicked: root.keyframeRightClicked()
        }
    }

    Connections {
        target: producer
        function onPositionChanged() {
            if (!stopScrolling) Logic.scrollIfNeeded()
        }
    }

    Connections {
        target: filter
        function onChanged(name) {
            var parameterIndex = parameters.parameterIndex(name)
            if (parameterIndex > -1) {
                currentTrack = parameterIndex
                var keyframeIndex = parameters.keyframeIndex(parameterIndex, producer.position + producer.in)
                if (keyframeIndex > -1)
                    selection = [keyframeIndex]
            }
        }
    }

    Connections {
        target: keyframes
        function onSetZoom(value) { setZoom(value) }
        function onZoomIn() { zoomIn() }
        function onZoomOut() {zoomOut() }
        function onZoomToFit() { zoomToFit() }
        function onResetZoom() { resetZoom() }
        function onSeekPreviousSimple() { Logic.seekPreviousSimple() }
        function onSeekNextSimple() { Logic.seekNextSimple() }
    }

    // This provides continuous scrolling at the left/right edges.
    Timer {
        id: scrollTimer
        interval: 25
        repeat: true
        triggeredOnStart: true
        property var item
        property bool backwards
        onTriggered: {
            var delta = backwards? -10 : 10
            if (item) item.x += delta
            tracksFlickable.contentX += delta
            if (tracksFlickable.contentX <= 0)
                stop()
        }
    }
}
