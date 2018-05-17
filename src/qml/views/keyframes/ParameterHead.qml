/*
 * Copyright (c) 2017-2018 Meltytech, LLC
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
    id: paramHeadRoot
    property string trackName: ''
    property bool isLocked: false
    property bool selected: false
    property bool current: false
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
    Column {
        id: trackHeadColumn
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
            width: paramHeadRoot.width - trackHeadColumn.anchors.margins * 2 - 8
        }
        RowLayout {
            spacing: 8
            ToolButton {
                id: previousButton
                implicitWidth: 20
                implicitHeight: 20
                iconName: 'media-skip-backward'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/media-skip-backward.png'
                onClicked: {
                    if (delegateIndex >= 0) {
                        root.selection = [keyframes.seekPrevious()]
                        root.currentTrack = delegateIndex
                    } else {
                        var position = producer.position + producer.in
                        if (position > filter.out)
                            position = filter.out
                        else if (position > filter.out - filter.animateOut)
                            position = filter.out - filter.animateOut
                        else if (producer.position > filter.in + filter.animateIn)
                            position = filter.in + filter.animateIn
                        else if (producer.position > filter.in)
                            position = filter.in
                        else
                            position = 0
                        producer.position = position - producer.in
                    }
                }
                tooltip: (delegateIndex >= 0) ? qsTr('Seek to previous keyframe') : qsTr('Seek backwards')
            }

            ToolButton {
                id: deleteButton
                visible: delegateIndex >= 0
                enabled: root.selection.length > 0
                implicitWidth: 20
                implicitHeight: 20
                iconName: 'edit-delete'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/edit-delete.png'
                opacity: enabled? 1.0 : 0.5
                onClicked: {
                    parameters.remove(root.currentTrack, root.selection[0])
                    root.selection = []
                }
                tooltip: qsTr('Delete the selected keyframe')
            }
            Item {
                visible: delegateIndex < 0
                width: 20
                height: 20
            }

            ToolButton {
                id: nextButton
                implicitWidth: 20
                implicitHeight: 20
                iconName: 'media-skip-forward'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/media-skip-forward.png'
                onClicked: {
                    if (delegateIndex >= 0) {
                        root.selection = [keyframes.seekNext()]
                        root.currentTrack = delegateIndex
                    } else {
                        var position = producer.position + producer.in
                        if (position < filter.in)
                            position = filter.in
                        else if (position < filter.in + filter.animateIn)
                            position = filter.in + filter.animateIn
                        else if (position < filter.out - filter.animateOut)
                            position = filter.out - filter.animateOut
                        else if (position < filter.out)
                            position = filter.out
                        else
                            position = producer.out
                        producer.position = position - producer.in
                    }
                }
                tooltip: (delegateIndex >= 0) ? qsTr('Seek to next keyframe') : qsTr('Seek forwards')
            }

            ToolButton {
                id: lockButton
                visible: false && delegateIndex >= 0
                implicitWidth: 20
                implicitHeight: 20
                iconName: isLocked ? 'object-locked' : 'object-unlocked'
                iconSource: isLocked ? 'qrc:///icons/oxygen/32x32/status/object-locked.png' : 'qrc:///icons/oxygen/32x32/status/object-unlocked.png'
//                onClicked: timeline.setTrackLock(index, !isLocked)
                tooltip: isLocked? qsTr('Unlock track') : qsTr('Lock track')
            }
        }
    }
}
