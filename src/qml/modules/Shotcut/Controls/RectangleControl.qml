/*
 * Copyright (c) 2014 Meltytech, LLC
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

import QtQuick 2.1

Item {
    id: item
    anchors.fill: parent

    property real widthScale: 1.0
    property real heightScale: 1.0
    property real aspectRatio: 0.0
    property int handleSize: 10
    property int borderSize: 2
    property alias rectangle: rectangle
    property color handleColor: Qt.rgba(1, 1, 1, 0.9)

    signal rectChanged(Rectangle rect)

    function setHandles(rect) {
        topLeftHandle.x = Math.round(rect.x * widthScale)
        topLeftHandle.y = Math.round(rect.y * heightScale)
        if (aspectRatio === 0.0) {
            bottomRightHandle.x = topLeftHandle.x + Math.round(rect.width * widthScale) - handleSize
            bottomRightHandle.y = topLeftHandle.y + Math.round(rect.height * heightScale) - handleSize
        } else if (aspectRatio > 1.0) {
            bottomRightHandle.x = topLeftHandle.x + Math.round(rect.width * widthScale) - handleSize
            bottomRightHandle.y = topLeftHandle.y + Math.round(rect.width * widthScale / aspectRatio) - handleSize
        } else {
            bottomRightHandle.x = topLeftHandle.x + Math.round(rect.height * heightScale * aspectRatio) - handleSize
            bottomRightHandle.y = topLeftHandle.y + Math.round(rect.height * heightScale) - handleSize
        }
        topRightHandle.x = bottomRightHandle.x
        topRightHandle.y = topLeftHandle.y
        bottomLeftHandle.x = topLeftHandle.x
        bottomLeftHandle.y = bottomRightHandle.y
    }

    Rectangle {
        id: rectangle
        color: 'transparent'
        border.width: borderSize
        border.color: handleColor
        anchors.top: topLeftHandle.top
        anchors.left: topLeftHandle.left
        anchors.right: bottomRightHandle.right
        anchors.bottom: bottomRightHandle.bottom
    }
    Rectangle {
        // Provides contrasting thick line to above rectangle.
        color: 'transparent'
        border.width: handleSize - borderSize
        border.color: Qt.rgba(0, 0, 0, 0.4)
        anchors.fill: rectangle
        anchors.margins: borderSize
    }

    Rectangle {
        id: positionHandle
        color: Qt.rgba(0, 0, 0, 0.5)
        border.width: borderSize
        border.color: handleColor
        width: handleSize * 2
        height: handleSize * 2
        radius: width / 2
        anchors.centerIn: rectangle
        MouseArea {
            id: positionMouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeAllCursor
            drag.target: rectangle
            onEntered: {
                rectangle.anchors.top = undefined
                rectangle.anchors.left = undefined
                rectangle.anchors.right = undefined
                rectangle.anchors.bottom = undefined
                topLeftHandle.anchors.left = rectangle.left
                topLeftHandle.anchors.top = rectangle.top
                topRightHandle.anchors.right = rectangle.right
                topRightHandle.anchors.top = rectangle.top
                bottomLeftHandle.anchors.left = rectangle.left
                bottomLeftHandle.anchors.bottom = rectangle.bottom
                bottomRightHandle.anchors.right = rectangle.right
                bottomRightHandle.anchors.bottom = rectangle.bottom
            }
            onPositionChanged: rectChanged(rectangle)
            onReleased: {
                rectChanged(rectangle)
                rectangle.anchors.top = topLeftHandle.top
                rectangle.anchors.left = topLeftHandle.left
                rectangle.anchors.right = bottomRightHandle.right
                rectangle.anchors.bottom = bottomRightHandle.bottom
                topLeftHandle.anchors.left = undefined
                topLeftHandle.anchors.top = undefined
                topRightHandle.anchors.right = undefined
                topRightHandle.anchors.top = undefined
                bottomLeftHandle.anchors.left = undefined
                bottomLeftHandle.anchors.bottom = undefined
                bottomRightHandle.anchors.right = undefined
                bottomRightHandle.anchors.bottom = undefined
            }
        }
    }
    Rectangle {
        id: topLeftHandle
        color: handleColor
        width: handleSize
        height: handleSize
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeFDiagCursor
            drag.target: parent
            onEntered: {
                rectangle.anchors.top = parent.top
                rectangle.anchors.left = parent.left
                topRightHandle.anchors.top = rectangle.top
                bottomLeftHandle.anchors.left = rectangle.left
            }
            onPositionChanged: {
                if (aspectRatio !== 0.0)
                    parent.x = topRightHandle.x + handleSize - rectangle.height * aspectRatio
                parent.x = Math.min(parent.x, bottomRightHandle.x)
                parent.y = Math.min(parent.y, bottomRightHandle.y)
                rectChanged(rectangle)
            }
            onReleased: {
                rectChanged(rectangle)
                topRightHandle.anchors.top = undefined
                bottomLeftHandle.anchors.left = undefined
            }
        }
    }

    Rectangle {
        id: topRightHandle
        color: handleColor
        width: handleSize
        height: handleSize
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeBDiagCursor
            drag.target: parent
            onEntered: {
                rectangle.anchors.top = parent.top
                rectangle.anchors.right = parent.right
                rectangle.anchors.bottom = bottomLeftHandle.bottom
                rectangle.anchors.left = bottomLeftHandle.left
                topLeftHandle.anchors.top = rectangle.top
                bottomRightHandle.anchors.right = rectangle.right
            }
            onPositionChanged: {
                if (aspectRatio !== 0.0)
                    parent.x = topLeftHandle.x + rectangle.height * aspectRatio - handleSize
                parent.x = Math.max(parent.x, bottomLeftHandle.x)
                parent.y = Math.min(parent.y, bottomLeftHandle.y)
                rectChanged(rectangle)
            }
            onReleased: {
                rectChanged(rectangle)
                topLeftHandle.anchors.top = undefined
                bottomRightHandle.anchors.right = undefined
            }
        }
    }

    Rectangle {
        id: bottomLeftHandle
        color: handleColor
        width: handleSize
        height: handleSize
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeBDiagCursor
            drag.target: parent
            onEntered: {
                rectangle.anchors.bottom = parent.bottom
                rectangle.anchors.left = parent.left
                rectangle.anchors.top = topRightHandle.top
                rectangle.anchors.right = topRightHandle.right
                topLeftHandle.anchors.left = rectangle.left
                bottomRightHandle.anchors.bottom = rectangle.bottom
            }
            onPositionChanged: {
                if (aspectRatio !== 0.0)
                    parent.x = topRightHandle.x + handleSize - rectangle.height * aspectRatio
                parent.x = Math.min(parent.x, topRightHandle.x)
                parent.y = Math.max(parent.y, topRightHandle.y)
                rectChanged(rectangle)
            }
            onReleased: {
                rectChanged(rectangle)
                topLeftHandle.anchors.left = undefined
                bottomRightHandle.anchors.bottom = undefined
            }
        }
    }

    Rectangle {
        id: bottomRightHandle
        color: handleColor
        width: handleSize
        height: handleSize
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeFDiagCursor
            drag.target: parent
            onEntered: {
                rectangle.anchors.bottom = parent.bottom
                rectangle.anchors.right = parent.right
                topRightHandle.anchors.right = rectangle.right
                bottomLeftHandle.anchors.bottom = rectangle.bottom
            }
            onPositionChanged: {
                if (aspectRatio !== 0.0)
                    parent.x = topLeftHandle.x + rectangle.height * aspectRatio - handleSize
                parent.x = Math.max(parent.x, topLeftHandle.x)
                parent.y = Math.max(parent.y, topLeftHandle.y)
                rectChanged(rectangle)
            }
            onReleased: {
                rectChanged(rectangle)
                topRightHandle.anchors.right = undefined
                bottomLeftHandle.anchors.bottom = undefined
            }
        }
    }
}
