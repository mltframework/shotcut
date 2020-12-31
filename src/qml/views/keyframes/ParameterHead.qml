/*
 * Copyright (c) 2017-2020 Meltytech, LLC
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
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
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
                color: Qt.rgba(selectedTrackColor.r * selectedTrackColor.a + activePalette.window.r * (1.0 - selectedTrackColor.a),
                               selectedTrackColor.g * selectedTrackColor.a + activePalette.window.g * (1.0 - selectedTrackColor.a),
                               selectedTrackColor.b * selectedTrackColor.a + activePalette.window.b * (1.0 - selectedTrackColor.a),
                               1.0)
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
            leftMargin: (trackHeadRoot.height < 50)? 0 : 4
            rightMargin: 4
            topMargin: 4
            bottomMargin: 4
        }

        Control {
            id: control
            contentItem: Label {
                text: trackName
                color: activePalette.windowText
                elide: Qt.ElideRight
                x: 4
                y: 3
                width: paramHeadRoot.width - trackHeadColumn.anchors.margins * 2 - 8 - (paramHeadRoot.height < 30? 90 : 0)
            }
            Shotcut.HoverTip { text: trackName }
        }
        RowLayout {
            ToolButton {
                id: previousButton
                icon.name: 'media-skip-backward'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/media-skip-backward.png'
                onClicked: {
                    if (delegateIndex >= 0) {
                        root.currentTrack = delegateIndex
                        root.selection = [keyframes.seekPrevious()]
                    } else {
                        Logic.seekPreviousSimple()
                    }
                }
                Shotcut.HoverTip { text: (delegateIndex >= 0) ? qsTr('Seek to previous keyframe') : qsTr('Seek backwards') }
            }

            ToolButton {
                id: addButton
                visible: delegateIndex >= 0
                icon.name: 'chronometer';
                icon.source: 'qrc:///icons/oxygen/32x32/actions/chronometer.png'
                onClicked: {
                    parameters.addKeyframe(delegateIndex, producer.position - (filter.in - producer.in))
                    root.selection = [parameters.keyframeIndex(delegateIndex, producer.position)]
                }
                Shotcut.HoverTip { text: qsTr('Add a keyframe at play head') }
            }
            Item {
                visible: delegateIndex < 0 && paramHeadRoot.height >= 30
                width: 24
                height: 24
            }

            ToolButton {
                id: deleteButton
                visible: delegateIndex >= 0
                enabled: delegateIndex === root.currentTrack && root.selection.length > 0
                icon.name: 'edit-delete'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-delete.png'
                opacity: enabled? 1.0 : 0.5
                onClicked: {
                    parameters.remove(delegateIndex, root.selection[0])
                    root.selection = []
                }
                Shotcut.HoverTip { text: qsTr('Delete the selected keyframe') }
            }
            Item {
                visible: delegateIndex < 0 && paramHeadRoot.height >= 30
                width: 24
                height: 24
            }

            ToolButton {
                id: nextButton
                icon.name: 'media-skip-forward'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/media-skip-forward.png'
                onClicked: {
                    if (delegateIndex >= 0) {
                        root.currentTrack = delegateIndex
                        root.selection = [keyframes.seekNext()]
                    } else {
                        Logic.seekNextSimple()
                    }
                }
                Shotcut.HoverTip { text: (delegateIndex >= 0) ? qsTr('Seek to next keyframe') : qsTr('Seek forwards') }
            }

            ToolButton {
                id: lockButton
                visible: false && delegateIndex >= 0
                icon.name: isLocked ? 'object-locked' : 'object-unlocked'
                icon.source: isLocked ? 'qrc:///icons/oxygen/32x32/status/object-locked.png' : 'qrc:///icons/oxygen/32x32/status/object-unlocked.png'
//                onClicked: timeline.setTrackLock(index, !isLocked)
                Shotcut.HoverTip { text: isLocked? qsTr('Unlock track') : qsTr('Lock track') }
            }
        }
    }
}
