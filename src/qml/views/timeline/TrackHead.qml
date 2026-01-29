/*
 * Copyright (c) 2013-2025 Meltytech, LLC
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
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

Rectangle {
    id: trackHeadRoot

    property string trackName: ''
    property bool isMute
    property bool isHidden
    property bool isComposite
    property bool isLocked
    property bool isVideo
    property bool isFiltered
    property bool isTopVideo
    property bool isBottomVideo
    property bool isTopAudio
    property bool isBottomAudio
    property bool selected: false
    property bool current: false

    signal clicked

    function pulseLockButton() {
        lockButtonAnim.restart();
    }

    color: selected ? selectedTrackColor : (index % 2) ? activePalette.alternateBase : activePalette.base
    border.color: selected ? 'red' : 'transparent'
    border.width: selected ? 1 : 0
    clip: true
    state: 'normal'
    states: [
        State {
            name: 'selected'
            when: trackHeadRoot.selected

            PropertyChanges {
                target: trackHeadRoot
                color: isVideo ? root.shotcutBlue : 'darkseagreen'
            }
        },
        State {
            name: 'current'
            when: trackHeadRoot.current

            PropertyChanges {
                target: trackHeadRoot
                color: Qt.rgba(selectedTrackColor.r * selectedTrackColor.a + activePalette.window.r * (1 - selectedTrackColor.a), selectedTrackColor.g * selectedTrackColor.a + activePalette.window.g * (1 - selectedTrackColor.a), selectedTrackColor.b * selectedTrackColor.a + activePalette.window.b * (1 - selectedTrackColor.a), 1)
            }
        },
        State {
            when: !trackHeadRoot.selected && !trackHeadRoot.current
            name: 'normal'

            PropertyChanges {
                target: trackHeadRoot
                color: (index % 2) ? activePalette.alternateBase : activePalette.base
            }
        }
    ]
    transitions: [
        Transition {
            to: '*'

            ColorAnimation {
                target: trackHeadRoot
                duration: 100
            }
        }
    ]

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            parent.clicked();
            nameEdit.focus = false;
            if (mouse.button === Qt.RightButton)
                root.timelineRightClicked();
        }
    }

    Flow {
        id: trackHeadColumn

        flow: (trackHeadRoot.height < 50) ? Flow.LeftToRight : Flow.TopToBottom
        spacing: (trackHeadRoot.height < 50) ? 0 : 6

        anchors {
            top: parent.top
            left: parent.left
            leftMargin: 8
            rightMargin: (trackHeadRoot.height < 50) ? 0 : 4
            topMargin: (trackHeadRoot.height < 50) ? 0 : 4
            bottomMargin: (trackHeadRoot.height < 50) ? 0 : 4
        }

        Rectangle {
            color: 'transparent'
            width: trackHeadRoot.width - trackHeadColumn.anchors.margins * 2 - (trackHeadRoot.height < 50 ? 120 : 0)
            radius: 2
            border.color: (!timeline.isFloating() && trackNameMouseArea.containsMouse) ? activePalette.shadow : 'transparent'
            height: nameEdit.height

            MouseArea {
                id: trackNameMouseArea

                height: parent.height
                width: nameEdit.width
                hoverEnabled: true
                onClicked: {
                    if (!timeline.isFloating()) {
                        nameEdit.focus = true;
                        nameEdit.selectAll();
                    }
                }
            }

            Control {
                Shotcut.HoverTip {
                    text: trackName
                }

                contentItem: Label {
                    text: trackName
                    color: activePalette.windowText
                    elide: Qt.ElideRight
                    leftPadding: 4
                    topPadding: 3
                    width: nameEdit.width
                }
            }

            TextField {
                id: nameEdit

                visible: focus
                width: parent.width
                selectByMouse: true
                text: trackName
                onEditingFinished: {
                    timeline.setTrackName(index, text);
                    focus = false;
                }
                Keys.onTabPressed: editingFinished()
            }
        }

        RowLayout {
            spacing: 8

            ToolButton {
                id: lockButton

                icon.name: isLocked ? 'object-locked' : 'object-unlocked'
                icon.source: isLocked ? 'qrc:///icons/oxygen/32x32/status/object-locked.png' : 'qrc:///icons/oxygen/32x32/status/object-unlocked.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus
                onClicked: timeline.setTrackLock(index, !isLocked)
                transformOrigin: Item.Center

                Shotcut.HoverTip {
                    text: isLocked ? qsTr('Unlock track') : qsTr('Lock track')
                }

                SequentialAnimation {
                    id: lockButtonAnim

                    loops: 2

                    NumberAnimation {
                        target: lockButton
                        property: 'scale'
                        to: 2
                        duration: 200
                    }

                    NumberAnimation {
                        target: lockButton
                        property: 'scale'
                        to: 1
                        duration: 200
                    }
                }
            }

            ToolButton {
                id: muteButton

                icon.name: isMute ? 'audio-volume-muted' : 'audio-volume-high'
                icon.source: isMute ? 'qrc:///icons/oxygen/32x32/status/audio-volume-muted.png' : 'qrc:///icons/oxygen/32x32/status/audio-volume-high.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus

                MouseArea {
                    anchors.fill: parent
                    onClicked: (mouse) => {
                        if (mouse.modifiers & Qt.AltModifier) {
                            timeline.toggleOtherTracksMute(index)
                        } else {
                            timeline.toggleTrackMute(index)
                        }
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Mute/Unmute - Alt+Click to toggle mute of other tracks')
                }
            }

            ToolButton {
                id: hideButton

                visible: isVideo
                icon.name: isHidden ? 'layer-visible-off' : 'layer-visible-on'
                icon.source: isHidden ? 'qrc:///icons/oxygen/32x32/actions/layer-visible-off.png' : 'qrc:///icons/oxygen/32x32/actions/layer-visible-on.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus

                MouseArea {
                    anchors.fill: parent
                    onClicked: (mouse) => {
                        if (mouse.modifiers & Qt.AltModifier) {
                            timeline.toggleOtherTracksHidden(index)
                        } else {
                            timeline.toggleTrackHidden(index)
                        }
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Show/Hide - Alt+Click to toggle visibility of other tracks')
                }
            }

            ToolButton {
                visible: isFiltered
                icon.name: 'view-filter'
                icon.source: 'qrc:///icons/oxygen/32x32/status/view-filter.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus
                onClicked: {
                    trackHeadRoot.clicked();
                    nameEdit.focus = false;
                    timeline.filteredClicked();
                }

                Shotcut.HoverTip {
                    text: qsTr('Filters')
                }
            }
        }
    }
}
