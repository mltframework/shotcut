/*
 * Copyright (c) 2017-2018 Meltytech, LLC
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

import QtQuick 2.5
import QtQml.Models 2.1
import QtQuick.Controls 1.3
import Shotcut.Controls 1.0
import QtGraphicalEffects 1.0
import QtQuick.Window 2.2
import 'Keyframes.js' as Logic

Rectangle {
    id: root
    width: 400
    SystemPalette { id: activePalette }
    color: activePalette.window

    property int selectedIndex: -1
    property int headerWidth: 140
    property int currentTrack: 0
    property color selectedTrackColor: Qt.rgba(0.8, 0.8, 0, 0.3);
    property bool stopScrolling: false
    property color shotcutBlue: Qt.rgba(23/255, 92/255, 118/255, 1.0)
    property double timeScale: 1.0
    property var selection: []

    signal keyframeClicked()

    onTimeScaleChanged: redrawWaveforms()

    function redrawWaveforms() {
        Logic.scrollIfNeeded()
        beforeClip.generateWaveform()
        activeClip.generateWaveform()
        afterClip.generateWaveform()
    }

    function setZoom(value) {
        keyframesToolbar.scaleSlider.value = value
    }

    function adjustZoom(by) {
        setZoom(keyframesToolbar.scaleSlider.value + by)
    }

    function zoomIn() {
        adjustZoom(0.0625)
    }

    function zoomOut() {
        adjustZoom(-0.0625)
    }

    function resetZoom() {
        setZoom(1.0)
    }

    function zoomByWheel(wheel) {
        if (wheel.modifiers & Qt.ControlModifier) {
            adjustZoom(wheel.angleDelta.y / 720)
        }
        if (wheel.modifiers & Qt.ShiftModifier) {
            multitrack.trackHeight = Math.max(30, multitrack.trackHeight + wheel.angleDelta.y / 5)
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: menu.popup()
    }

    KeyframesToolbar {
        id: keyframesToolbar
        width: parent.width
        height: ruler.height + 6
        anchors.top: parent.top
        z: 1
    }

    Row {
        anchors.top: keyframesToolbar.bottom
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
                contentY: scrollView.flickableItem.contentY
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
                        selected: false
//                        onIsLockedChanged: parametersRepeater.itemAt(index).isLocked = isLocked
                    }
                    Repeater {
                        id: trackHeaderRepeater
                        model: parameters
                        ParameterHead {
                            trackName: model.name
                            delegateIndex: index
//                            isLocked: model.locked
                            width: headerWidth
                            height: Logic.trackHeight(model.isCurve)
                            current: false // index === currentTrack
//                            onIsLockedChanged: parametersRepeater.itemAt(index).isLocked = isLocked
                            onClicked: {
                                currentTrack = index
//                                timeline.selectTrackHead(currentTrack)
                            }
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
            onClicked: producer.position = (scrollView.flickableItem.contentX + mouse.x) / timeScale
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
                if (mouse.modifiers === Qt.ShiftModifier || mouse.buttons === Qt.LeftButton) {
                    producer.position = (scrollView.flickableItem.contentX + mouse.x) / timeScale
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

            Column {
                Flickable {
                    // Non-slider scroll area for the Ruler.
                    contentX: scrollView.flickableItem.contentX
                    width: root.width - headerWidth
                    height: ruler.height
                    interactive: false

                    Ruler {
                        id: ruler
                        width: producer.duration * timeScale
                        index: index
                    }
                }
                ScrollView {
                    id: scrollView
                    width: root.width - headerWidth
                    height: root.height - ruler.height - keyframesToolbar.height

                    Item {
                        width: tracksContainer.width + headerWidth
                        height: trackHeaders.height + 30 // 30 is padding
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
                                    color: (false /*index === currentTrack*/)? selectedTrackColor : (index % 2)? activePalette.alternateBase : activePalette.base
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
                                                filter.in = n
                                                // Show amount trimmed as a time in a "bubble" help.
                                                var s = application.timecode(Math.abs(clip.originalX))
                                                s = '%1%2 = %3'.arg((clip.originalX < 0)? '-' : (clip.originalX > 0)? '+' : '')
                                                               .arg(s.substring(3))
                                                               .arg(application.timecode(n))
                                                bubbleHelp.show(clip.x, trackRoot.y + trackRoot.height, s)
                                            }
                                        }
                                        onTrimmedIn: bubbleHelp.hide()
                                        onTrimmingOut: {
                                            var n = filter.out - delta
                                            if (delta != 0 && n >= filter.in && n <= producer.out) {
                                                filter.out = n
                                                // Show amount trimmed as a time in a "bubble" help.
                                                var s = application.timecode(Math.abs(clip.originalX))
                                                s = '%1%2 = %3'.arg((clip.originalX < 0)? '+' : (clip.originalX > 0)? '-' : '')
                                                               .arg(s.substring(3))
                                                               .arg(application.timecode(n))
                                                bubbleHelp.show(clip.x + clip.width, trackRoot.y + trackRoot.height, s)
                                            }
                                        }
                                        onTrimmedOut: bubbleHelp.hide()
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
                height: root.height - scrollView.__horizontalScrollBar.height - keyframesToolbar.height
                x: producer.position * timeScale - scrollView.flickableItem.contentX
                y: 0
            }
            TimelinePlayhead {
                id: playhead
                visible: producer.position > -1 && metadata !== null
                x: producer.position * timeScale - scrollView.flickableItem.contentX - 5
                y: 0
                width: 11
                height: 5
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
            bubbleHelp.x = x + tracksArea.x - scrollView.flickableItem.contentX - bubbleHelpLabel.width
            bubbleHelp.y = y + tracksArea.y - scrollView.flickableItem.contentY - bubbleHelpLabel.height
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

    Menu {
        id: menu
        MenuItem {
            text: qsTr('Show Audio Waveforms')
            checkable: true
            checked: settings.timelineShowWaveforms
            onTriggered: {
                if (checked) {
                    if (settings.timelineShowWaveforms) {
                        settings.timelineShowWaveforms = checked
                        redrawWaveforms()
                    } else {
                        settings.timelineShowWaveforms = checked
                        producer.remakeAudioLevels()
                    }
                } else {
                    settings.timelineShowWaveforms = checked
                }
            }
        }
        MenuItem {
            text: qsTr('Show Video Thumbnails')
            checkable: true
            checked: settings.timelineShowThumbnails
            onTriggered: settings.timelineShowThumbnails = checked
        }
        MenuItem {
            text: qsTr('Reload')
            onTriggered: parameters.reload()
        }
        onPopupVisibleChanged: {
            if (visible && application.OS === 'Windows' && __popupGeometry.height > 0) {
                // Try to fix menu running off screen. This only works intermittently.
                menu.__yOffset = Math.min(0, Screen.height - (__popupGeometry.y + __popupGeometry.height + 40))
                menu.__xOffset = Math.min(0, Screen.width - (__popupGeometry.x + __popupGeometry.width))
            }
        }
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
                root.keyframeClicked()
            }
        }
    }

    Connections {
        target: producer
        onPositionChanged: if (!stopScrolling) {
           var x = position * timeScale;
           if (!scrollView) return;
           if (x > scrollView.flickableItem.contentX + scrollView.width - 50)
               scrollView.flickableItem.contentX = x - scrollView.width + 50;
           else if (x < 50)
               scrollView.flickableItem.contentX = 0;
           else if (x < scrollView.flickableItem.contentX + 50)
               scrollView.flickableItem.contentX = x - 50;
        }
    }

    Connections {
        target: filter
        onChanged: {
            var parameterIndex = parameters.parameterIndex(name)
            if (parameterIndex > -1) {
                currentTrack = parameterIndex
                var keyframeIndex = parameters.keyframeIndex(parameterIndex, producer.position + producer.in)
                if (keyframeIndex > -1)
                    selection = [keyframeIndex]
            }
        }
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
            scrollView.flickableItem.contentX += delta
            if (scrollView.flickableItem.contentX <= 0)
                stop()
        }
    }
}
