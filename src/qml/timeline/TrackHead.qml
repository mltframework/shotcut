/*
 * Copyright (c) 2013 Meltytech, LLC
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
        acceptedButtons: Qt.LeftButton
        onClicked: {
            parent.clicked()
            nameEdit.visible = false
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
            CheckBox {
                id: muteButton
                checked: isMute
                style: CheckBoxStyle {
                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 2
                        color: isMute? activePalette.highlight : trackHeadRoot.color
                        border.color: activePalette.shadow
                        border.width: 1
                        Text {
                            id: muteText
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr('M', 'Mute')
                            color: isMute? activePalette.highlightedText : activePalette.windowText
                        }
                    }
                }
                onClicked: timeline.toggleTrackMute(index)
                Shotcut.ToolTip { text: qsTr('Mute') }
            }

            CheckBox {
                id: hideButton
                checked: isHidden
                visible: isVideo
                style: CheckBoxStyle {
                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 2
                        color: isHidden? activePalette.highlight : trackHeadRoot.color
                        border.color: activePalette.shadow
                        border.width: 1
                        Text {
                            id: hideText
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr('H', 'Hide')
                            color: isHidden? activePalette.highlightedText : activePalette.windowText
                        }
                    }
                }
                onClicked: timeline.toggleTrackHidden(index)
                Shotcut.ToolTip { text: qsTr('Hide') }
            }

            CheckBox {
                id: compositeButton
                visible: isVideo
                checked: isComposite
                style: CheckBoxStyle {
                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 2
                        color: isComposite? activePalette.highlight : trackHeadRoot.color
                        border.color: activePalette.shadow
                        border.width: 1
                        Text {
                            id: compositeText
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr('C', 'Composite')
                            color: isComposite? activePalette.highlightedText : activePalette.windowText
                        }
                    }
                }
                onClicked: timeline.setTrackComposite(index, checkedState)
                Shotcut.ToolTip { text: qsTr('Composite') }
            }

            CheckBox {
                id: lockButton
                checked: isLocked
                style: CheckBoxStyle {
                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 2
                        color: isLocked ? activePalette.highlight : trackHeadRoot.color
                        border.color: activePalette.shadow
                        border.width: 1
                        Text {
                            id: lockText
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr('L', 'Lock')
                            color: isLocked ? activePalette.highlightedText : activePalette.windowText
                        }
                    }
                }
                SequentialAnimation {
                    id: lockButtonAnim
                    loops: 2
                    NumberAnimation {
                        target: lockButton
                        property: "scale"
                        to: 1.8
                        duration: 200
                    }
                    NumberAnimation {
                        target: lockButton
                        property: "scale"
                        to: 1
                        duration: 200
                    }
                }

                onClicked: timeline.setTrackLock(index, !isLocked)
                Shotcut.ToolTip { text: qsTr('Lock track') }
            }
        }
    }
}
