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
import QtQml.Models 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12
import Shotcut.Controls 1.0 as Shotcut
import 'Timeline.js' as Logic

Rectangle {
    id: root
    SystemPalette { id: activePalette }
    color: activePalette.window

    signal clipClicked()

    function setZoom(value, targetX) {
        if (!targetX)
            targetX = tracksFlickable.contentX + tracksFlickable.width / 2
        var offset = targetX - tracksFlickable.contentX
        var before = multitrack.scaleFactor

        toolbar.scaleSlider.value = value

        if (!settings.timelineCenterPlayhead && !settings.timelineScrollZoom)
            tracksFlickable.contentX = (targetX * multitrack.scaleFactor / before) - offset

        for (var i = 0; i < tracksRepeater.count; i++)
            tracksRepeater.itemAt(i).redrawWaveforms(false)
    }

    Timer {
        id: scrollZoomTimer
        interval: 100
        onTriggered: {
            Logic.scrollIfNeeded(true)
        }
    }

    function adjustZoom(by, targetX) {
        setZoom(toolbar.scaleSlider.value + by, targetX)
        if (settings.timelineScrollZoom && !settings.timelineCenterPlayhead)
            scrollZoomTimer.restart()
    }

    function zoomIn() {
        adjustZoom(0.0625)
    }

    function zoomOut() {
        adjustZoom(-0.0625)
    }

    function zoomToFit() {
        setZoom(Math.pow((tracksFlickable.width - 50) * multitrack.scaleFactor / tracksContainer.width - 0.01, 1/3))
        tracksFlickable.contentX = 0
    }

    function resetZoom() {
        setZoom(1.0)
    }

    function makeTracksTaller() {
        multitrack.trackHeight += 20
    }

    function makeTracksShorter() {
        multitrack.trackHeight = Math.max(10, multitrack.trackHeight - 20)
    }

    function pulseLockButtonOnTrack(index) {
        trackHeaderRepeater.itemAt(index).pulseLockButton()
    }

    function selectMultitrack() {
        for (var i = 0; i < trackHeaderRepeater.count; i++)
            trackHeaderRepeater.itemAt(i).selected = false
        cornerstone.selected = true
    }

    function trackAt(index) {
        return tracksRepeater.itemAt(index)
    }

    function resetDrag() {
        dragDelta = Qt.point(0, 0)
    }

    property int headerWidth: 140
    property int currentTrack: 0
    property color selectedTrackColor: Qt.rgba(0.8, 0.8, 0, 0.3);
    property alias trackCount: tracksRepeater.count
    property bool stopScrolling: false
    property color shotcutBlue: Qt.rgba(23/255, 92/255, 118/255, 1.0)
    property var dragDelta

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: menu.popup()
    }

    DropArea {
        anchors.fill: parent
        onEntered: {
            if (drag.formats.indexOf('application/vnd.mlt+xml') >= 0 || drag.hasUrls)
                drag.acceptProposedAction()
        }
        onExited: Logic.dropped()
        onPositionChanged: {
            if (drag.formats.indexOf('application/vnd.mlt+xml') >= 0 || drag.hasUrls)
                Logic.dragging(drag, drag.hasUrls? 0 : parseInt(drag.text))
        }
        onDropped: {
            if (drop.formats.indexOf('application/vnd.mlt+xml') >= 0) {
                if (currentTrack >= 0) {
                    Logic.acceptDrop(drop.getDataAsString('application/vnd.mlt+xml'))
                    drop.acceptProposedAction()
                }
            } else if (drop.hasUrls) {
                Logic.acceptDrop(drop.urls)
                drop.acceptProposedAction()
            }
            Logic.dropped()
        }
    }

    TimelineToolbar {
        id: toolbar
        width: parent.width
        anchors.top: parent.top
        z: 1
    }

    Row {
        anchors.fill: parent
        anchors.topMargin: toolbar.height
        Column {
            z: 1

            Rectangle {
                id: cornerstone
                property bool selected: false
                // Padding between toolbar and track headers.
                width: headerWidth
                height: ruler.height
                color: selected? shotcutBlue : activePalette.window
                border.color: selected? 'red' : 'transparent'
                border.width: selected? 1 : 0
                visible: trackHeaderRepeater.count
                z: 1
                Label {
                    text: qsTr('Output')
                    color: activePalette.windowText
                    elide: Qt.ElideRight
                    x: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 8
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: {
                        timeline.selectMultitrack()
                        if (mouse.button == Qt.RightButton) {
                            menu.popup()
                        }
                    }
                }
                ToolButton {
                    visible: multitrack.filtered
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    action: Action {
                        icon.name: 'view-filter'
                        icon.source: 'qrc:///icons/oxygen/32x32/status/view-filter.png'
                    }
                    Shotcut.HoverTip { text: qsTr('Filters') }
                    onClicked: {
                        timeline.selectMultitrack()
                        timeline.filteredClicked()
                    }
                }
            }
            Flickable {
                // Non-slider scroll area for the track headers.
                contentY: tracksFlickable.contentY
                width: headerWidth
                height: trackHeaders.height
                interactive: false

                Column {
                    id: trackHeaders
                    Repeater {
                        id: trackHeaderRepeater
                        model: multitrack
                        TrackHead {
                            trackName: model.name
                            isMute: model.mute
                            isHidden: model.hidden
                            isComposite: model.composite
                            isLocked: model.locked
                            isVideo: !model.audio
                            isFiltered: model.filtered
                            isBottomVideo: model.isBottomVideo
                            width: headerWidth
                            height: Logic.trackHeight(model.audio)
                            current: index === currentTrack
                            onIsLockedChanged: tracksRepeater.itemAt(index).isLocked = isLocked
                            onClicked: {
                                currentTrack = index
                                timeline.selectTrackHead(currentTrack)
                            }
                        }
                    }
                }
                Rectangle {
                    // thin dividing line between headers and tracks
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
                timeline.position = (tracksFlickable.contentX + mouse.x) / multitrack.scaleFactor
                bubbleHelp.hide()
            }
            property bool scim: false
            onReleased: scim = false
            onExited: scim = false
            onPositionChanged: {
                if (mouse.modifiers === (Qt.ShiftModifier | Qt.AltModifier) || mouse.buttons === Qt.LeftButton) {
                    timeline.position = (tracksFlickable.contentX + mouse.x) / multitrack.scaleFactor
                    bubbleHelp.hide()
                    scim = true
                } else {
                    scim = false
                }
            }
            onWheel: Logic.onMouseWheel(wheel)

            Timer {
                id: scrubTimer
                interval: 25
                repeat: true
                running: parent.scim && parent.containsMouse
                         && (parent.mouseX < 50 || parent.mouseX > parent.width - 50)
                         && (timeline.position * multitrack.scaleFactor >= 50)
                onTriggered: {
                    if (parent.mouseX < 50)
                        timeline.position -= 10
                    else
                        timeline.position += 10
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
                        width: tracksContainer.width
                        timeScale: multitrack.scaleFactor
                    }
                }
                Flickable {
                    id: tracksFlickable
                    y: ruler.height
                    width: root.width - headerWidth - 16
                    height: root.height - toolbar.height - ruler.height - 16
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
                            // These make the striped background for the tracks.
                            // It is important that these are not part of the track visual hierarchy;
                            // otherwise, the clips will be obscured by the Track's background.
                            Repeater {
                                model: multitrack
                                delegate: Rectangle {
                                    width: tracksContainer.width
                                    color: (index === currentTrack)? selectedTrackColor : (index % 2)? activePalette.alternateBase : activePalette.base
                                    height: Logic.trackHeight(audio)
                                }
                            }
                        }
                        Column {
                            id: tracksContainer
                            Repeater { id: tracksRepeater; model: trackDelegateModel }
                        }
                        Item {
                            id: selectionContainer
                            visible: false
                            Repeater {
                                id: selectionRepeater
                                model: timeline.selection
                                Rectangle {
                                    property var clip: trackAt(modelData.y).clipAt(modelData.x)
                                    property var track: trackAt(clip.trackIndex + dragDelta.y)
                                    x: clip.x + dragDelta.x
                                    y: track.y
                                    width: clip.width
                                    height: track.height
                                    color: 'transparent'
                                    border.color: 'red'
                                    visible: !clip.Drag.active && clip.trackIndex === clip.originalTrackIndex
                                }
                            }
                        }

                    }
                }
            }

            CornerSelectionShadow {
                y: tracksRepeater.count ? tracksRepeater.itemAt(currentTrack).y + ruler.height - tracksFlickable.contentY : 0
                clip: timeline.selection.length ?
                        tracksRepeater.itemAt(timeline.selection[0].y).clipAt(timeline.selection[0].x) : null
                opacity: clip && clip.x + clip.width < tracksFlickable.contentX ? 1 : 0
            }

            CornerSelectionShadow {
                y: tracksRepeater.count ? tracksRepeater.itemAt(currentTrack).y + ruler.height - tracksFlickable.contentY : 0
                clip: timeline.selection.length ?
                        tracksRepeater.itemAt(timeline.selection[timeline.selection.length - 1].y).clipAt(timeline.selection[timeline.selection.length - 1].x) : null
                opacity: clip && clip.x > tracksFlickable.contentX + tracksFlickable.width ? 1 : 0
                anchors.right: parent.right
                mirrorGradient: true
            }

            Rectangle {
                id: cursor
                visible: timeline.position > -1
                color: activePalette.text
                width: 1
                height: root.height - toolbar.height - horizontalScrollBar.height
                x: timeline.position * multitrack.scaleFactor - tracksFlickable.contentX
                y: 0
            }
            Shotcut.TimelinePlayhead {
                id: playhead
                visible: timeline.position > -1
                x: timeline.position * multitrack.scaleFactor - tracksFlickable.contentX - width/2
                y: 0
                width: 16
                height: 8
            }
        }
    }

    Rectangle {
        id: dropTarget
        height: multitrack.trackHeight
        opacity: 0.5
        visible: false
        Text {
            anchors.fill: parent
            anchors.leftMargin: 100
            text: settings.timelineRipple? qsTr('Insert') : qsTr('Overwrite')
            style: Text.Outline
            styleColor: 'white'
            font.pixelSize: Math.min(Math.max(parent.height * 0.8, 15), 30)
            verticalAlignment: Text.AlignVCenter
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
            bubbleHelp.y = Math.max(toolbar.height, y + tracksArea.y - tracksFlickable.contentY - bubbleHelpLabel.height)
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
        Menu {
            title: qsTr('Track Operations')
            MenuItem {
                text: qsTr('Add Audio Track') + (application.OS === 'OS X'? '    ⌘U' : ' (Ctrl+U)')
                onTriggered: timeline.addAudioTrack();
            }
            MenuItem {
                text: qsTr('Add Video Track') + (application.OS === 'OS X'? '    ⌘I' : ' (Ctrl+I)')
                onTriggered: timeline.addVideoTrack();
            }
            MenuItem {
                text: qsTr('Insert Track') + (application.OS === 'OS X'? '    ⌥⌘I' : ' (Ctrl+Alt+I)')
                onTriggered: timeline.insertTrack()
            }
            MenuItem {
                text: qsTr('Remove Track') + (application.OS === 'OS X'? '    ⌥⌘U' : ' (Ctrl+Alt+U)')
                onTriggered: timeline.removeTrack()
            }
        }
        Menu {
            title: qsTr('Track Height')
            width: 210
            MenuItem {
                enabled: multitrack.trackHeight > 10
                text: qsTr('Make Tracks Shorter') + (application.OS === 'OS X'? '    ⌘-' : ' (Ctrl+-)')
                onTriggered: makeTracksShorter()
            }
            MenuItem {
                text: qsTr('Make Tracks Taller') + (application.OS === 'OS X'? '    ⌘+' : ' (Ctrl++)')
                onTriggered: makeTracksTaller()
            }
            MenuItem {
                text: qsTr('Reset Track Height') + (application.OS === 'OS X'? '    ⌘=' : ' (Ctrl+=)')
                onTriggered: multitrack.trackHeight = 50
            }
        }
        Menu {
            title: qsTr('Selection')
            MenuItem {
                text: qsTr('Select All') + (application.OS === 'OS X'? '    ⌘A' : ' (Ctrl+A)')
                onTriggered: timeline.selectAll()
            }
            MenuItem {
                text: qsTr('Select None') + (application.OS === 'OS X'? '    ⌘D' : ' (Ctrl+D)')
                onTriggered: {
                    timeline.selection = []
                    multitrack.reload()
                }
            }
        }
        Menu {
            title: qsTr('Options')
            width: 310
            MenuItem {
                text: qsTr("Ripple All Tracks") + (application.OS === 'OS X'? '    ⌥⌘R' : ' (Ctrl+Alt+R)')
                checkable: true
                checked: settings.timelineRippleAllTracks
                onTriggered: settings.timelineRippleAllTracks = checked
            }
            MenuItem {
                text: qsTr('Show Audio Waveforms')
                checkable: true
                checked: settings.timelineShowWaveforms
                onTriggered: {
                    if (checked) {
                        if (settings.timelineShowWaveforms) {
                            settings.timelineShowWaveforms = checked
                            for (var i = 0; i < tracksRepeater.count; i++)
                                tracksRepeater.itemAt(i).redrawWaveforms()
                        } else {
                            settings.timelineShowWaveforms = checked
                            for (i = 0; i < tracksRepeater.count; i++)
                                tracksRepeater.itemAt(i).remakeWaveforms(false)
                        }
                    } else {
                        settings.timelineShowWaveforms = checked
                    }
                }
            }
            MenuItem {
                text: qsTr('Use Higher Performance Waveforms')
                checkable: true
                checked: settings.timelineFramebufferWaveform
                onTriggered: {
                    settings.timelineFramebufferWaveform = checked
                    if (settings.timelineShowWaveforms)
                        multitrack.reload()
                }
            }
            MenuItem {
                text: qsTr('Show Video Thumbnails')
                checkable: true
                checked: settings.timelineShowThumbnails
                onTriggered: settings.timelineShowThumbnails = checked
            }
            MenuItem {
                text: qsTr('Center the Playhead') + (application.OS === 'OS X'? '    ⇧⌘P' : ' (Ctrl+Shift+P)')
                checkable: true
                checked: settings.timelineCenterPlayhead
                onTriggered: settings.timelineCenterPlayhead = checked
            }
            MenuItem {
                text: qsTr('Scroll to Playhead on Zoom') + (application.OS === 'OS X'? '    ⌥⌘P' : ' (Ctrl+Alt+P)')
                checkable: true
                checked: settings.timelineScrollZoom
                onTriggered: settings.timelineScrollZoom = checked
            }
        }
        Menu {
            title: qsTr('Other')
            width: 270
            MenuItem {
                text: qsTr('Copy Timeline to Source') + (application.OS === 'OS X'? '    ⌥⌘C' : ' (Ctrl+Alt+C)')
                onTriggered: timeline.copyToSource()
            }
            MenuItem {
                text: qsTr('Reload') + (application.OS === 'OS X'? '    F5' : ' (F5)')
                onTriggered: multitrack.reload()
            }
            MenuItem {
                id: propertiesMenuItem
                enabled: false
                text: qsTr('Properties')
                onTriggered: timeline.openProperties()
            }
        }
        MenuItem {
            text: qsTr('Cancel')
            onTriggered: menu.dismiss()
        }
    }

    DelegateModel {
        id: trackDelegateModel
        model: multitrack
        Track {
            model: multitrack
            rootIndex: trackDelegateModel.modelIndex(index)
            height: Logic.trackHeight(audio)
            isAudio: audio
            isMute: mute
            isCurrentTrack: currentTrack === index
            timeScale: multitrack.scaleFactor
            onClipClicked: {
                var trackIndex = track.DelegateModel.itemsIndex
                var clipIndex = clip.DelegateModel.itemsIndex
                currentTrack = trackIndex
                if (mouse && mouse.modifiers & Qt.ControlModifier)
                    timeline.selection = Logic.toggleSelection(trackIndex, clipIndex)
                else if (mouse && mouse.modifiers & Qt.ShiftModifier)
                    timeline.selection = Logic.selectRange(trackIndex, clipIndex)
                else if (!Logic.selectionContains(trackIndex, clipIndex))
                    // select one
                    timeline.selection = [Qt.point(clipIndex, trackIndex)]
                root.clipClicked()
            }
            onClipDragged: {
                // This provides continuous scrolling at the left/right edges.
                if (x > tracksFlickable.contentX + tracksFlickable.width - 50) {
                    scrollTimer.item = clip
                    scrollTimer.backwards = false
                    scrollTimer.start()
                } else if (x < 50) {
                    tracksFlickable.contentX = 0;
                    scrollTimer.stop()
                } else if (x < tracksFlickable.contentX + 50) {
                    scrollTimer.item = clip
                    scrollTimer.backwards = true
                    scrollTimer.start()
                } else {
                    scrollTimer.stop()
                }
                dragDelta = Qt.point(clip.x - clip.originalX, clip.trackIndex - clip.originalTrackIndex)
                selectionContainer.visible = true
            }
            onClipDropped: {
                scrollTimer.running = false
                bubbleHelp.hide()
                selectionContainer.visible = false
            }
            onClipDraggedToTrack: {
                var i = clip.trackIndex + direction
                var track = trackAt(i)
                clip.reparent(track)
                clip.trackIndex = track.DelegateModel.itemsIndex
            }
            onCheckSnap: {
                for (var i = 0; i < tracksRepeater.count; i++)
                    tracksRepeater.itemAt(i).snapClip(clip)
            }
            Image {
                anchors.fill: parent
                source: "qrc:///icons/light/16x16/track-locked.png"
                fillMode: Image.Tile
                opacity: parent.isLocked
                visible: opacity
                Behavior on opacity { NumberAnimation {} }
                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        mouse.accepted = true;
                        trackHeaderRepeater.itemAt(index).pulseLockButton()
                    }
                }
            }
        }
    }
    
    Connections {
        target: timeline
        onPositionChanged: if (!stopScrolling) Logic.scrollIfNeeded()
        onDragging: Logic.dragging(pos, duration)
        onDropped: Logic.dropped()
        onDropAccepted: Logic.acceptDrop(xml)
        onSelectionChanged: {
            cornerstone.selected = timeline.isMultitrackSelected()
            var selectedTrack = timeline.selectedTrack()
            for (var i = 0; i < trackHeaderRepeater.count; i++)
                trackHeaderRepeater.itemAt(i).selected = (i === selectedTrack)
            propertiesMenuItem.enabled = (cornerstone.selected || (selectedTrack >= 0 && selectedTrack < trackHeaderRepeater.count))
        }
        onZoomIn: zoomIn()
        onZoomOut: zoomOut()
        onZoomToFit: zoomToFit()
        onResetZoom: resetZoom()
        onMakeTracksShorter: makeTracksShorter()
        onMakeTracksTaller: makeTracksTaller()
    }

    Connections {
        target: multitrack
        onLoaded: toolbar.scaleSlider.value = Math.pow(multitrack.scaleFactor - 0.01, 1.0 / 3.0)
        onScaleFactorChanged: if (settings.timelineCenterPlayhead) Logic.scrollIfNeeded()
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
