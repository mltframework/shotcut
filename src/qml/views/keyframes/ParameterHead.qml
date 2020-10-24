/*
 * Copyright (c) 2017-2019 Meltytech, LLC
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
import 'Keyframes.js' as Logic

Rectangle {
    id: paramHeadRoot
    property string trackName: ''
    property bool isLocked: false
    property bool selected: false
    property bool current: false
    property bool isCurve: false
    property int delegateIndex: -1
    signal clicked()

    SystemPalette { id: activePalette }
    color: selected ? selectedTrackColor : (delegateIndex % 2)? activePalette.alternateBase : activePalette.base
    border.color: selected? 'red' : 'transparent'
    border.width: selected? 1 : 0
    clip: true
    state: 'normal'
    states: [
        State {
            name: 'selected'
            when: paramHeadRoot.selected
            PropertyChanges {
                target: paramHeadRoot
                color: root.shotcutBlue
            }
        },
        State {
            name: 'current'
            when: paramHeadRoot.current
            PropertyChanges {
                target: paramHeadRoot
                color: selectedTrackColor
            }
        },
        State {
            name: 'normal'
            when: !paramHeadRoot.selected && !paramHeadRoot.current
            PropertyChanges {
                target: paramHeadRoot
                color: (delegateIndex % 2)? activePalette.alternateBase : activePalette.base
            }
        }
    ]
    transitions: [
        Transition {
            to: '*'
            ColorAnimation { target: paramHeadRoot; duration: 100 }
        }
    ]

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            parent.clicked()
            if (mouse.button == Qt.RightButton)
                menu.popup()
        }
    }
    Flow {
        id: trackHeadColumn
        flow: (paramHeadRoot.height < 30)? Flow.LeftToRight : Flow.TopToBottom
        spacing: (paramHeadRoot.height < 50)? 0 : 6
        anchors {
            top: parent.top
            left: parent.left
            margins: (paramHeadRoot.height < 50)? 0 : 4
        }

        Label {
            text: trackName
            color: activePalette.windowText
            elide: Qt.ElideRight
            x: 4
            y: 3
            width: paramHeadRoot.width - trackHeadColumn.anchors.margins * 2 - 8 - (paramHeadRoot.height < 30? 90 : 0)
            Shotcut.ToolTip { text: parent.text }
        }
        RowLayout {
            spacing: 8
            ToolButton {
                id: previousButton
                implicitWidth: 18
                implicitHeight: 18
                iconName: 'media-skip-backward'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/media-skip-backward.png'
                onClicked: {
                    if (delegateIndex >= 0) {
                        root.currentTrack = delegateIndex
                        root.selection = [keyframes.seekPrevious()]
                    } else {
                        Logic.seekPreviousSimple()
                    }
                }
                tooltip: (delegateIndex >= 0) ? qsTr('Seek to previous keyframe') : qsTr('Seek backwards')
            }

            ToolButton {
                id: addButton
                visible: delegateIndex >= 0
                implicitWidth: 18
                implicitHeight: 18
                iconName: 'chronometer';
                iconSource: 'qrc:///icons/oxygen/32x32/actions/chronometer.png'
                onClicked: {
                    parameters.addKeyframe(delegateIndex, producer.position - (filter.in - producer.in))
                    root.selection = [parameters.keyframeIndex(delegateIndex, producer.position)]
                }
                tooltip: qsTr('Add a keyframe at play head')
            }
            Item {
                visible: delegateIndex < 0 && paramHeadRoot.height >= 30
                width: 18
                height: 18
            }

            ToolButton {
                id: deleteButton
                visible: delegateIndex >= 0
                enabled: delegateIndex === root.currentTrack && root.selection.length > 0
                implicitWidth: 18
                implicitHeight: 18
                iconName: 'edit-delete'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/edit-delete.png'
                opacity: enabled? 1.0 : 0.5
                onClicked: {
                    parameters.remove(delegateIndex, root.selection[0])
                    root.selection = []
                }
                tooltip: qsTr('Delete the selected keyframe')
            }
            Item {
                visible: delegateIndex < 0 && paramHeadRoot.height >= 30
                width: 18
                height: 18
            }

            ToolButton {
                id: nextButton
                implicitWidth: 18
                implicitHeight: 18
                iconName: 'media-skip-forward'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/media-skip-forward.png'
                onClicked: {
                    if (delegateIndex >= 0) {
                        root.currentTrack = delegateIndex
                        root.selection = [keyframes.seekNext()]
                    } else {
                        Logic.seekNextSimple()
                    }
                }
                tooltip: (delegateIndex >= 0) ? qsTr('Seek to next keyframe') : qsTr('Seek forwards')
            }

            ToolButton {
                id: lockButton
                visible: false && delegateIndex >= 0
                implicitWidth: 18
                implicitHeight: 18
                iconName: isLocked ? 'object-locked' : 'object-unlocked'
                iconSource: isLocked ? 'qrc:///icons/oxygen/32x32/status/object-locked.png' : 'qrc:///icons/oxygen/32x32/status/object-unlocked.png'
//                onClicked: timeline.setTrackLock(index, !isLocked)
                tooltip: isLocked? qsTr('Unlock track') : qsTr('Lock track')
            }
        }
    }
}
