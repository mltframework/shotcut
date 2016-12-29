/*
 * Copyright (c) 2016-2017 Meltytech, LLC
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
//    property alias trackCount: tracksRepeater.count
    property bool stopScrolling: false
    property color shotcutBlue: Qt.rgba(23/255, 92/255, 118/255, 1.0)

    function clearCurrentFilter() {
    }

    function setCurrentFilter(index) {
        attachedFilters.setCurrentFilter(index)
        selectedIndex = index
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: menu.popup()
    }

    KeyframesToolbar {
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
                id: cornerstone
                property bool selected: false
                // Padding between toolbar and track headers.
                width: headerWidth
                height: ruler.height
                color: selected? shotcutBlue : activePalette.window
                border.color: selected? 'red' : 'transparent'
                border.width: selected? 1 : 0
                z: 1
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
//                    onClicked: timeline.selectMultitrack()
                }
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
                        trackName: attachedfiltersmodel.producerTitle
                        isLocked: false //model.locked
                        width: headerWidth
                        height: Logic.trackHeight(false)
                        selected: false
                        current: 0 === currentTrack
//                        onIsLockedChanged: tracksRepeater.itemAt(index).isLocked = isLocked
                        onClicked: {
                            currentTrack = index
//                            timeline.selectTrackHead(currentTrack)
                        }
                    }
//                    Repeater {
//                        id: trackHeaderRepeater
//                        model: multitrack
//                        ParameterHead {
//                            trackName: model.name
//                            isMute: model.mute
//                            isHidden: model.hidden
//                            isComposite: model.composite
//                            isLocked: model.locked
//                            isVideo: !model.audio
//                            width: headerWidth
//                            height: Logic.trackHeight(model.audio)
//                            selected: false
//                            current: index === currentTrack
//                            onIsLockedChanged: tracksRepeater.itemAt(index).isLocked = isLocked
//                            onClicked: {
//                                currentTrack = index
//                                timeline.selectTrackHead(currentTrack)
//                            }
//                        }
//                    }
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
//            onClicked: timeline.position = (scrollView.flickableItem.contentX + mouse.x) / multitrack.scaleFactor
            property bool scim: false
            onReleased: scim = false
            onExited: scim = false
            onPositionChanged: {
                if (mouse.modifiers === Qt.ShiftModifier || mouse.buttons === Qt.LeftButton) {
//                    timeline.position = (scrollView.flickableItem.contentX + mouse.x) / multitrack.scaleFactor
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
//                         && (timeline.position * multitrack.scaleFactor >= 50)
//                onTriggered: {
//                    if (parent.mouseX < 50)
//                        timeline.position -= 10
//                    else
//                        timeline.position += 10
//                }
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
//                            Repeater {
//                                model: multitrack
//                                delegate: Rectangle {
//                                    width: tracksContainer.width
//                                    color: (index === currentTrack)? selectedTrackColor : (index % 2)? activePalette.alternateBase : activePalette.base
//                                    height: Logic.trackHeight(audio)
//                                }
//                            }
                        }
                        Column {
                            id: tracksContainer
//                            Repeater { id: tracksRepeater; model: trackDelegateModel }
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
                x: -scrollView.flickableItem.contentX // timeline.position * multitrack.scaleFactor - scrollView.flickableItem.contentX
                y: 0
            }
            TimelinePlayhead {
                id: playhead
                visible: timeline.position > -1
                x: - scrollView.flickableItem.contentX - 5 //timeline.position * multitrack.scaleFactor - scrollView.flickableItem.contentX - 5
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
        onPopupVisibleChanged: {
            if (visible && application.OS === 'Windows' && __popupGeometry.height > 0) {
                // Try to fix menu running off screen. This only works intermittently.
                menu.__yOffset = Math.min(0, Screen.height - (__popupGeometry.y + __popupGeometry.height + 40))
                menu.__xOffset = Math.min(0, Screen.width - (__popupGeometry.x + __popupGeometry.width))
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
