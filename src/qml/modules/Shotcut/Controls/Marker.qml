/*
 * Copyright (c) 2021-2022 Meltytech, LLC
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
import QtQuick.Dialogs
import Shotcut.Controls as Shotcut

Item {
    id: root

    property real timeScale: 1
    property var snapper
    property var start: 0
    property var end: 0
    property var markerColor: 'black'
    property var text: ""
    property var index: 0

    signal exited()
    signal mouseStatusChanged(int mouseX, int mouseY, var text, int start, int end)
    signal seekRequested(int pos)

    function updatePosition() {
        markerStart.x = (start * timeScale) - 7;
        markerEnd.x = end * timeScale;
    }

    x: 0
    width: parent.width
    height: 17
    onTimeScaleChanged: {
        updatePosition();
    }
    onStartChanged: {
        updatePosition();
    }
    onEndChanged: {
        updatePosition();
    }

    SystemPalette {
        id: activePalette
    }

    Shotcut.MarkerMenu {
        id: menu

        target: timeline
        index: root.index
    }

    Shotcut.MarkerStart {
        id: markerStart

        width: 7
        height: 17
        x: (start * timeScale) - 7
        fillColor: root.markerColor

        MouseArea {
            id: startMouseArea

            property int lockWidth: 0
            property var dragStartX: 0
            property bool dragInProgress: false

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true
            cursorShape: pressed ? Qt.SizeHorCursor : Qt.PointingHandCursor
            onPressed: (mouse)=> {
                if (mouse.button === Qt.LeftButton) {
                    dragInProgress = true;
                    dragStartX = markerStart.x;
                    if (mouse.modifiers & Qt.ControlModifier)
                        lockWidth = -1;
                    else
                        lockWidth = markerEnd.x - markerStart.x;
                }
            }
            onPositionChanged: (mouse)=> {
                var newStart = root.start;
                var newEnd = root.end;
                if (dragInProgress) {
                    if (typeof snapper !== 'undefined')
                        markerStart.x = snapper.getSnapPosition(markerStart.x + width) - width;

                    newStart = Math.round((markerStart.x + 7) / timeScale);
                    if (lockWidth != -1) {
                        markerEnd.x = markerStart.x + lockWidth;
                        newEnd += newStart - root.start;
                    }
                }
                mouseStatusChanged(mouse.x + markerStart.x, mouse.y, text, newStart, newEnd);
            }
            onReleased: (mouse)=> {
                dragInProgress = false;
                if (mouse.button == Qt.LeftButton && markerStart.x != dragStartX) {
                    var newStart = Math.round((markerStart.x + 7) / timeScale);
                    var newEnd = root.end;
                    if (lockWidth != -1)
                        newEnd += newStart - root.start;

                    markers.move(index, newStart, newEnd);
                }
            }
            onClicked: (mouse)=> {
                if (mouse.button === Qt.RightButton)
                    menu.popup();
                else
                    root.seekRequested(start);
            }
            onExited: root.exited()

            drag {
                target: pressedButtons & Qt.LeftButton ? parent : undefined
                axis: Drag.XAxis
                threshold: 0
                minimumX: -7
                maximumX: startMouseArea.lockWidth == -1 ? markerEnd.x - 7 : root.width
            }

        }

    }

    Shotcut.MarkerEnd {
        id: markerEnd

        width: 7
        height: 17
        x: end * timeScale
        fillColor: root.markerColor

        MouseArea {
            id: endMouseArea

            property int lockWidth: 0
            property var dragStartX: 0
            property bool dragInProgress: false

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true
            cursorShape: pressed ? Qt.SizeHorCursor : Qt.PointingHandCursor
            onPressed: (mouse)=> {
                if (mouse.button === Qt.LeftButton) {
                    dragInProgress = true;
                    dragStartX = markerEnd.x;
                    if (mouse.modifiers & Qt.ControlModifier)
                        lockWidth = -1;
                    else
                        lockWidth = markerEnd.x - markerStart.x;
                }
            }
            onPositionChanged: (mouse)=> {
                var newStart = root.start;
                var newEnd = root.end;
                if (dragInProgress) {
                    if (typeof snapper !== 'undefined')
                        markerEnd.x = snapper.getSnapPosition(markerEnd.x);

                    newEnd = Math.round(markerEnd.x / timeScale);
                    if (lockWidth != -1) {
                        markerStart.x = markerEnd.x - lockWidth;
                        newStart += newEnd - root.end;
                    }
                }
                mouseStatusChanged(mouse.x + markerEnd.x, mouse.y, text, newStart, newEnd);
            }
            onReleased: (mouse)=> {
                dragInProgress = false;
                if (mouse.button == Qt.LeftButton && markerEnd.x != dragStartX) {
                    var newEnd = Math.round(markerEnd.x / timeScale);
                    var newStart = root.start;
                    if (lockWidth != -1)
                        newStart += newEnd - root.end;

                    markers.move(index, newStart, newEnd);
                }
            }
            onClicked: (mouse)=> {
                if (mouse.button === Qt.RightButton)
                    menu.popup();
                else
                    root.seekRequested(end);
            }
            onExited: root.exited()

            drag {
                target: pressedButtons & Qt.LeftButton ? parent : undefined
                axis: Drag.XAxis
                threshold: 0
                minimumX: endMouseArea.lockWidth == -1 ? markerStart.x + 7 : endMouseArea.lockWidth - 7
                maximumX: root.width
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

            property var dragStartX
            property int startDragStartX
            property int endDragStartX
            property bool dragInProgress: false

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true
            cursorShape: pressed ? Qt.SizeHorCursor : Qt.PointingHandCursor
            onPressed: (mouse)=> {
                if (mouse.button === Qt.LeftButton) {
                    dragInProgress = true;
                    markerLink.anchors.left = undefined;
                    markerLink.anchors.right = undefined;
                    dragStartX = markerLink.x;
                    startDragStartX = markerStart.x;
                    endDragStartX = markerEnd.x;
                }
            }
            onPositionChanged: (mouse)=> {
                var newStart = root.start;
                var newEnd = root.end;
                if (dragInProgress) {
                    var delta = dragStartX - markerLink.x;
                    if (typeof snapper !== 'undefined') {
                        var snapDelta = startDragStartX - (snapper.getSnapPosition(startDragStartX + 7 - delta) - 7);
                        if (snapDelta == delta)
                            snapDelta = endDragStartX - snapper.getSnapPosition(endDragStartX - delta);

                        delta = snapDelta;
                    }
                    markerStart.x = startDragStartX - delta;
                    markerEnd.x = endDragStartX - delta;
                    markerLink.x = dragStartX - delta;
                    newStart = Math.round((markerStart.x + 7) / timeScale);
                    newEnd += newStart - root.start;
                }
                mouseStatusChanged(mouse.x + markerLink.x, mouse.y, text, newStart, newEnd);
            }
            onReleased: (mouse)=> {
                dragInProgress = false;
                if (mouse.button === Qt.LeftButton) {
                    markerLink.anchors.left = markerStart.right;
                    markerLink.anchors.right = markerEnd.left;
                    if (dragStartX != markerLink.x) {
                        var newStart = Math.round((markerStart.x + 7) / timeScale);
                        markers.move(index, newStart, root.end + newStart - root.start);
                    }
                }
            }
            onClicked: (mouse)=> {
                if (mouse.button === Qt.RightButton)
                    menu.popup();
                else
                    root.seekRequested(mouse.x / timeScale < (end - start) / 2 ? start : end);
            }
            onExited: root.exited()

            drag {
                target: pressedButtons & Qt.LeftButton ? parent : undefined
                axis: Drag.XAxis
                threshold: 0
                minimumX: 0
                maximumX: root.width
            }

        }

    }

}
