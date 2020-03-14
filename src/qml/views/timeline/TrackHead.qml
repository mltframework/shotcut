/*
 * Copyright (c) 2013-2020 Meltytech, LLC
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

import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Rectangle {
    id: trackHeadRoot
    property string trackName: ''
    property bool isMute
    property bool isHidden
    property bool isComposite
    property bool isLocked
    property bool isVideo
    property bool isFiltered
    property bool isBottomVideo
    property bool selected: false
    property bool current: false
    signal clicked()

    function pulseLockButton() {
        lockButtonAnim.restart();
    }

    SystemPalette { id: activePalette }
    color: selected ? selectedTrackColor : (index % 2)? activePalette.alternateBase : activePalette.base
    border.color: selected? 'red' : 'transparent'
    border.width: selected? 1 : 0
    clip: true
    state: 'normal'
    states: [
        State {
            name: 'selected'
            when: trackHeadRoot.selected
            PropertyChanges {
                target: trackHeadRoot
                color: isVideo? root.shotcutBlue : 'darkseagreen'
            }
        },
        State {
            name: 'current'
            when: trackHeadRoot.current
            PropertyChanges {
                target: trackHeadRoot
                color: selectedTrackColor
            }
        },
        State {
            when: !trackHeadRoot.selected && !trackHeadRoot.current
            name: 'normal'
            PropertyChanges {
                target: trackHeadRoot
                color: (index % 2)? activePalette.alternateBase : activePalette.base
            }
        }
    ]
    transitions: [
        Transition {
            to: '*'
            ColorAnimation { target: trackHeadRoot; duration: 100 }
        }
    ]

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            parent.clicked()
            nameEdit.focus = false
            if (mouse.button == Qt.RightButton)
                menu.popup()
        }
    }
    Flow {
        id: trackHeadColumn
        flow: (trackHeadRoot.height < 50)? Flow.LeftToRight : Flow.TopToBottom
        spacing: (trackHeadRoot.height < 50)? 0 : 6
        anchors {
            top: parent.top
            left: parent.left
            margins: (trackHeadRoot.height < 50)? 0 : 4
        }

        Rectangle {
            color: 'transparent'
            width: trackHeadRoot.width - trackHeadColumn.anchors.margins * 2 - (trackHeadRoot.height < 50? 100 : 0)
            radius: 2
            border.color: (!timeline.isFloating() && trackNameMouseArea.containsMouse)? activePalette.shadow : 'transparent'
            height: nameEdit.height
            MouseArea {
                id: trackNameMouseArea
                height: parent.height
                width: nameEdit.width
                hoverEnabled: true
                onClicked: if (!timeline.isFloating()) {
                    nameEdit.focus = true
                    nameEdit.selectAll()
                }
            }
            Label {
                text: trackName
                color: activePalette.windowText
                elide: Qt.ElideRight
                x: 4
                y: 3
                width: parent.width - 8
                ToolTip{ text: parent.text }
            }
            TextField {
                id: nameEdit
                visible: focus
                width: parent.width
                text: trackName
                onEditingFinished: {
                    timeline.setTrackName(index, text)
                    focus = false
                }
            }
        }
        RowLayout {
            spacing: 8
            ToolButton {
                id: lockButton
                implicitWidth: 18
                implicitHeight: 18
                height: width
                iconName: isLocked ? 'object-locked' : 'object-unlocked'
                iconSource: isLocked ? 'qrc:///icons/oxygen/32x32/status/object-locked.png' : 'qrc:///icons/oxygen/32x32/status/object-unlocked.png'
                onClicked: timeline.setTrackLock(index, !isLocked)
                tooltip: isLocked? qsTr('Unlock track') : qsTr('Lock track')

                SequentialAnimation {
                    id: lockButtonAnim
                    loops: 2
                    NumberAnimation {
                        target: lockButton
                        property: 'width'
                        to: 32
                        duration: 200
                    }
                    NumberAnimation {
                        target: lockButton
                        property: 'width'
                        to: 18
                        duration: 200
                    }
                }
            }

            ToolButton {
                id: muteButton
                implicitWidth: 18
                implicitHeight: 18
                iconName: isMute ? 'audio-volume-muted' : 'audio-volume-high'
                iconSource: isMute ? 'qrc:///icons/oxygen/32x32/status/audio-volume-muted.png' : 'qrc:///icons/oxygen/32x32/status/audio-volume-high.png'
                onClicked: timeline.toggleTrackMute(index)
                tooltip: isMute? qsTr('Unmute') : qsTr('Mute')
            }

            ToolButton {
                id: hideButton
                visible: isVideo
                implicitWidth: 18
                implicitHeight: 18
                iconName: isHidden ? 'layer-visible-off' : 'layer-visible-on'
                iconSource: isHidden? 'qrc:///icons/oxygen/32x32/actions/layer-visible-off.png' : 'qrc:///icons/oxygen/32x32/actions/layer-visible-on.png'
                onClicked: timeline.toggleTrackHidden(index)
                tooltip: isHidden? qsTr('Show') : qsTr('Hide')
            }

            ToolButton {
                visible: isFiltered
                anchors.right: parent.right
                implicitWidth: 18
                implicitHeight: 18
                iconName: 'view-filter'
                iconSource: 'qrc:///icons/oxygen/32x32/status/view-filter.png'
                tooltip: qsTr('Filters')
                onClicked: {
                    trackHeadRoot.clicked()
                    nameEdit.focus = false
                    timeline.filteredClicked()
                }
            }
        }
    }
}
