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
import QtQml.Models 2.1
import QtQuick.Controls 1.0
import 'Timeline.js' as Logic

Rectangle {
    id: root
    SystemPalette { id: activePalette }
    color: activePalette.window

    property int headerWidth: 140
    property int currentTrack: 0
    property int currentClip: -1
    property int currentClipTrack: -1
    property color selectedTrackColor: Qt.tint(activePalette.base, Qt.rgba(0.8, 0.8, 0, 0.3));
    property alias trackCount: tracksRepeater.count

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: menu.popup()
    }

    DropArea {
        anchors.fill: parent
        onEntered: {
            if (drag.formats.indexOf('application/mlt+xml') >= 0)
                drag.acceptProposedAction()
        }
        onExited: Logic.dropped()
        onPositionChanged: {
            if (drag.formats.indexOf('application/mlt+xml') >= 0)
                Logic.dragging(drag, drag.text)
        }
        onDropped: {
            if (drop.formats.indexOf('application/mlt+xml') >= 0) {
                if (currentTrack >= 0) {
                    Logic.acceptDrop()
                    drop.acceptProposedAction()
                }
            }
            Logic.dropped()
        }
    }

    TimelineToolbar {
        id: toolbar
        width: parent.width
        height: ruler.height + 6
        anchors.top: parent.top
        z: 1
    }

    Row {
        anchors.top: toolbar.bottom
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
                    Repeater {
                        model: multitrack
                        TrackHead {
                            trackName: model.name
                            isMute: model.mute
                            isHidden: model.hidden
                            isComposite: model.composite
                            isVideo: !model.audio
                            color: (index === currentTrack)? selectedTrackColor : (index % 2)? activePalette.alternateBase : activePalette.base
                            width: headerWidth
                            height: model.audio? multitrack.trackHeight : multitrack.trackHeight * 2
                            onClicked: currentTrack = index
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

            ZoomSlider {
                id: scaleSlider
                width: headerWidth
                height: root.height - trackHeaders.height - ruler.height - toolbar.height + 4
                z: 2
                onValueChanged: Logic.scrollIfNeeded()
            }
        }

        MouseArea {
            id: tracksArea
            width: root.width - headerWidth
            height: root.height

            // This provides continuous scrubbing and scimming at the left/right edges.
            focus: true
            hoverEnabled: true
            property bool scim: false
            onReleased: scrubTimer.stop()
            onMouseXChanged: {
                if (scim || pressedButtons === Qt.LeftButton) {
                    timeline.position = (scrollView.flickableItem.contentX + mouse.x) / multitrack.scaleFactor
                    if ((scrollView.flickableItem.contentX > 0 && mouse.x < 50) || (mouse.x > scrollView.width - 50))
                        scrubTimer.start()
                }
            }
            Timer {
                id: scrubTimer
                interval: 25
                repeat: true
                onTriggered: {
                    if (parent.scim || parent.pressedButtons === Qt.LeftButton) {
                        if (parent.mouseX < 50)
                            timeline.position -= 10
                        else if (parent.mouseX > scrollView.flickableItem.contentX - 50)
                            timeline.position += 10
                    }
                    if (parent.mouseX >= 50 && parent.mouseX <= scrollView.width - 50)
                        stop()
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
                        width: tracksContainer.width
                        index: index
                        timeScale: multitrack.scaleFactor
                    }
                }
                ScrollView {
                    id: scrollView
                    width: root.width - headerWidth
                    height: root.height - ruler.height - toolbar.height
        
                    Item {
                        width: tracksContainer.width + headerWidth
                        height: trackHeaders.height + 30 // 30 is padding
                        Column {
                            // These make the striped background for the tracks.
                            // It is important that these are not part of the track visual hierarchy;
                            // otherwise, the clips will be obscured by the Track's background.
                            Repeater {
                                model: multitrack
                                delegate: Rectangle {
                                    width: tracksContainer.width
                                    color: (index === currentTrack)? selectedTrackColor : (index % 2)? activePalette.alternateBase : activePalette.base
                                    height: audio? multitrack.trackHeight : multitrack.trackHeight * 2
                                }
                            }
                        }
                        Column {
                            id: tracksContainer
                            Repeater { id: tracksRepeater; model: trackDelegateModel }
                        }
                    }
                }
            }
            Rectangle {
                id: cursor
                visible: timeline.position > -1
                color: activePalette.text
                width: 1
                height: root.height - scrollView.__horizontalScrollBar.height - toolbar.height
                x: timeline.position * multitrack.scaleFactor - scrollView.flickableItem.contentX
                y: 0
            }
            Canvas {
                id: playhead
                visible: timeline.position > -1
                x: timeline.position * multitrack.scaleFactor - scrollView.flickableItem.contentX - 5
                y: 0
                width: 11
                height: 5
                property bool init: true
                onPaint: {
                    if (init) {
                        init = false;
                        var cx = getContext('2d');
                        cx.fillStyle = activePalette.windowText;
                        cx.beginPath();
                        // Start from the root-left point.
                        cx.lineTo(width, 0);
                        cx.lineTo(width / 2.0, height);
                        cx.lineTo(0, 0);
                        cx.fill();
                        cx.closePath();
                    }
                }
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
            text: toolbar.ripple? qsTr('Insert') : qsTr('Overwrite')
            style: Text.Outline
            styleColor: 'white'
            font.pixelSize: Math.min(Math.max(parent.height * 0.8, 15), 30)
            verticalAlignment: Text.AlignVCenter
        }
    }

    Menu {
        id: menu
        // XXX This is a workaround for menus appearing in wrong location in a Quick
        // view used in a DockWidget on OS X.
        Component.onCompleted: if (timeline.yoffset) __yOffset = timeline.yoffset
        MenuItem {
            text: qsTr('Add Audio Track')
            shortcut: qsTr('Ctrl+U')
            onTriggered: timeline.addAudioTrack();
        }
        MenuItem {
            text: qsTr('Add Video Track')
            shortcut: qsTr('Ctrl+Y')
            onTriggered: timeline.addVideoTrack();
        }
        MenuItem {
            enabled: multitrack.trackHeight >= 50
            text: qsTr('Make Tracks Shorter')
            shortcut: qsTr('Ctrl+K')
            onTriggered: multitrack.trackHeight = Math.max(30, multitrack.trackHeight - 20)
        }
        MenuItem {
            text: qsTr('Make Tracks Taller')
            shortcut: qsTr('Ctrl+L')
            onTriggered: multitrack.trackHeight += 20
        }
        MenuItem {
            text: qsTr('Close')
            shortcut: qsTr('Ctrl+W')
            onTriggered: {
                timeline.close()
                scaleSlider.value = 1
            }
        }
    }

    Keys.onUpPressed: timeline.selectTrack(-1)
    Keys.onDownPressed: timeline.selectTrack(1)
    Keys.onPressed: {
        tracksArea.scim = (event.modifiers === Qt.ShiftModifier)
        switch (event.key) {
        case Qt.Key_B:
            timeline.overwrite(currentTrack)
            break;
        case Qt.Key_C:
            timeline.append(currentTrack)
            break;
        case Qt.Key_S:
            timeline.splitClip(currentTrack)
            break;
        case Qt.Key_V:
            timeline.insert(currentTrack)
            break;
        case Qt.Key_X:
            timeline.remove(currentClipTrack, currentClip)
            currentClip = -1
            break;
        case Qt.Key_Z:
            timeline.lift(currentClipTrack, currentClip)
            currentClip = -1
            break;
        case Qt.Key_Delete:
        case Qt.Key_Backspace:
            if (event.modifiers & Qt.ShiftModifier)
                timeline.remove(currentClipTrack, currentClip)
            else
                timeline.lift(currentClipTrack, currentClip)
            currentClip = -1
            break;
        case Qt.Key_Equal:
            scaleSlider.value += 0.0625
            for (var i = 0; i < tracksRepeater.count; i++)
                tracksRepeater.itemAt(i).redrawWaveforms()
            break;
        case Qt.Key_Minus:
            scaleSlider.value -= 0.0625
            for (var i = 0; i < tracksRepeater.count; i++)
                tracksRepeater.itemAt(i).redrawWaveforms()
            break;
        case Qt.Key_0:
            scaleSlider.value = 1.0
            for (var i = 0; i < tracksRepeater.count; i++)
                tracksRepeater.itemAt(i).redrawWaveforms()
            break;
        default:
            timeline.pressKey(event.key, event.modifiers)
            break;
        }
    }
    Keys.onReleased: {
        tracksArea.scim = false
        switch (event.key) {
        case Qt.Key_B:
        case Qt.Key_C:
        case Qt.Key_S:
        case Qt.Key_V:
        case Qt.Key_X:
        case Qt.Key_Z:
        case Qt.Key_Delete:
        case Qt.Key_Backspace:
        case Qt.Key_Equal:
        case Qt.Key_Minus:
        case Qt.Key_0:
            break;
        default:
            timeline.releaseKey(event.key, event.modifiers)
            break;
        }
    }

    DelegateModel {
        id: trackDelegateModel
        model: multitrack
        Track {
            model: multitrack
            rootIndex: trackDelegateModel.modelIndex(index)
            height: audio? multitrack.trackHeight : multitrack.trackHeight * 2
            width: childrenRect.width
            isAudio: audio
            timeScale: multitrack.scaleFactor
            onClipSelected: {
                currentClip = clip.DelegateModel.itemsIndex
                currentClipTrack = track.DelegateModel.itemsIndex
                for (var i = 0; i < tracksRepeater.count; i++)
                    if (i !== track.DelegateModel.itemsIndex) tracksRepeater.itemAt(i).resetStates();
                timeline.selectClip(currentClipTrack, currentClip)
            }
            onClipDragged: {
                // This provides continuous scrolling at the left/right edges.
                if (x > scrollView.flickableItem.contentX + scrollView.width - 50) {
                    scrollTimer.item = clip
                    scrollTimer.backwards = false
                    scrollTimer.start()
                } else if (x < 50) {
                    scrollView.flickableItem.contentX = 0;
                    scrollTimer.stop()
                } else if (x < scrollView.flickableItem.contentX + 50) {
                    scrollTimer.item = clip
                    scrollTimer.backwards = true
                    scrollTimer.start()
                } else {
                    scrollTimer.stop()
                }
            }
            onClipDropped: scrollTimer.running = false
            onClipDraggedToTrack: {
                var i = clip.trackIndex + direction
                if (i >= 0  && i < tracksRepeater.count) {
                    var track = tracksRepeater.itemAt(i)
                    clip.reparent(track)
                    clip.trackIndex = track.DelegateModel.itemsIndex
                }
            }
            onCheckSnap: {
                for (var i = 0; i < tracksRepeater.count; i++)
                    tracksRepeater.itemAt(i).snapClip(clip)
            }
        }
    }
    
    Connections {
        target: timeline
        onPositionChanged: Logic.scrollIfNeeded()
        onDragging: Logic.dragging(pos, duration)
        onDropped: Logic.dropped()
        onDropAccepted: Logic.acceptDrop()
    }

    Connections {
        target: multitrack
        onLoaded: scaleSlider.value = Math.pow(multitrack.scaleFactor - 0.01, 1.0 / 3.0)
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
