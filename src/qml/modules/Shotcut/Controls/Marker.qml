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
import QtQuick.Dialogs 1.3
import Shotcut.Controls 1.0 as Shotcut

Item {
    id: root
    property real timeScale: 1.0
    property var snapper
    property var start: 0
    property var end: 0
    property var markerColor: 'black'
    property var text: ""
    property var index: 0
    signal editRequested(int index)
    signal deleteRequested(int index)
    signal exited()
    signal mouseStatusChanged(int mouseX, int mouseY, var text, int start, int end)
    signal seekRequested(int pos)
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

    ColorDialog {
        id: colorDialog
        title: qsTr("Marker Color")
        showAlphaChannel: false
        color: root.markerColor
        onAccepted: {
            markers.setColor(root.index, color)
        }
        modality: application.dialogModality
    }

    Menu {
        id: menu
        title: qsTr('Marker Operations')
        MenuItem {
            text: qsTr('Edit...') + (application.OS === 'OS X'? '    M' : ' (M)')
            onTriggered: {
                menu.dismiss()
                root.editRequested(root.index)
            }
        }
        MenuItem {
            text: qsTr('Delete') + (application.OS === 'OS X'? '    ⇧⌘M' : ' (Ctrl+Shift+M)')
            onTriggered: root.deleteRequested(root.index)
        }
        MenuItem {
            text: qsTr('Choose Color...')
            onTriggered: {
                colorDialog.color = root.markerColor
                colorDialog.open()
            }
        }
        Menu {
            id: colorMenu
            width: 100
            title: qsTr('Choose Recent Color')
            Instantiator {
                model: markers.recentColors
                MenuItem {
                    id: menuItem
                    background: Rectangle {
                        color: modelData
                        border.width: 3
                        border.color: menuItem.highlighted ? activePalette.highlight : modelData
                    }
                    contentItem: Text {
                        text: modelData
                        horizontalAlignment: Text.AlignHCenter
                        color: application.contrastingColor(modelData)
                    }
                    onTriggered: markers.setColor(root.index, modelData)
                }
                onObjectAdded: colorMenu.insertItem(index, object)
                onObjectRemoved: colorMenu.removeItem(object)
            }
        }
        MenuSeparator { }
        MenuItem {
            text: qsTr('Cancel')
            onTriggered: menu.dismiss()
        }
    }

    Shotcut.MarkerStart {
        id: markerStart
        width: 7
        height: 17
        x: (start * timeScale) - 7
        fillColor: root.markerColor

        MouseArea {
            id: startMouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            property int lockWidth: 0
            property var dragStartX: 0
            property bool dragInProgress: false
            drag {
                target: pressedButtons & Qt.LeftButton ? parent : undefined
                axis: Drag.XAxis
                threshold: 0
                minimumX: -7
                maximumX: startMouseArea.lockWidth == -1 ? markerEnd.x - 7 : root.width
            }
            onPressed: {
                if (mouse.button === Qt.LeftButton) {
                    dragInProgress = true
                    dragStartX = markerStart.x
                    if (mouse.modifiers & Qt.ControlModifier) {
                        lockWidth = -1
                    } else {
                        lockWidth = markerEnd.x - markerStart.x
                    }
                }
            }
            onPositionChanged: {
                if (dragInProgress) {
                    if (typeof snapper !== 'undefined') {
                        markerStart.x = snapper.getSnapPosition(markerStart.x + width) - width
                    }
                    if (lockWidth != -1) {
                        markerEnd.x = markerStart.x + lockWidth
                    }
                }
                mouseStatusChanged(mouse.x + markerStart.x, mouse.y, text, (markerStart.x + 7) / timeScale, markerEnd.x / timeScale)
            }
            onReleased: {
                dragInProgress = false
                if (mouse.button == Qt.LeftButton && markerStart.x != dragStartX) {
                    markers.move(index, Math.round((markerStart.x + 7) / timeScale), Math.round(markerEnd.x / timeScale))
                }
            }
            onClicked: {
                if (mouse.button === Qt.RightButton) {
                    menu.popup()
                } else {
                    root.seekRequested(start)
                }
            }
            onExited: root.exited()
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
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            property int lockWidth: 0
            property var dragStartX: 0
            property bool dragInProgress: false
            drag {
                target: pressedButtons & Qt.LeftButton ? parent : undefined
                axis: Drag.XAxis
                threshold: 0
                minimumX: endMouseArea.lockWidth == -1 ? markerStart.x + 7 : 0
                maximumX: root.width
            }
            onPressed: {
                if (mouse.button === Qt.LeftButton) {
                    dragInProgress = true
                    dragStartX = markerEnd.x
                    if (mouse.modifiers & Qt.ControlModifier)
                        lockWidth = -1
                    else
                        lockWidth = markerEnd.x - markerStart.x
                }
            }
            onPositionChanged: {
                if (dragInProgress) {
                    if (typeof snapper !== 'undefined') {
                        markerEnd.x = snapper.getSnapPosition(markerEnd.x)
                    }
                    if (lockWidth != -1) {
                        markerStart.x = markerEnd.x - lockWidth
                    }
                }
                mouseStatusChanged(mouse.x + markerEnd.x, mouse.y, text, (markerStart.x + 7) / timeScale, markerEnd.x / timeScale)
            }
            onReleased: {
                dragInProgress = false
                if (mouse.button == Qt.LeftButton && markerEnd.x != dragStartX) {
                    markers.move(index, Math.round((markerStart.x + 7) / timeScale), Math.round(markerEnd.x / timeScale))
                }
            }
            onClicked: {
                if (mouse.button === Qt.RightButton) {
                    menu.popup()
                } else {
                    root.seekRequested(end)
                }
            }
            onExited: root.exited()
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
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            property var dragStartX
            property int startDragStartX
            property int endDragStartX
            property bool dragInProgress: false
            drag {
                target: pressedButtons & Qt.LeftButton ? parent : undefined
                axis: Drag.XAxis
                threshold: 0
                minimumX: 0
                maximumX: root.width
            }
            onPressed: {
                if (mouse.button === Qt.LeftButton) {
                    dragInProgress = true
                    markerLink.anchors.left = undefined
                    markerLink.anchors.right = undefined
                    dragStartX = markerLink.x
                    startDragStartX = markerStart.x
                    endDragStartX = markerEnd.x
                }
            }
            onPositionChanged: {
                if (dragInProgress) {
                    var delta = dragStartX - markerLink.x
                    if (typeof snapper !== 'undefined') {
                        var snapDelta = startDragStartX - (snapper.getSnapPosition(startDragStartX + 7 - delta) - 7)
                        if (snapDelta == delta) {
                            snapDelta = endDragStartX - snapper.getSnapPosition(endDragStartX - delta)
                        }
                        delta = snapDelta
                    }
                    markerStart.x = startDragStartX - delta
                    markerEnd.x = endDragStartX - delta
                    markerLink.x = dragStartX - delta
                }
                mouseStatusChanged(mouse.x + markerLink.x, mouse.y, text, (markerStart.x + 7) / timeScale, markerEnd.x / timeScale)
            }
            onReleased: {
                dragInProgress = false
                if (mouse.button === Qt.LeftButton) {
                    markerLink.anchors.left = markerStart.right
                    markerLink.anchors.right = markerEnd.left
                    if (dragStartX != markerLink.x) {
                        markers.move(index, (markerStart.x + 7) / timeScale, markerEnd.x / timeScale)
                    }
                }
            }
            onClicked: {
                if (mouse.button === Qt.RightButton) {
                    menu.popup()
                } else {
                    root.seekRequested(mouse.x/timeScale < (end - start)/2 ? start : end)
                }
            }
            onExited: root.exited()
        }
    }
}
