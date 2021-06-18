/*
 * Copyright (c) 2014-2020 Meltytech, LLC
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
    property color handleColor: Qt.rgba(1, 1, 1, enabled? 0.9 : 0.2)
    property int snapMargin: 10
    property alias withRotation: rotationHandle.visible
    property alias rotation: rotationGroup.rotation
    property bool _positionDragLocked: false
    property bool _positionDragEnabled: false

    signal rectChanged(Rectangle rect)
    signal rotated(real degrees, var mouse)
    signal rotationReleased()

    Component.onCompleted: {
        _positionDragLocked = filter.get('_shotcut:positionDragLocked') === '1'
    }

    function setHandles(rect) {
        if ( rect.width < 0 || rect.height < 0)
            return
        topLeftHandle.x = (rect.x * widthScale)
        topLeftHandle.y = (rect.y * heightScale)
        if (aspectRatio === 0.0) {
            bottomRightHandle.x = topLeftHandle.x + (rect.width * widthScale) - handleSize
            bottomRightHandle.y = topLeftHandle.y + (rect.height * heightScale) - handleSize
        } else if (aspectRatio > 1.0) {
            bottomRightHandle.x = topLeftHandle.x + (rect.width * widthScale) - handleSize
            bottomRightHandle.y = topLeftHandle.y + (rect.width * widthScale / aspectRatio) - handleSize
        } else {
            bottomRightHandle.x = topLeftHandle.x + (rect.height * heightScale * aspectRatio) - handleSize
            bottomRightHandle.y = topLeftHandle.y + (rect.height * heightScale) - handleSize
        }
        topRightHandle.x = bottomRightHandle.x
        topRightHandle.y = topLeftHandle.y
        bottomLeftHandle.x = topLeftHandle.x
        bottomLeftHandle.y = bottomRightHandle.y
    }

    function snapGrid(v, gridSize) {
        var polarity = (v < 0) ? -1 : 1
        v = v * polarity
        var delta = v % gridSize
        if (delta < snapMargin) {
            v = v - delta
        } else if ((gridSize - delta) < snapMargin) {
            v = v + gridSize - delta
        }
        return v * polarity
    }

    function snapX(x) {
        if (!video.snapToGrid || video.grid === 0) {
            return x
        }
        if (video.grid !== 95 && video.grid !== 8090) {
            var n = (video.grid > 10000) ? video.grid - 10000 : parent.width / video.grid
            return snapGrid(x, n)
        } else {
            var deltas = null
            if (video.grid === 8090) {
                // 80/90% Safe Areas
                deltas = [0.0, 0.05, 0.1, 0.9, 0.95, 1.0]
            } else if (video.grid === 95) {
                // EBU R95 Safe Areas
                deltas = [0.0, 0.035, 0.05, 0.95, 0.965, 1.0]
            }
            if (deltas) {
                for (var i = 0; i < deltas.length; i++) {
                    var delta = x - deltas[i] * parent.width
                    if (Math.abs(delta) < snapMargin)
                        return x - delta
                }
            }
        }
        return x
    }

    function snapY(y) {
        if (!video.snapToGrid || video.grid === 0) {
            return y
        }
        if (video.grid !== 95 && video.grid !== 8090) {
            var n = (video.grid > 10000) ? video.grid - 10000 : parent.height / video.grid
            return snapGrid(y, n)
        } else {
            var deltas = null
            if (video.grid === 8090) {
                // 80/90% Safe Areas
                deltas = [0.0, 0.05, 0.1, 0.9, 0.95, 1.0]
            } else if (video.grid === 95) {
                // EBU R95 Safe Areas
                deltas = [0.0, 0.035, 0.05, 0.95, 0.965, 1.0]
            }
            if (deltas) {
                for (var i = 0; i < deltas.length; i++) {
                    var delta = y - deltas[i] * parent.height
                    if (Math.abs(delta) < snapMargin)
                        return y - delta
                }
            }
        }
        return y
    }

    function isRotated() {
        //Math.abs(rotationGroup.rotation - 0) > 0.0001
        return rotationGroup.rotation !== 0 || rotationLine.rotation !== 0
    }

    Rectangle {
        id: rectangle
        visible: !isRotated()
        color: 'transparent'
        border.width: borderSize
        border.color: handleColor
        anchors.top: topLeftHandle.top
        anchors.left: topLeftHandle.left
        anchors.right: bottomRightHandle.right
        anchors.bottom: bottomRightHandle.bottom
        focus: true
        Keys.onPressed: {
            if (event.key === Qt.Key_Shift) {
                _positionDragEnabled = true
            }
        }
        Keys.onReleased: {
            if (event.key === Qt.Key_Shift) {
                _positionDragEnabled = false
            }
        }
    }
    Rectangle {
        // Provides contrasting thick line to above rectangle.
        visible: !isRotated()
        color: 'transparent'
        border.width: handleSize - borderSize
        border.color: Qt.rgba(0, 0, 0, item.enabled? 0.4 : 0.2)
        anchors.fill: rectangle
        anchors.margins: borderSize
    }

    Item {
        id: rotationGroup
        anchors.fill: rectangle

        Rectangle {
            id: positionHandle
            opacity: item.enabled? 0.5 : 0.2
            border.width: borderSize
            border.color: handleColor
            width: handleSize * 2
            height: handleSize * 2
            radius: width / 2
            anchors.centerIn: parent
            z: 1
            gradient: Gradient {
                GradientStop {
                    position: (_positionDragLocked || _positionDragEnabled || positionMouseArea.pressed)? 0.0 : 1.0
                    color: 'black'
                }
                GradientStop {
                    position: (_positionDragLocked || _positionDragEnabled || positionMouseArea.pressed)? 1.0 : 0.0
                    color: 'white'
                }
            }
            function centerX() { return x + width / 2 }
        }

        MouseArea {
            id: positionMouseArea
            anchors.fill: (_positionDragLocked || _positionDragEnabled)? parent : positionHandle
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeAllCursor
            drag.target: rectangle
            onDoubleClicked: {
                _positionDragLocked = !_positionDragLocked
                filter.set('_shotcut:positionDragLocked', _positionDragLocked)
            }
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
            onPositionChanged: {
                rectangle.x = snapX(rectangle.x + rectangle.width / 2) - rectangle.width / 2
                rectangle.y = snapY(rectangle.y + rectangle.height / 2) - rectangle.height / 2
                rectChanged(rectangle)
            }
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

        Rectangle {
            id: rotationHandle
            visible: false
            color: handleColor
            opacity: item.enabled? 0.5 : 0.2
            width: handleSize * 1.5
            height: handleSize * 1.5
            radius: width / 2
            z: 1
            anchors.centerIn: rotationGroup
            anchors.verticalCenterOffset: -item.height / 4
            border.width: borderSize
            border.color: Qt.rgba(0, 0, 0, enabled? 0.9 : 0.2)
            function centerX() { return x + width / 2 }
            MouseArea {
                id: rotationMouseArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                cursorShape: pressed? Qt.ClosedHandCursor : Qt.OpenHandCursor
                drag.target: parent
                property real startRotation: 0
                function getRotationDegrees() {
                    var radians = Math.atan2(rotationHandle.centerX() - positionHandle.centerX(), positionHandle.y - rotationHandle.y)
                    if (radians < 0)
                        radians += 2 * Math.PI
                    return 180 / Math.PI * radians
                }

                onPressed: {
                    parent.anchors.centerIn = undefined
                    startRotation = rotationGroup.rotation
                }
                onPositionChanged: {
                    var degrees = getRotationDegrees()
                    rotated(startRotation + degrees, mouse)
                    rotationLine.rotation = degrees
                }
                onReleased: {
                    rotationLine.rotation = 0
                    rotationGroup.rotation = startRotation + getRotationDegrees()
                    parent.anchors.centerIn = rotationGroup
                    rotationReleased()
                }
            }
        }
        Rectangle {
            id: rotationLine
            height: -rotationHandle.anchors.verticalCenterOffset - rotationHandle.height + positionHandle.height / 2
            anchors.horizontalCenter: positionHandle.horizontalCenter
            anchors.bottom: positionHandle.verticalCenter
            transformOrigin: Item.Bottom
            width: 2
            color: handleColor
            visible: rotationHandle.visible
            antialiasing: true
        }
    }

    Rectangle {
        id: topLeftHandle
        visible: !isRotated()
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
                topLeftHandle.x = snapX(topLeftHandle.x)
                topLeftHandle.y = snapY(topLeftHandle.y)
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
        visible: !isRotated()
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
                topRightHandle.x = snapX(topRightHandle.x + handleSize) - handleSize
                topRightHandle.y = snapY(topRightHandle.y)
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
        visible: !isRotated()
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
                bottomLeftHandle.x = snapX(bottomLeftHandle.x)
                bottomLeftHandle.y = snapY(bottomLeftHandle.y + handleSize) - handleSize
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
        visible: !isRotated()
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
                bottomRightHandle.x = snapX(bottomRightHandle.x + handleSize) - handleSize
                bottomRightHandle.y = snapY(bottomRightHandle.y + handleSize) - handleSize
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
