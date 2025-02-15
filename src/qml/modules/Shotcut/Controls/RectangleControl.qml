/*
 * Copyright (c) 2014-2022 Meltytech, LLC
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

Item {
    // 80/90% Safe Areas
    // EBU R95 Safe Areas
    // 80/90% Safe Areas
    // EBU R95 Safe Areas
    id: item

    property real widthScale: 1
    property real heightScale: 1
    property real aspectRatio: 0
    property int handleSize: 10
    property int borderSize: 2
    property bool isReframe: false
    property alias rectangle: rectangle
    property color handleColor: Qt.rgba(1, 1, 1, enabled ? 0.9 : 0.2)
    property int snapMargin: 10
    property alias withRotation: rotationHandle.visible
    property alias rotation: rotationGroup.rotation
    property bool _positionDragLocked: false
    property bool _positionDragEnabled: false

    signal rectChanged(Rectangle rect)
    signal rotated(real degrees, var mouse)
    signal rotationReleased

    function setHandles(rect) {
        if (rect.width < 0 || rect.height < 0)
            return;
        topLeftHandle.x = (rect.x * widthScale);
        topLeftHandle.y = (rect.y * heightScale);
        if (aspectRatio === 0) {
            bottomRightHandle.x = topLeftHandle.x + (rect.width * widthScale) - handleSize;
            bottomRightHandle.y = topLeftHandle.y + (rect.height * heightScale) - handleSize;
        } else if (aspectRatio > 1) {
            bottomRightHandle.x = topLeftHandle.x + (rect.width * widthScale) - handleSize;
            bottomRightHandle.y = topLeftHandle.y + (rect.width * widthScale / aspectRatio) - handleSize;
        } else {
            bottomRightHandle.x = topLeftHandle.x + (rect.height * heightScale * aspectRatio) - handleSize;
            bottomRightHandle.y = topLeftHandle.y + (rect.height * heightScale) - handleSize;
        }
        topRightHandle.x = bottomRightHandle.x;
        topRightHandle.y = topLeftHandle.y;
        bottomLeftHandle.x = topLeftHandle.x;
        bottomLeftHandle.y = bottomRightHandle.y;
        topHandle.x = topLeftHandle.x + (rect.width * widthScale) / 2 - (handleSize / 2);
        topHandle.y = topLeftHandle.y;
        bottomHandle.x = topLeftHandle.x + (rect.width * widthScale) / 2 - (handleSize / 2);
        bottomHandle.y = bottomRightHandle.y;
        leftHandle.x = topLeftHandle.x;
        leftHandle.y = topLeftHandle.y + (rect.height * heightScale) / 2 - (handleSize / 2);
        rightHandle.x = bottomRightHandle.x;
        rightHandle.y = topLeftHandle.y + (rect.height * heightScale) / 2 - (handleSize / 2);
    }

    function snapGrid(v, gridSize) {
        var polarity = (v < 0) ? -1 : 1;
        v = v * polarity;
        var delta = v % gridSize;
        if (delta < snapMargin)
            v = v - delta;
        else if ((gridSize - delta) < snapMargin)
            v = v + gridSize - delta;
        return v * polarity;
    }

    function snapX(x) {
        if (isReframe && (x < 0 || x > profile.width * widthScale)) {
            return Math.min(Math.max(x, 0), profile.width * widthScale);
        }
        if (!video.snapToGrid || video.grid === 0)
            return x;
        if (video.grid !== 95 && video.grid !== 8090) {
            var n = (video.grid > 10000) ? parent.width / (profile.width / (video.grid - 10000)) : parent.width / video.grid;
            return snapGrid(x, n);
        } else {
            var deltas = null;
            if (video.grid === 8090)
                deltas = [0, 0.05, 0.1, 0.9, 0.95, 1];
            else if (video.grid === 95)
                deltas = [0, 0.035, 0.05, 0.95, 0.965, 1];
            if (deltas) {
                for (var i = 0; i < deltas.length; i++) {
                    var delta = x - deltas[i] * parent.width;
                    if (Math.abs(delta) < snapMargin)
                        return x - delta;
                }
            }
        }
        return x;
    }

    function snapY(y) {
        if (isReframe && (y < 0 || y > profile.height * widthScale)) {
            return Math.min(Math.max(y, 0), profile.height * widthScale);
        }
        if (!video.snapToGrid || video.grid === 0)
            return y;
        if (video.grid !== 95 && video.grid !== 8090) {
            var n = (video.grid > 10000) ? parent.height / (profile.height / (video.grid - 10000)) : parent.height / video.grid;
            return snapGrid(y, n);
        } else {
            var deltas = null;
            if (video.grid === 8090)
                deltas = [0, 0.05, 0.1, 0.9, 0.95, 1];
            else if (video.grid === 95)
                deltas = [0, 0.035, 0.05, 0.95, 0.965, 1];
            if (deltas) {
                for (var i = 0; i < deltas.length; i++) {
                    var delta = y - deltas[i] * parent.height;
                    if (Math.abs(delta) < snapMargin)
                        return y - delta;
                }
            }
        }
        return y;
    }

    function coerceReframeEven(handle) {
        if (isReframe) {
            handle.x += Math.round(rectangle.width / widthScale) % 2;
            handle.y += Math.round(rectangle.height / heightScale) % 2;
        }
    }

    function isRotated() {
        //Math.abs(rotationGroup.rotation - 0) > 0.0001
        return rotationGroup.rotation !== 0 || rotationLine.rotation !== 0;
    }

    anchors.fill: parent
    Component.onCompleted: {
        _positionDragLocked = filter.get('_shotcut:positionDragLocked') === '1';
    }

    Rectangle {
        // Provides contrasting outline
        visible: !isRotated()
        color: 'transparent'
        border.width: borderSize * 2
        border.color: Qt.rgba(0, 0, 0, item.enabled ? 0.4 : 0.2)
        anchors.fill: rectangle
        anchors.margins: -borderSize
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
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Shift)
                _positionDragEnabled = true;
        }
        Keys.onReleased: event => {
            if (event.key === Qt.Key_Shift)
                _positionDragEnabled = false;
        }
    }

    Rectangle {
        visible: isReframe
        color: Qt.rgba(0, 0, 0, item.enabled ? 0.4 : 0)
        anchors.left: parent.left
        anchors.right: rectangle.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }

    Rectangle {
        visible: isReframe
        color: Qt.rgba(0, 0, 0, item.enabled ? 0.4 : 0)
        anchors.left: rectangle.left
        anchors.right: rectangle.right
        anchors.top: parent.top
        anchors.bottom: rectangle.top
    }

    Rectangle {
        visible: isReframe
        color: Qt.rgba(0, 0, 0, item.enabled ? 0.4 : 0)
        anchors.left: rectangle.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }

    Rectangle {
        visible: isReframe
        color: Qt.rgba(0, 0, 0, item.enabled ? 0.4 : 0)
        anchors.left: rectangle.left
        anchors.right: rectangle.right
        anchors.top: rectangle.bottom
        anchors.bottom: parent.bottom
    }

    Item {
        id: rotationGroup

        anchors.fill: rectangle

        Rectangle {
            id: positionHandle

            function centerX() {
                return x + width / 2;
            }

            opacity: item.enabled ? 0.5 : 0.2
            border.width: borderSize
            border.color: handleColor
            width: handleSize * 2
            height: handleSize * 2
            radius: width / 2
            anchors.centerIn: parent
            z: 1

            gradient: Gradient {
                GradientStop {
                    position: (_positionDragLocked || _positionDragEnabled || positionMouseArea.pressed) ? 0 : 1
                    color: 'black'
                }

                GradientStop {
                    position: (_positionDragLocked || _positionDragEnabled || positionMouseArea.pressed) ? 1 : 0
                    color: 'white'
                }
            }
        }

        MouseArea {
            id: positionMouseArea

            anchors.fill: (_positionDragLocked || _positionDragEnabled) ? parent : positionHandle
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeAllCursor
            drag.target: rectangle
            property var startX;
            property var startY;
            onDoubleClicked: {
                _positionDragLocked = !_positionDragLocked;
                filter.set('_shotcut:positionDragLocked', _positionDragLocked);
            }
            onEntered: {
                rectangle.anchors.top = undefined;
                rectangle.anchors.left = undefined;
                rectangle.anchors.right = undefined;
                rectangle.anchors.bottom = undefined;
                topLeftHandle.anchors.left = rectangle.left;
                topLeftHandle.anchors.top = rectangle.top;
                topRightHandle.anchors.right = rectangle.right;
                topRightHandle.anchors.top = rectangle.top;
                bottomLeftHandle.anchors.left = rectangle.left;
                bottomLeftHandle.anchors.bottom = rectangle.bottom;
                bottomRightHandle.anchors.right = rectangle.right;
                bottomRightHandle.anchors.bottom = rectangle.bottom;
                topHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                topHandle.anchors.top = rectangle.top;
                bottomHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                bottomHandle.anchors.bottom = rectangle.bottom;
                leftHandle.anchors.verticalCenter = rectangle.verticalCenter;
                leftHandle.anchors.left = rectangle.left;
                rightHandle.anchors.verticalCenter = rectangle.verticalCenter;
                rightHandle.anchors.right = rectangle.right;
                startX = rectangle.x;
                startY = rectangle.y;
            }
            onPositionChanged: mouse => {
                var newX;
                var newY;
                if (!(mouse.modifiers & Qt.AltModifier)) {
                    newX = snapX(rectangle.x + rectangle.width / 2) - rectangle.width / 2;
                    newY = snapY(rectangle.y + rectangle.height / 2) - rectangle.height / 2;
                }
                if (isReframe) {
                    newX = Math.min(Math.max(rectangle.x, 0), widthScale * profile.width - rectangle.width);
                    newY = Math.min(Math.max(rectangle.y, 0), heightScale * profile.height - rectangle.height);
                }
                if (mouse.modifiers & Qt.ShiftModifier) {
                    var deltaX = Math.abs(startX - newX);
                    var deltaY = Math.abs(startY - newY);
                    if (deltaX < deltaY) {
                        // Constrain the move to only vertical
                        newX = startX;
                    } else {
                        // Constrain the move to only horizontal
                        newY = startY;
                    }
                }
                rectangle.x = newX;
                rectangle.y = newY;
                rectChanged(rectangle);
            }
            onReleased: {
                rectChanged(rectangle);
                rectangle.anchors.top = topLeftHandle.top;
                rectangle.anchors.left = topLeftHandle.left;
                rectangle.anchors.right = bottomRightHandle.right;
                rectangle.anchors.bottom = bottomRightHandle.bottom;
                topLeftHandle.anchors.left = undefined;
                topLeftHandle.anchors.top = undefined;
                topRightHandle.anchors.right = undefined;
                topRightHandle.anchors.top = undefined;
                bottomLeftHandle.anchors.left = undefined;
                bottomLeftHandle.anchors.bottom = undefined;
                bottomRightHandle.anchors.right = undefined;
                bottomRightHandle.anchors.bottom = undefined;
                topHandle.anchors.horizontalCenter = undefined;
                topHandle.anchors.top = undefined;
                bottomHandle.anchors.horizontalCenter = undefined;
                bottomHandle.anchors.bottom = undefined;
                leftHandle.anchors.verticalCenter = undefined;
                leftHandle.anchors.left = undefined;
                rightHandle.anchors.verticalCenter = undefined;
                rightHandle.anchors.right = undefined;
            }
        }

        Rectangle {
            id: rotationHandle

            function centerX() {
                return x + width / 2;
            }

            visible: false
            color: handleColor
            opacity: item.enabled ? 0.5 : 0.2
            width: handleSize * 1.5
            height: handleSize * 1.5
            radius: width / 2
            z: 1
            anchors.centerIn: rotationGroup
            anchors.verticalCenterOffset: -item.height / 4
            border.width: borderSize
            border.color: Qt.rgba(0, 0, 0, enabled ? 0.9 : 0.2)

            MouseArea {
                id: rotationMouseArea

                property real startRotation: 0

                function getRotationDegrees() {
                    var radians = Math.atan2(rotationHandle.centerX() - positionHandle.centerX(), positionHandle.y - rotationHandle.y);
                    if (radians < 0)
                        radians += 2 * Math.PI;
                    return 180 / Math.PI * radians;
                }

                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                drag.target: parent
                onPressed: {
                    parent.anchors.centerIn = undefined;
                    startRotation = rotationGroup.rotation;
                }
                onPositionChanged: mouse => {
                    var degrees = getRotationDegrees();
                    rotated(startRotation + degrees, mouse);
                    rotationLine.rotation = degrees;
                }
                onReleased: {
                    rotationLine.rotation = 0;
                    rotationGroup.rotation = startRotation + getRotationDegrees();
                    parent.anchors.centerIn = rotationGroup;
                    rotationReleased();
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
                rectangle.anchors.top = parent.top;
                rectangle.anchors.left = parent.left;
                rectangle.anchors.bottom = bottomRightHandle.bottom;
                rectangle.anchors.right = bottomRightHandle.right;
                topRightHandle.anchors.top = rectangle.top;
                bottomLeftHandle.anchors.left = rectangle.left;
                topHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                topHandle.anchors.top = rectangle.top;
                bottomHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                bottomHandle.anchors.bottom = rectangle.bottom;
                leftHandle.anchors.verticalCenter = rectangle.verticalCenter;
                leftHandle.anchors.left = rectangle.left;
                rightHandle.anchors.verticalCenter = rectangle.verticalCenter;
                rightHandle.anchors.right = rectangle.right;
            }
            onPositionChanged: mouse => {
                if (!(mouse.modifiers & Qt.AltModifier)) {
                    topLeftHandle.x = snapX(topLeftHandle.x);
                    topLeftHandle.y = snapY(topLeftHandle.y);
                }
                if (aspectRatio !== 0)
                    parent.x = topRightHandle.x + handleSize - rectangle.height * aspectRatio;
                if (isReframe) {
                    parent.x += Math.round(rectangle.width / widthScale) % 2;
                    parent.y += Math.round(rectangle.height / heightScale) % 2;
                }
                coerceReframeEven(parent);
                parent.x = Math.min(parent.x, bottomRightHandle.x);
                parent.y = Math.min(parent.y, bottomRightHandle.y);
                rectChanged(rectangle);
            }
            onReleased: {
                rectChanged(rectangle);
                topRightHandle.anchors.top = undefined;
                bottomLeftHandle.anchors.left = undefined;
                topHandle.anchors.horizontalCenter = undefined;
                topHandle.anchors.top = undefined;
                bottomHandle.anchors.horizontalCenter = undefined;
                bottomHandle.anchors.bottom = undefined;
                leftHandle.anchors.verticalCenter = undefined;
                leftHandle.anchors.left = undefined;
                rightHandle.anchors.verticalCenter = undefined;
                rightHandle.anchors.right = undefined;
            }
        }
    }

    Rectangle {
        id: topHandle

        color: handleColor
        width: handleSize
        height: handleSize
        visible: aspectRatio !== 0 ? false : !isRotated()

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeVerCursor
            drag.target: parent
            onEntered: {
                rectangle.anchors.top = parent.top;
                topLeftHandle.anchors.top = rectangle.top;
                topRightHandle.anchors.top = rectangle.top;
                leftHandle.anchors.verticalCenter = rectangle.verticalCenter;
                rightHandle.anchors.verticalCenter = rectangle.verticalCenter;
            }
            onPositionChanged: mouse => {
                topHandle.x = topLeftHandle.x + rectangle.width / 2 - (handleSize / 2);
                if (!(mouse.modifiers & Qt.AltModifier))
                    topHandle.y = snapY(topHandle.y);
                coerceReframeEven(parent);
                parent.x = Math.min(parent.x, bottomHandle.x);
                parent.y = Math.min(parent.y, bottomHandle.y);
                rectChanged(rectangle);
            }
            onReleased: {
                rectChanged(rectangle);
                topLeftHandle.anchors.top = undefined;
                topRightHandle.anchors.top = undefined;
                leftHandle.anchors.verticalCenter = undefined;
                rightHandle.anchors.verticalCenter = undefined;
            }
        }
    }

    Rectangle {
        id: bottomHandle

        color: handleColor
        width: handleSize
        height: handleSize
        visible: aspectRatio !== 0 ? false : !isRotated()

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeVerCursor
            drag.target: parent
            onEntered: {
                rectangle.anchors.bottom = parent.bottom;
                bottomLeftHandle.anchors.bottom = rectangle.bottom;
                bottomRightHandle.anchors.bottom = rectangle.bottom;
                leftHandle.anchors.verticalCenter = rectangle.verticalCenter;
                rightHandle.anchors.verticalCenter = rectangle.verticalCenter;
            }
            onPositionChanged: mouse => {
                bottomHandle.x = topLeftHandle.x + rectangle.width / 2 - (handleSize / 2);
                if (!(mouse.modifiers & Qt.AltModifier))
                    bottomHandle.y = snapY(bottomHandle.y + handleSize) - handleSize;
                coerceReframeEven(parent);
                parent.x = Math.max(parent.x, topHandle.x);
                parent.y = Math.max(parent.y, topHandle.y);
                rectChanged(rectangle);
            }
            onReleased: {
                rectChanged(rectangle);
                bottomLeftHandle.anchors.bottom = undefined;
                bottomRightHandle.anchors.bottom = undefined;
                leftHandle.anchors.verticalCenter = undefined;
                rightHandle.anchors.verticalCenter = undefined;
            }
        }
    }

    Rectangle {
        id: leftHandle

        color: handleColor
        width: handleSize
        height: handleSize
        visible: aspectRatio !== 0 ? false : !isRotated()

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            onEntered: {
                rectangle.anchors.left = parent.left;
                topLeftHandle.anchors.left = rectangle.left;
                bottomLeftHandle.anchors.left = rectangle.left;
                topHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                bottomHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
            }
            onPositionChanged: mouse => {
                if (!(mouse.modifiers & Qt.AltModifier))
                    leftHandle.x = snapX(leftHandle.x);
                leftHandle.y = topLeftHandle.y + rectangle.height / 2 - (handleSize / 2);
                coerceReframeEven(parent);
                parent.x = Math.min(parent.x, rightHandle.x);
                parent.y = Math.min(parent.y, rightHandle.y);
                rectChanged(rectangle);
            }
            onReleased: {
                rectChanged(rectangle);
                topLeftHandle.anchors.left = undefined;
                bottomLeftHandle.anchors.left = undefined;
                topHandle.anchors.horizontalCenter = undefined;
                bottomHandle.anchors.horizontalCenter = undefined;
            }
        }
    }

    Rectangle {
        id: rightHandle

        color: handleColor
        width: handleSize
        height: handleSize
        visible: aspectRatio !== 0 ? false : !isRotated()

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            onEntered: {
                rectangle.anchors.right = parent.right;
                topRightHandle.anchors.right = rectangle.right;
                bottomRightHandle.anchors.right = rectangle.right;
                topHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                bottomHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
            }
            onPositionChanged: mouse => {
                if (!(mouse.modifiers & Qt.AltModifier))
                    rightHandle.x = snapX(rightHandle.x + handleSize) - handleSize;
                rightHandle.y = topLeftHandle.y + rectangle.height / 2 - (handleSize / 2);
                coerceReframeEven(parent);
                parent.x = Math.max(parent.x, leftHandle.x);
                parent.y = Math.max(parent.y, leftHandle.y);
                rectChanged(rectangle);
            }
            onReleased: {
                rectChanged(rectangle);
                topRightHandle.anchors.right = undefined;
                bottomRightHandle.anchors.right = undefined;
                topHandle.anchors.horizontalCenter = undefined;
                bottomHandle.anchors.horizontalCenter = undefined;
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
                rectangle.anchors.top = parent.top;
                rectangle.anchors.right = parent.right;
                rectangle.anchors.bottom = bottomLeftHandle.bottom;
                rectangle.anchors.left = bottomLeftHandle.left;
                topLeftHandle.anchors.top = rectangle.top;
                bottomRightHandle.anchors.right = rectangle.right;
                topHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                topHandle.anchors.top = rectangle.top;
                bottomHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                bottomHandle.anchors.bottom = rectangle.bottom;
                leftHandle.anchors.verticalCenter = rectangle.verticalCenter;
                leftHandle.anchors.left = rectangle.left;
                rightHandle.anchors.verticalCenter = rectangle.verticalCenter;
                rightHandle.anchors.right = rectangle.right;
            }
            onPositionChanged: mouse => {
                if (!(mouse.modifiers & Qt.AltModifier)) {
                    topRightHandle.x = snapX(topRightHandle.x + handleSize) - handleSize;
                    topRightHandle.y = snapY(topRightHandle.y);
                }
                if (aspectRatio !== 0)
                    parent.x = topLeftHandle.x + rectangle.height * aspectRatio - handleSize;
                coerceReframeEven(parent);
                parent.x = Math.max(parent.x, bottomLeftHandle.x);
                parent.y = Math.min(parent.y, bottomLeftHandle.y);
                rectChanged(rectangle);
            }
            onReleased: {
                rectChanged(rectangle);
                topLeftHandle.anchors.top = undefined;
                bottomRightHandle.anchors.right = undefined;
                topHandle.anchors.horizontalCenter = undefined;
                topHandle.anchors.top = undefined;
                bottomHandle.anchors.horizontalCenter = undefined;
                bottomHandle.anchors.bottom = undefined;
                leftHandle.anchors.verticalCenter = undefined;
                leftHandle.anchors.left = undefined;
                rightHandle.anchors.verticalCenter = undefined;
                rightHandle.anchors.right = undefined;
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
                rectangle.anchors.bottom = parent.bottom;
                rectangle.anchors.left = parent.left;
                rectangle.anchors.top = topRightHandle.top;
                rectangle.anchors.right = topRightHandle.right;
                topLeftHandle.anchors.left = rectangle.left;
                bottomRightHandle.anchors.bottom = rectangle.bottom;
                topHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                topHandle.anchors.top = rectangle.top;
                bottomHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                bottomHandle.anchors.bottom = rectangle.bottom;
                leftHandle.anchors.verticalCenter = rectangle.verticalCenter;
                leftHandle.anchors.left = rectangle.left;
                rightHandle.anchors.verticalCenter = rectangle.verticalCenter;
                rightHandle.anchors.right = rectangle.right;
            }
            onPositionChanged: mouse => {
                if (!(mouse.modifiers & Qt.AltModifier)) {
                    bottomLeftHandle.x = snapX(bottomLeftHandle.x);
                    bottomLeftHandle.y = snapY(bottomLeftHandle.y + handleSize) - handleSize;
                }
                if (aspectRatio !== 0)
                    parent.x = topRightHandle.x + handleSize - rectangle.height * aspectRatio;
                coerceReframeEven(parent);
                parent.x = Math.min(parent.x, topRightHandle.x);
                parent.y = Math.max(parent.y, topRightHandle.y);
                rectChanged(rectangle);
            }
            onReleased: {
                rectChanged(rectangle);
                topLeftHandle.anchors.left = undefined;
                bottomRightHandle.anchors.bottom = undefined;
                topHandle.anchors.horizontalCenter = undefined;
                topHandle.anchors.top = undefined;
                bottomHandle.anchors.horizontalCenter = undefined;
                bottomHandle.anchors.bottom = undefined;
                leftHandle.anchors.verticalCenter = undefined;
                leftHandle.anchors.left = undefined;
                rightHandle.anchors.verticalCenter = undefined;
                rightHandle.anchors.right = undefined;
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
                rectangle.anchors.bottom = parent.bottom;
                rectangle.anchors.right = parent.right;
                topRightHandle.anchors.right = rectangle.right;
                bottomLeftHandle.anchors.bottom = rectangle.bottom;
                topHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                topHandle.anchors.top = rectangle.top;
                bottomHandle.anchors.horizontalCenter = rectangle.horizontalCenter;
                bottomHandle.anchors.bottom = rectangle.bottom;
                leftHandle.anchors.verticalCenter = rectangle.verticalCenter;
                leftHandle.anchors.left = rectangle.left;
                rightHandle.anchors.verticalCenter = rectangle.verticalCenter;
                rightHandle.anchors.right = rectangle.right;
            }
            onPositionChanged: mouse => {
                if (!(mouse.modifiers & Qt.AltModifier)) {
                    bottomRightHandle.x = snapX(bottomRightHandle.x + handleSize) - handleSize;
                    bottomRightHandle.y = snapY(bottomRightHandle.y + handleSize) - handleSize;
                }
                if (aspectRatio !== 0)
                    parent.x = topLeftHandle.x + rectangle.height * aspectRatio - handleSize;
                coerceReframeEven(parent);
                parent.x = Math.max(parent.x, topLeftHandle.x);
                parent.y = Math.max(parent.y, topLeftHandle.y);
                rectChanged(rectangle);
            }
            onReleased: {
                rectChanged(rectangle);
                topRightHandle.anchors.right = undefined;
                bottomLeftHandle.anchors.bottom = undefined;
                topHandle.anchors.horizontalCenter = undefined;
                topHandle.anchors.top = undefined;
                bottomHandle.anchors.horizontalCenter = undefined;
                bottomHandle.anchors.bottom = undefined;
                leftHandle.anchors.verticalCenter = undefined;
                leftHandle.anchors.left = undefined;
                rightHandle.anchors.verticalCenter = undefined;
                rightHandle.anchors.right = undefined;
            }
        }
    }
}
