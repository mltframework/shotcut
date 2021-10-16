/*
 * Copyright (c) 2021 Meltytech, LLC
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
import QtQuick.Shapes 1.12

Item {
    id: root
    property real timeScale: 1.0;
    property var start: 0
    property var end: 0
    property var markerColor: 'black'
    property var text: ""
    property var index: 0
    signal editRequested(int index)
    signal deleteRequested(int index)
    x: 0
    width: parent.width
    height: 17

    SystemPalette { id: activePalette }

    onTimeScaleChanged: {
        updatePosition()
    }
    onStartChanged: {
        updatePosition()
    }
    onEndChanged: {
        updatePosition()
    }

    function updatePosition() {
        markerStart.x = (start * timeScale) - 7
        markerEnd.x = end * timeScale
    }

    Menu {
        id: menu
        title: qsTr('Marker Operations')
        MenuItem {
            text: qsTr('Edit...')
            onTriggered: {
                menu.dismiss()
                root.editRequested(root.index)
            }
        }
        MenuItem {
            text: qsTr('Delete')
            onTriggered: root.deleteRequested(root.index)
        }
    }

    Shape {
        id: markerStart
        width: 7
        height: 17
        x: (start * timeScale) - 7
        antialiasing: true
        ShapePath {
            strokeWidth: 1
            strokeColor: 'transparent'
            fillColor: root.markerColor
            startX: 0
            startY: 0
            PathLine { x: 0; y: 10 }
            PathLine { x: 7; y: 17 }
            PathLine { x: 7; y: 0 }
            PathLine { x: 0; y: 0 }
        }

        MouseArea {
            id: startMouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            property int lockWidth: 0
            property var dragStartX: 0
            drag {
                target: pressedButtons & Qt.LeftButton ? parent : undefined
                axis: Drag.XAxis
                threshold: 0
                minimumX: 0
                maximumX: startMouseArea.lockWidth == -1 ? markerEnd.x - 7 : root.width
            }
            onPressed: {
               dragStartX = markerStart.x
               if (mouse.modifiers & Qt.ControlModifier) {
                   lockWidth = -1
               } else if (mouse.button === Qt.LeftButton) {
                   lockWidth = markerEnd.x - markerStart.x
               }
            }
            onPositionChanged: {
                if (lockWidth != -1) {
                    markerEnd.x = markerStart.x + lockWidth
                }
            }
            onReleased: {
                if (mouse.button == Qt.LeftButton && markerStart.x != dragStartX) {
                    markers.move(index, Math.round((markerStart.x + 7) / timeScale), Math.round(markerEnd.x / timeScale))
                }
            }
            onClicked: {
                if (mouse.button === Qt.RightButton) {
                    menu.popup()
                } else {
                    timeline.position = start
                }
            }
        }
    }

    Shape {
        id: markerEnd
        width: 7
        height: 17
        x: end * timeScale
        antialiasing: true
        ShapePath {
            strokeWidth: 1
            strokeColor: 'transparent'
            fillColor: root.markerColor
            startX: 0
            startY: 0
            PathLine { x: 0; y: 17 }
            PathLine { x: 7; y: 10 }
            PathLine { x: 7; y: 0 }
            PathLine { x: 0; y: 0 }
        }

        MouseArea {
            id: endMouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            property int lockWidth: 0
            property var dragStartX: 0
            drag {
                target: pressedButtons & Qt.LeftButton ? parent : undefined
                axis: Drag.XAxis
                threshold: 0
                minimumX: endMouseArea.lockWidth == -1 ? markerStart.x + 7 : 0
                maximumX: root.width
            }
            onPressed: {
               dragStartX = markerEnd.x
               if (mouse.modifiers & Qt.ControlModifier)
                   lockWidth = -1
               else
                   lockWidth = markerEnd.x - markerStart.x
            }
            onPositionChanged: {
                if (lockWidth != -1) {
                    markerStart.x = markerEnd.x - lockWidth
                }
            }
            onReleased: {
                if (mouse.button == Qt.LeftButton && markerEnd.x != dragStartX) {
                    markers.move(index, Math.round((markerStart.x + 7) / timeScale), Math.round(markerEnd.x / timeScale))
                }
            }
            onClicked: {
                if (mouse.button === Qt.RightButton) {
                    menu.popup()
                } else {
                    timeline.position = end
                }
            }
        }
    }

    Rectangle {
        id: markerLink
        anchors.left: markerStart.right
        anchors.right: markerEnd.left
        height: 7
        color: root.markerColor

        MouseArea {
            id: linkMouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            property var dragStartX
            property int startDragStartX
            property int endDragStartX
            drag {
                target: pressedButtons & Qt.LeftButton ? parent : undefined
                axis: Drag.XAxis
                threshold: 0
                minimumX: 0
                maximumX: root.width
            }
            onPressed: {
                markerLink.anchors.left = undefined
                markerLink.anchors.right = undefined
                dragStartX = markerLink.x
                startDragStartX = markerStart.x
                endDragStartX = markerEnd.x
            }
            onPositionChanged: {
                var delta = dragStartX - markerLink.x
                markerStart.x = startDragStartX - delta
                markerEnd.x = endDragStartX - delta
            }
            onReleased: {
                markerLink.anchors.left = markerStart.right
                markerLink.anchors.right = markerEnd.left
                if (mouse.button == Qt.LeftButton && dragStartX != markerLink.x) {
                    markers.move(index, (markerStart.x + 7) / timeScale, markerEnd.x / timeScale)
                }
            }
            onClicked: {
                if (mouse.button === Qt.RightButton) {
                    menu.popup()
                } else {
                    timeline.position = mouse.x/timeScale < (end - start)/2 ? start : end
                }
            }
        }
    }
}
