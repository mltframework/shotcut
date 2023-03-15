/*
 * Copyright (c) 2017-2022 Meltytech, LLC
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
import "Keyframes.js" as Logic
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

Rectangle {
    id: paramHeadRoot

    property string trackName: ''
    property bool isLocked: false
    property bool selected: false
    property bool current: false
    property bool isCurve: false
    property bool zoomHeight: false
    property int delegateIndex: -1

    signal clicked
    signal rightClicked

    color: selected ? selectedTrackColor : (delegateIndex % 2) ? activePalette.alternateBase : activePalette.base
    border.color: selected ? 'red' : 'transparent'
    border.width: selected ? 1 : 0
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
                color: Qt.rgba(selectedTrackColor.r * selectedTrackColor.a + activePalette.window.r * (1 - selectedTrackColor.a), selectedTrackColor.g * selectedTrackColor.a + activePalette.window.g * (1 - selectedTrackColor.a), selectedTrackColor.b * selectedTrackColor.a + activePalette.window.b * (1 - selectedTrackColor.a), 1)
            }
        },
        State {
            name: 'normal'
            when: !paramHeadRoot.selected && !paramHeadRoot.current

            PropertyChanges {
                target: paramHeadRoot
                color: (delegateIndex % 2) ? activePalette.alternateBase : activePalette.base
            }
        }
    ]
    transitions: [
        Transition {
            to: '*'

            ColorAnimation {
                target: paramHeadRoot
                duration: 100
            }
        }
    ]

    SystemPalette {
        id: activePalette
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            parent.clicked();
            if (mouse.button == Qt.RightButton)
                parent.rightClicked();
        }
    }

    Flow {
        id: trackHeadColumn

        flow: (paramHeadRoot.height < 30) ? Flow.LeftToRight : Flow.TopToBottom
        spacing: (paramHeadRoot.height < 50) ? 0 : 6

        anchors {
            top: parent.top
            left: parent.left
            margins: (paramHeadRoot.height < 50) ? 0 : 4
        }

        Control {
            id: control

            width: paramHeadRoot.width - trackHeadColumn.anchors.margins * 2 - 8 - (paramHeadRoot.height < 30 ? toolButtonsLayout.width : 0)

            Shotcut.HoverTip {
                text: trackName
            }

            contentItem: Label {
                text: trackName
                color: activePalette.windowText
                elide: Qt.ElideRight
                leftPadding: 4
                topPadding: 3
                width: control.width
            }
        }

        RowLayout {
            id: toolButtonsLayout

            spacing: (paramHeadRoot.height < 30) ? 0 : 6

            ToolButton {
                id: previousButton

                icon.name: 'media-skip-backward'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/media-skip-backward.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus
                onClicked: {
                    if (delegateIndex >= 0) {
                        root.currentTrack = delegateIndex;
                        root.selection = [keyframes.seekPrevious()];
                    } else {
                        Logic.seekPreviousSimple();
                    }
                }

                Shotcut.HoverTip {
                    text: (delegateIndex >= 0) ? qsTr('Seek to previous keyframe') : qsTr('Seek backwards')
                }
            }

            ToolButton {
                id: addButton

                visible: delegateIndex >= 0
                icon.name: 'chronometer'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/chronometer.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus
                onClicked: {
                    parameters.addKeyframe(delegateIndex, producer.position - (filter.in - producer.in));
                    root.selection = [parameters.keyframeIndex(delegateIndex, producer.position)];
                }

                Shotcut.HoverTip {
                    text: qsTr('Add a keyframe at play head')
                }
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
                icon.width: 16
                icon.height: 16
                padding: 1
                icon.name: 'edit-delete'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-delete.png'
                opacity: enabled ? 1 : 0.5
                focusPolicy: Qt.NoFocus
                onClicked: {
                    parameters.remove(delegateIndex, root.selection[0]);
                    root.selection = [];
                }

                Shotcut.HoverTip {
                    text: qsTr('Delete the selected keyframe')
                }
            }

            Item {
                visible: delegateIndex < 0 && paramHeadRoot.height >= 30
                width: 18
                height: 18
            }

            ToolButton {
                id: nextButton

                icon.name: 'media-skip-forward'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/media-skip-forward.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus
                onClicked: {
                    if (delegateIndex >= 0) {
                        root.currentTrack = delegateIndex;
                        root.selection = [keyframes.seekNext()];
                    } else {
                        Logic.seekNextSimple();
                    }
                }

                Shotcut.HoverTip {
                    text: (delegateIndex >= 0) ? qsTr('Seek to next keyframe') : qsTr('Seek forwards')
                }
            }

            ToolButton {
                id: lockButton

                visible: false && delegateIndex >= 0
                icon.name: isLocked ? 'object-locked' : 'object-unlocked'
                icon.source: isLocked ? 'qrc:///icons/oxygen/32x32/status/object-locked.png' : 'qrc:///icons/oxygen/32x32/status/object-unlocked.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus

                //                onClicked: timeline.setTrackLock(index, !isLocked)
                Shotcut.HoverTip {
                    text: isLocked ? qsTr('Unlock track') : qsTr('Lock track')
                }
            }

            Shotcut.ToolButton {
                focusPolicy: Qt.NoFocus
                visible: delegateIndex >= 0 && paramHeadRoot.isCurve
                checkable: true
                checked: zoomHeight

                Shotcut.HoverTip {
                    text: qsTr('Zoom keyframe values')
                }

                action: Action {
                    id: zoomFitKeyframeAction

                    icon.name: 'zoom-fit-best'
                    icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-fit-best.png'
                    onTriggered: {
                        zoomHeight = !zoomHeight;
                        root.paramRepeater.itemAt(delegateIndex).setMinMax(zoomHeight);
                    }
                }
            }
        }
    }
}
