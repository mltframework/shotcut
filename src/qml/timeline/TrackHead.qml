/*
 * Copyright (c) 2013-2016 Meltytech, LLC
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

import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0 as Shotcut

Rectangle {
    id: trackHeadRoot
    property string trackName: ''
    property bool isMute
    property bool isHidden
    property int isComposite
    property bool isLocked
    property bool isVideo
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
            nameEdit.visible = false
            if (mouse.button == Qt.RightButton)
                menu.popup()
        }
    }
    Column {
        id: trackHeadColumn
        spacing: (trackHeadRoot.height < 50)? 0 : 6
        anchors {
            top: parent.top
            left: parent.left
            margins: (trackHeadRoot.height < 50)? 0 : 4
        }

        Rectangle {
            color: 'transparent'
            width: trackHeadRoot.width - trackHeadColumn.anchors.margins * 2
            radius: 2
            border.color: trackNameMouseArea.containsMouse? activePalette.shadow : 'transparent'
            height: nameEdit.height
            MouseArea {
                id: trackNameMouseArea
                height: parent.height
                width: nameEdit.width
                hoverEnabled: true
                onClicked: {
                    nameEdit.visible = true
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
            }
            TextField {
                id: nameEdit
                visible: false
                width: trackHeadRoot.width - trackHeadColumn.anchors.margins * 2
                text: trackName
                onAccepted: {
                    timeline.setTrackName(index, text)
                    visible = false
                }
                onFocusChanged: visible = focus
            }
        }
        RowLayout {
            spacing: 0
            ToolButton {
                id: muteButton
                checked: isMute
                implicitWidth: 16
                implicitHeight: 16
                iconName: isMute ? 'dialog-cancel' : 'player-volume'
                iconSource: isMute ? 'qrc:///icons/oxygen/32x32/actions/dialog-cancel.png' : 'qrc:///icons/oxygen/32x32/actions/player-volume.png'
                onClicked: timeline.toggleTrackMute(index)
                Shotcut.ToolTip { text: qsTr('Mute') }
            }

            ToolButton {
                id: hideButton
                checked: isHidden
                visible: isVideo
                implicitWidth: 16
                implicitHeight: 16
                iconName: isHidden ? 'track-hidden' : 'track-visible'
                iconSource: isHidden? 'qrc:///icons/oxygen/32x32/actions/track-hidden.png' : 'qrc:///icons/oxygen/32x32/actions/track-visible.png'
                onClicked: timeline.toggleTrackHidden(index)
                Shotcut.ToolTip { text: qsTr('Hide') }
            }

            ToolButton {
                id: compositeButton
                visible: isVideo
                checked: isComposite
                implicitWidth: 16
                implicitHeight: 16
                iconName: isComposite ? 'layers' : 'layers-flat'
                iconSource: isComposite ? 'qrc:///icons/oxygen/32x32/actions/layers.png' : 'qrc:///icons/oxygen/32x32/actions/layers-flat.png'
                onClicked: timeline.setTrackComposite(index, checkedState)
                Shotcut.ToolTip { text: qsTr('Composite') }
            }

            ToolButton {
                id: lockButton
                checked: isLocked
                implicitWidth: 16
                implicitHeight: 16
                iconName: isLocked ? 'padlock-closed' : 'padlock-opened'
                iconSource: isLocked ? 'qrc:///icons/oxygen/32x32/actions/padlock-closed.png' : 'qrc:///icons/oxygen/32x32/actions/padlock-opened.png'
                onClicked: timeline.setTrackLock(index, !isLocked)
                Shotcut.ToolTip { text: qsTr('Lock track') }
            }
        }
    }
}
