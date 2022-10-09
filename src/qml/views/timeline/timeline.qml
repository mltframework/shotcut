/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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

import QtGraphicalEffects 1.12
import QtQml.Models 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import Shotcut.Controls 1.0 as Shotcut
import "Timeline.js" as Logic

Rectangle {
    id: root

    property int headerWidth: 140
    property color selectedTrackColor: Qt.rgba(0.8, 0.8, 0, 0.3)
    property alias trackCount: tracksRepeater.count
    property bool stopScrolling: false
    property color shotcutBlue: Qt.rgba(23 / 255, 92 / 255, 118 / 255, 1)
    property var dragDelta

    signal clipClicked()
    signal timelineRightClicked()
    signal clipRightClicked()

    function setZoom(value, targetX) {
        if (!targetX)
            targetX = tracksFlickable.contentX + tracksFlickable.width / 2;

        var offset = targetX - tracksFlickable.contentX;
        var before = multitrack.scaleFactor;
        if (isNaN(value))
            value = 0;

        multitrack.scaleFactor = Math.pow(value, 3) + 0.01;
        if (!settings.timelineCenterPlayhead && !settings.timelineScrollZoom)
            tracksFlickable.contentX = (targetX * multitrack.scaleFactor / before) - offset;

        for (var i = 0; i < tracksRepeater.count; i++) tracksRepeater.itemAt(i).redrawWaveforms(false)
        if (settings.timelineScrollZoom && !settings.timelineCenterPlayhead)
            scrollZoomTimer.restart();

    }

    function adjustZoom(by, targetX) {
        var value = Math.pow(multitrack.scaleFactor - 0.01, 1 / 3);
        setZoom(value + by, targetX);
    }

    function pulseLockButtonOnTrack(index) {
        trackHeaderRepeater.itemAt(index).pulseLockButton();
    }

    function selectMultitrack() {
        for (var i = 0; i < trackHeaderRepeater.count; i++) trackHeaderRepeater.itemAt(i).selected = false
        cornerstone.selected = true;
    }

    function trackAt(index) {
        return tracksRepeater.itemAt(index);
    }

    function resetDrag() {
        dragDelta = Qt.point(0, 0);
    }

    function insertTrackPrompt() {
        trackTypeDialog.show();
    }

    color: activePalette.window

    SystemPalette {
        id: activePalette
    }

    Timer {
        id: scrollZoomTimer

        interval: 100
        onTriggered: {
            Logic.scrollIfNeeded(true);
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: root.timelineRightClicked()
    }

    DropArea {
        anchors.fill: parent
        onEntered: {
            if (drag.formats.indexOf('application/vnd.mlt+xml') >= 0 || drag.hasUrls)
                drag.acceptProposedAction();

        }
        onExited: Logic.dropped()
        onPositionChanged: {
            if (drag.formats.indexOf('application/vnd.mlt+xml') >= 0 || drag.hasUrls)
                Logic.dragging(drag, drag.hasUrls ? 0 : parseInt(drag.text));

        }
        onDropped: {
            if (drop.formats.indexOf('application/vnd.mlt+xml') >= 0) {
                if (timeline.currentTrack >= 0) {
                    Logic.acceptDrop(drop.getDataAsString('application/vnd.mlt+xml'));
                    drop.acceptProposedAction();
                }
            } else if (drop.hasUrls) {
                Logic.acceptDrop(drop.urls);
                drop.acceptProposedAction();
            }
            Logic.dropped();
        }
    }

    Row {
        anchors.fill: parent

        Column {
            z: 1

            Rectangle {
                id: cornerstone

                property bool selected: false

                width: headerWidth
                height: ruler.height
                color: selected ? shotcutBlue : activePalette.window
                border.color: selected ? 'red' : 'transparent'
                border.width: selected ? 1 : 0
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
                        timeline.selectMultitrack();
                        if (mouse.button == Qt.RightButton)
                            root.timelineRightClicked();

                    }
                }

                ToolButton {
                    visible: multitrack.filtered
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: {
                        timeline.selectMultitrack();
                        timeline.filteredClicked();
                    }

                    Shotcut.HoverTip {
                        text: qsTr('Filters')
                    }

                    action: Action {
                        icon.name: 'view-filter'
                        icon.source: 'qrc:///icons/oxygen/32x32/status/view-filter.png'
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
                            id: trackHead

                            property var trackIndex: index

                            trackName: model.name
                            isMute: model.mute
                            isHidden: model.hidden
                            isComposite: model.composite
                            isLocked: model.locked
                            isVideo: !model.audio
                            isFiltered: model.filtered
                            isTopVideo: model.isTopVideo
                            isBottomVideo: model.isBottomVideo
                            isTopAudio: model.isTopAudio
                            isBottomAudio: model.isBottomAudio
                            width: headerWidth
                            height: Logic.trackHeight(model.audio)
                            current: index === timeline.currentTrack
                            onIsLockedChanged: tracksRepeater.itemAt(index).isLocked = isLocked
                            onClicked: {
                                timeline.currentTrack = index;
                                timeline.selectTrackHead(timeline.currentTrack);
                            }

                            MouseArea {
                                id: dragMouseArea

                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                hoverEnabled: true
                                cursorShape: Qt.DragMoveCursor
                                drag.target: dragItem
                                drag.axis: Drag.YAxis
                                drag.maximumY: trackHeaders.height - y - height / 2
                                drag.minimumY: -y - height / 2
                                width: containsMouse | drag.active ? Math.max(parent.width, dragItemText.contentWidth + 20) : 8
                                onReleased: {
                                    if (drag.active)
                                        dragItem.Drag.drop();

                                }

                                Rectangle {
                                    id: dragItem

                                    property var trackHead: trackHead

                                    anchors.left: dragMouseArea.left
                                    anchors.top: dragMouseArea.top
                                    anchors.bottom: dragMouseArea.bottom
                                    width: dragMouseArea.width
                                    color: 'white'
                                    Drag.active: dragMouseArea.drag.active
                                    Drag.hotSpot.y: height / 2
                                    Drag.keys: ["trackHeader"]
                                    opacity: dragMouseArea.containsMouse | dragMouseArea.drag.active ? 0.7 : 0
                                    states: [
                                        State {
                                            when: dragMouseArea.drag.active | dragMouseArea.containsMouse

                                            ParentChange {
                                                target: dragItem
                                                parent: trackHeaders.parent
                                            }

                                        },
                                        State {
                                            when: !dragMouseArea.drag.active & !dragMouseArea.containsMouse

                                            ParentChange {
                                                target: dragItem
                                                parent: trackHead
                                            }

                                        }
                                    ]

                                    Text {
                                        id: dragItemText

                                        anchors.fill: parent
                                        text: qsTr("Move %1").arg(trackHead.trackName)
                                        style: Text.Outline
                                        styleColor: 'white'
                                        font.pixelSize: Math.min(Math.max(parent.height * 0.7, 15), 30)
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: 10
                                    }

                                    Behavior on opacity {
                                        NumberAnimation {
                                        }

                                    }

                                }

                                Behavior on width {
                                    PropertyAnimation {
                                        easing.type: Easing.InOutCubic
                                    }

                                }

                            }

                            DropArea {
                                id: dropArea

                                property var trackHead: trackHead
                                property bool containsValidDrag: false

                                anchors.fill: parent
                                keys: ["trackHeader"]
                                onEntered: {
                                    if (trackHead.isVideo == drag.source.trackHead.isVideo) {
                                        containsValidDrag = true;
                                        timeline.currentTrack = trackHead.trackIndex;
                                    } else {
                                        containsValidDrag = false;
                                    }
                                }
                                onExited: {
                                    containsValidDrag = false;
                                }
                                onDropped: {
                                    if (drop.proposedAction == Qt.MoveAction) {
                                        if (trackHead.isVideo && !drop.source.trackHead.isVideo) {
                                            application.showStatusMessage(qsTr('Can not move audio track above video track'));
                                        } else if (!trackHead.isVideo && drop.source.trackHead.isVideo) {
                                            application.showStatusMessage(qsTr('Can not move video track below audio track'));
                                        } else if (trackHead.trackIndex == drop.source.trackHead.trackIndex) {
                                            application.showStatusMessage(qsTr('Track %1 was not moved').arg(drop.source.trackHead.trackName));
                                        } else {
                                            drop.acceptProposedAction();
                                            timeline.moveTrack(drop.source.trackHead.trackIndex, trackHead.trackIndex);
                                        }
                                    }
                                    containsValidDrag = false;
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    color: "transparent"
                                    border.color: "green"
                                    border.width: 3
                                    visible: dropArea.containsValidDrag
                                }

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

        Item {
            id: tracksArea

            width: root.width - headerWidth
            height: root.height
            focus: true

            MouseArea {
                property bool skim: false

                // This provides skimming and continuous scrubbing at the left/right edges.
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    timeline.position = (tracksFlickable.contentX + mouse.x) / multitrack.scaleFactor;
                    bubbleHelp.hide();
                }
                onWheel: Logic.onMouseWheel(wheel)
                onReleased: skim = false
                onExited: skim = false
                onPositionChanged: {
                    if (mouse.modifiers === (Qt.ShiftModifier | Qt.AltModifier) || mouse.buttons === Qt.LeftButton) {
                        timeline.position = (tracksFlickable.contentX + mouse.x) / multitrack.scaleFactor;
                        bubbleHelp.hide();
                        skim = true;
                    } else {
                        skim = false;
                    }
                }
            }

            Timer {
                id: scrubTimer

                interval: 25
                repeat: true
                running: parent.skim && parent.containsMouse && (parent.mouseX < 50 || parent.mouseX > parent.width - 50) && (timeline.position * multitrack.scaleFactor >= 50)
                onTriggered: {
                    if (parent.mouseX < 50)
                        timeline.position -= 10;
                    else
                        timeline.position += 10;
                }
            }

            MouseArea {
                property real startX: mouseX
                property real startY: mouseY

                // This provides drag-scrolling the timeline with the middle mouse button.
                anchors.fill: parent
                acceptedButtons: Qt.MiddleButton
                cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.ArrowCursor
                drag.axis: Drag.XAndYAxis
                drag.filterChildren: true
                onPressed: {
                    startX = mouse.x;
                    startY = mouse.y;
                }
                onPositionChanged: {
                    var n = mouse.x - startX;
                    startX = mouse.x;
                    tracksFlickable.contentX = Logic.clamp(tracksFlickable.contentX - n, 0, Logic.scrollMax().x);
                    n = mouse.y - startY;
                    startY = mouse.y;
                    tracksFlickable.contentY = Logic.clamp(tracksFlickable.contentY - n, 0, Logic.scrollMax().y);
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
                    onContentXChanged: {
                        if (contentX === 0)
                            contentX = tracksFlickable.contentX;

                    }

                    Ruler {
                        id: ruler

                        width: tracksContainer.width
                        timeScale: multitrack.scaleFactor
                        onEditMarkerRequested: {
                            timeline.editMarker(index);
                        }
                        onDeleteMarkerRequested: {
                            timeline.deleteMarker(index);
                        }
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

                    Item {
                        id: tracksLayers

                        anchors.fill: parent

                        Column {
                            // These make the striped background for the tracks.
                            // It is important that these are not part of the track visual hierarchy;
                            // otherwise, the clips will be obscured by the Track's background.
                            Repeater {
                                model: multitrack

                                delegate: Rectangle {
                                    width: tracksContainer.width
                                    color: (index === timeline.currentTrack) ? selectedTrackColor : (index % 2) ? activePalette.alternateBase : activePalette.base
                                    height: Logic.trackHeight(audio)
                                }

                            }

                        }

                        Column {
                            id: tracksContainer

                            Repeater {
                                id: tracksRepeater

                                model: trackDelegateModel
                            }

                        }

                        Item {
                            id: selectionContainer

                            visible: false

                            Repeater {
                                id: selectionRepeater

                                model: timeline.selection

                                Rectangle {
                                    property var clip: trackAt(modelData.y).clipAt(modelData.x)
                                    property var track: typeof clip === 'undefined' ? trackAt(clip.trackIndex + dragDelta.y) : 0

                                    x: typeof clip === 'undefined' ? clip.x + dragDelta.x : 0
                                    y: track ? track.y : 0
                                    width: clip.width
                                    height: track ? track.height : 0
                                    color: 'transparent'
                                    border.color: 'red'
                                    visible: !clip.Drag.active && clip.trackIndex === clip.originalTrackIndex
                                }

                            }

                        }

                    }

                    ScrollBar.horizontal: ScrollBar {
                        id: horizontalScrollBar

                        height: 16
                        policy: ScrollBar.AlwaysOn
                        visible: tracksFlickable.contentWidth > tracksFlickable.width
                        parent: tracksFlickable.parent
                        anchors.top: tracksFlickable.bottom
                        anchors.left: tracksFlickable.left
                        anchors.right: tracksFlickable.right

                        background: Rectangle {
                            color: parent.palette.alternateBase
                        }

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

                        background: Rectangle {
                            color: parent.palette.alternateBase
                        }

                    }

                }

            }

            CornerSelectionShadow {
                y: tracksRepeater.count ? tracksRepeater.itemAt(timeline.currentTrack).y + ruler.height - tracksFlickable.contentY : 0
                clip: timeline.selection.length ? tracksRepeater.itemAt(timeline.selection[0].y).clipAt(timeline.selection[0].x) : null
                opacity: clip && clip.x + clip.width < tracksFlickable.contentX ? 1 : 0
            }

            CornerSelectionShadow {
                y: tracksRepeater.count ? tracksRepeater.itemAt(timeline.currentTrack).y + ruler.height - tracksFlickable.contentY : 0
                clip: timeline.selection.length ? tracksRepeater.itemAt(timeline.selection[timeline.selection.length - 1].y).clipAt(timeline.selection[timeline.selection.length - 1].x) : null
                opacity: clip && clip.x > tracksFlickable.contentX + tracksFlickable.width ? 1 : 0
                anchors.right: parent.right
                mirrorGradient: true
            }

            Rectangle {
                id: cursor

                visible: timeline.position > -1
                color: activePalette.text
                width: 1
                height: root.height - horizontalScrollBar.height
                x: timeline.position * multitrack.scaleFactor - tracksFlickable.contentX
                y: 0
            }

            Shotcut.TimelinePlayhead {
                id: playhead

                visible: timeline.position > -1
                x: timeline.position * multitrack.scaleFactor - tracksFlickable.contentX - width / 2
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
            text: settings.timelineRipple ? qsTr('Insert') : qsTr('Overwrite')
            style: Text.Outline
            styleColor: 'white'
            font.pixelSize: Math.min(Math.max(parent.height * 0.8, 15), 30)
            verticalAlignment: Text.AlignVCenter
        }

    }

    Rectangle {
        id: bubbleHelp

        property alias text: bubbleHelpLabel.text

        function show(text) {
            var point = application.mousePos;
            point = parent.mapFromGlobal(point.x, point.y);
            bubbleHelp.x = point.x + 20;
            bubbleHelp.y = Math.max(point.y - 20, 0);
            bubbleHelp.text = text;
            if (bubbleHelp.state !== 'visible')
                bubbleHelp.state = 'visible';

        }

        function hide() {
            bubbleHelp.state = 'invisible';
            bubbleHelp.opacity = 0;
        }

        color: application.toolTipBaseColor
        width: bubbleHelpLabel.width + 8
        height: bubbleHelpLabel.height + 8
        radius: 4
        state: 'invisible'
        states: [
            State {
                name: 'invisible'

                PropertyChanges {
                    target: bubbleHelp
                    opacity: 0
                }

            },
            State {
                name: 'visible'

                PropertyChanges {
                    target: bubbleHelp
                    opacity: 1
                }

            }
        ]
        transitions: [
            Transition {
                from: 'invisible'
                to: 'visible'

                OpacityAnimator {
                    target: bubbleHelp
                    duration: 200
                    easing.type: Easing.InOutQuad
                }

            },
            Transition {
                from: 'visible'
                to: 'invisible'

                OpacityAnimator {
                    target: bubbleHelp
                    duration: 200
                    easing.type: Easing.InOutQuad
                }

            }
        ]

        Label {
            id: bubbleHelpLabel

            textFormat: Text.RichText
            color: application.toolTipTextColor
            anchors.centerIn: parent
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
        id: trackDelegateModel

        model: multitrack

        Track {
            // Only allow one blank to be selected
            // select one
            //  Clear previous blank selection

            model: multitrack
            rootIndex: trackDelegateModel.modelIndex(index)
            height: Logic.trackHeight(audio)
            isAudio: audio
            isMute: mute
            isCurrentTrack: timeline.currentTrack === index
            timeScale: multitrack.scaleFactor
            onClipClicked: {
                var trackIndex = track.DelegateModel.itemsIndex;
                var clipIndex = clip.DelegateModel.itemsIndex;
                timeline.currentTrack = trackIndex;
                if (timeline.selection.length == 1 && tracksRepeater.itemAt(timeline.selection[0].y).clipAt(timeline.selection[0].x).isBlank)
                    timeline.selection = [];

                if (tracksRepeater.itemAt(trackIndex).clipAt(clipIndex).isBlank)
                    timeline.selection = [Qt.point(clipIndex, trackIndex)];
                else if (mouse && mouse.modifiers & Qt.ControlModifier)
                    timeline.selection = Logic.toggleSelection(trackIndex, clipIndex);
                else if (mouse && mouse.modifiers & Qt.ShiftModifier)
                    timeline.selection = Logic.selectRange(trackIndex, clipIndex);
                else if (!Logic.selectionContains(trackIndex, clipIndex))
                    timeline.selection = [Qt.point(clipIndex, trackIndex)];
                root.clipClicked();
            }
            onClipRightClicked: root.clipRightClicked()
            onClipDragged: {
                // This provides continuous scrolling at the left/right edges.
                if (x > tracksFlickable.contentX + tracksFlickable.width - 50) {
                    scrollTimer.item = clip;
                    scrollTimer.backwards = false;
                    scrollTimer.start();
                } else if (x < 50) {
                    tracksFlickable.contentX = 0;
                    scrollTimer.stop();
                } else if (x < tracksFlickable.contentX + 50) {
                    scrollTimer.item = clip;
                    scrollTimer.backwards = true;
                    scrollTimer.start();
                } else {
                    scrollTimer.stop();
                }
                dragDelta = Qt.point(clip.x - clip.originalX, clip.trackIndex - clip.originalTrackIndex);
                selectionContainer.visible = true;
            }
            onClipDropped: {
                scrollTimer.running = false;
                bubbleHelp.hide();
                selectionContainer.visible = false;
            }
            onClipDraggedToTrack: {
                var i = clip.trackIndex + direction;
                var track = trackAt(i);
                clip.reparent(track);
                clip.trackIndex = track.DelegateModel.itemsIndex;
            }
            onCheckSnap: {
                for (var i = 0; i < tracksRepeater.count; i++) tracksRepeater.itemAt(i).snapClip(clip)
            }

            Image {
                anchors.fill: parent
                source: "qrc:///icons/light/16x16/track-locked.png"
                fillMode: Image.Tile
                opacity: parent.isLocked
                visible: opacity

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        mouse.accepted = true;
                        trackHeaderRepeater.itemAt(index).pulseLockButton();
                    }
                }

                Behavior on opacity {
                    NumberAnimation {
                    }

                }

            }

        }

    }

    Connections {
        function onPositionChanged() {
            if (!stopScrolling)
                Logic.scrollIfNeeded();

        }

        function onDragging() {
            Logic.dragging(pos, duration);
        }

        function onDropped() {
            Logic.dropped();
        }

        function onDropAccepted() {
            Logic.acceptDrop(xml);
        }

        function onSelectionChanged() {
            cornerstone.selected = timeline.isMultitrackSelected();
            var selectedTrack = timeline.selectedTrack();
            for (var i = 0; i < trackHeaderRepeater.count; i++) trackHeaderRepeater.itemAt(i).selected = (i === selectedTrack)
        }

        function onZoomIn() {
            adjustZoom(0.0625);
        }

        function onZoomOut() {
            adjustZoom(-0.0625);
        }

        function onZoomToFit() {
            setZoom(Math.pow((tracksFlickable.width - 50) * multitrack.scaleFactor / tracksContainer.width - 0.01, 1 / 3));
            scrollZoomTimer.stop();
            tracksFlickable.contentX = 0;
        }

        function onSetZoom(value) {
            setZoom(value);
        }

        function onWarnTrackLocked() {
            pulseLockButtonOnTrack(trackIndex);
        }

        function onMultitrackSelected() {
            selectMultitrack();
        }

        function onRefreshWaveforms() {
            if (!settings.timelineShowWaveforms) {
                for (var i = 0; i < tracksRepeater.count; i++) tracksRepeater.itemAt(i).redrawWaveforms()
            } else {
                for (var i = 0; i < tracksRepeater.count; i++) tracksRepeater.itemAt(i).remakeWaveforms(false)
            }
        }

        function onUpdateThumbnails(trackIndex, clipIndex) {
            if (trackIndex >= 0 && trackIndex < tracksRepeater.count)
                tracksRepeater.itemAt(trackIndex).updateThumbnails(clipIndex);

        }

        target: timeline
    }

    Connections {
        function onScaleFactorChanged() {
            if (settings.timelineCenterPlayhead)
                Logic.scrollIfNeeded();

        }

        target: multitrack
    }

    // This provides continuous scrolling at the left/right edges.
    Timer {
        id: scrollTimer

        property var item
        property bool backwards

        interval: 25
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            var delta = backwards ? -10 : 10;
            if (item)
                item.x += delta;

            tracksFlickable.contentX += delta;
            if (tracksFlickable.contentX <= 0)
                stop();

        }
    }

    Window {
        id: trackTypeDialog

        width: 400
        height: 80
        flags: Qt.Dialog
        color: activePalette.window
        modality: application.dialogModality

        GridLayout {
            columns: 4
            anchors.fill: parent
            anchors.margins: 8

            Label {
                text: qsTr('Do you want to insert an audio or video track?')
                Layout.columnSpan: 4
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                Layout.fillWidth: true
            }

            RadioButton {
                text: qsTr("Audio")
                onClicked: {
                    timeline.insertAudioTrack();
                    trackTypeDialog.close();
                }
            }

            RadioButton {
                text: qsTr("Video")
                onClicked: {
                    timeline.insertVideoTrack();
                    trackTypeDialog.close();
                }
            }

            Label {
                Layout.fillWidth: true
            }

        }

    }

}
