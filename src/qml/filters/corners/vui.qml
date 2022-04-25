/*
 * Copyright (c) 2020 Meltytech, LLC
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
import Shotcut.Controls 1.0 as Shotcut


Shotcut.VuiBase {
    property string corner1xProperty: '0'
    property string corner1yProperty: '1'
    property string corner2xProperty: '2'
    property string corner2yProperty: '3'
    property string corner3xProperty: '4'
    property string corner3yProperty: '5'
    property string corner4xProperty: '6'
    property string corner4yProperty: '7'

    property var cornerProperties: ['shotcut:corner1', 'shotcut:corner2', 'shotcut:corner3', 'shotcut:corner4']
    property var corners: [Qt.rect(0, 0, 0, 0), Qt.rect(profile.width, 0, 0, 0), Qt.rect(profile.width, profile.height, 0, 0), Qt.rect(0, profile.height, 0, 0)]
    property var cornerStartValues: ['_shotcut:corner1StartValue', '_shotcut:corner2StartValue', '_shotcut:corner3StartValue', '_shotcut:corner4StartValue']
    property var cornerMiddleValues: ['_shotcut:corner1MiddleValue', '_shotcut:corner2MiddleValue', '_shotcut:corner3MiddleValue', '_shotcut:corner4MiddleValue']
    property var cornerEndValues: ['_shotcut:corner1EndValue', '_shotcut:corner2EndValue', '_shotcut:corner3EndValue', '_shotcut:corner4EndValue']

    property bool blockUpdate: false
    property real zoom: (video.zoom > 0)? video.zoom : 1.0
    property int handleSize: Math.max(Math.round(20 / zoom), 8)
    property real handleOffset: handleSize / 2
    property int borderSize: Math.max(Math.round(4 / zoom), 3)
    property color handleColor: 'white'
    property color borderColor: 'black'
    property int snapMargin: 10

    Component.onCompleted: {
        setCornersControl()
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setCornersControl() {
        var position = getPosition()
        blockUpdate = true
        for (var i in corners)
            corners[i] = filter.getRect(cornerProperties[i], position)
        setHandles()
        cornersControl.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
        blockUpdate = false
    }

    function resetFilter() {
        filter.resetProperty(corner1xProperty)
        filter.resetProperty(corner1yProperty)
        filter.resetProperty(corner2xProperty)
        filter.resetProperty(corner2yProperty)
        filter.resetProperty(corner3xProperty)
        filter.resetProperty(corner3yProperty)
        filter.resetProperty(corner4xProperty)
        filter.resetProperty(corner4yProperty)
        for (var i in cornerProperties)
            filter.resetProperty(cornerProperties[i])
    }

    function setFilterCorners(corners, position) {
        for (var i in cornerProperties)
            filter.set(cornerProperties[i], corners[i], position)
        filter.set(corner1xProperty, corners[0].x, position)
        filter.set(corner1yProperty, corners[0].y, position)
        filter.set(corner2xProperty, corners[1].x, position)
        filter.set(corner2yProperty, corners[1].y, position)
        filter.set(corner3xProperty, corners[2].x, position)
        filter.set(corner3yProperty, corners[2].y, position)
        filter.set(corner4xProperty, corners[3].x, position)
        filter.set(corner4yProperty, corners[3].y, position)
    }


    function updateFilterCorners(position) {
        if (blockUpdate) return

        corners[0].x = mapValueBack((corner1Handle.x + handleOffset) / video.rect.width)
        corners[0].y = mapValueBack((corner1Handle.y + handleOffset) / video.rect.height)
        corners[1].x = mapValueBack((corner2Handle.x + handleOffset) / video.rect.width)
        corners[1].y = mapValueBack((corner2Handle.y + handleOffset) / video.rect.height)
        corners[2].x = mapValueBack((corner3Handle.x + handleOffset) / video.rect.width)
        corners[2].y = mapValueBack((corner3Handle.y + handleOffset) / video.rect.height)
        corners[3].x = mapValueBack((corner4Handle.x + handleOffset) / video.rect.width)
        corners[3].y = mapValueBack((corner4Handle.y + handleOffset) / video.rect.height)

        if (position !== null) {
            filter.blockSignals = true
            if (position <= 0 && filter.animateIn > 0) {
                for (var i in cornerStartValues)
                    filter.set(cornerStartValues[i], corners[i])
            } else if (position >= filter.duration - 1 && filter.animateOut > 0) {
                for (i in cornerEndValues)
                    filter.set(cornerEndValues[i], corners[i])
            } else {
                for (i in cornerMiddleValues)
                    filter.set(cornerMiddleValues[i], corners[i])
            }
            filter.blockSignals = false
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            resetFilter()
            if (filter.animateIn > 0) {
                setFilterCorners([filter.getRect(cornerStartValues[0]),filter.getRect(cornerStartValues[1]),filter.getRect(cornerStartValues[2]),filter.getRect(cornerStartValues[3])], 0)
                setFilterCorners([filter.getRect(cornerMiddleValues[0]),filter.getRect(cornerMiddleValues[1]),filter.getRect(cornerMiddleValues[2]),filter.getRect(cornerMiddleValues[3])], filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                setFilterCorners([filter.getRect(cornerMiddleValues[0]),filter.getRect(cornerMiddleValues[1]),filter.getRect(cornerMiddleValues[2]),filter.getRect(cornerMiddleValues[3])], filter.duration - filter.animateOut)
                setFilterCorners([filter.getRect(cornerEndValues[0]),filter.getRect(cornerEndValues[1]),filter.getRect(cornerEndValues[2]),filter.getRect(cornerEndValues[3])], filter.duration - 1)
            }
        } else if (filter.keyframeCount(corner1xProperty) <= 0) {
            resetFilter()
            setFilterCorners([filter.getRect(cornerMiddleValues[0]),filter.getRect(cornerMiddleValues[1]),filter.getRect(cornerMiddleValues[2]),filter.getRect(cornerMiddleValues[3])], -1)
        } else if (position !== null) {
            setFilterCorners(corners, position)
        }
    }

    function mapValueForward(value) {
        var min = -1
        var max = 2
        return min + (max - min) * value
    }

    function mapValueBack(value) {
        var min = -1
        var max = 2
        return (value - min) / (max - min)
    }

    function setHandles() {
        corner1Handle.x = mapValueForward(corners[0].x) * video.rect.width - handleOffset
        corner1Handle.y = mapValueForward(corners[0].y) * video.rect.height - handleOffset
        corner2Handle.x = mapValueForward(corners[1].x) * video.rect.width - handleOffset
        corner2Handle.y = mapValueForward(corners[1].y) * video.rect.height - handleOffset
        corner3Handle.x = mapValueForward(corners[2].x) * video.rect.width - handleSize/2
        corner3Handle.y = mapValueForward(corners[2].y) * video.rect.height - handleOffset
        corner4Handle.x = mapValueForward(corners[3].x) * video.rect.width - handleOffset
        corner4Handle.y = mapValueForward(corners[3].y) * video.rect.height - handleOffset
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
            var n = (video.grid > 10000) ? cornersControl.width / (profile.width / (video.grid - 10000)) : cornersControl.width / video.grid
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
                    var delta = x - deltas[i] * cornersControl.width
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
            var n = (video.grid > 10000) ? cornersControl.height / (profile.height / (video.grid - 10000)) : cornersControl.height / video.grid
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
                    var delta = y - deltas[i] * cornersControl.height
                    if (Math.abs(delta) < snapMargin)
                        return y - delta
                }
            }
        }
        return y
    }

    Flickable {
        anchors.fill: parent
        interactive: false
        clip: true
        contentWidth: video.rect.width * zoom
        contentHeight: video.rect.height * zoom
        contentX: video.offset.x
        contentY: video.offset.y

        Item {
            id: videoItem
            x: video.rect.x
            y: video.rect.y
            width: video.rect.width
            height: video.rect.height
            scale: zoom

            Item {
                id: cornersControl
                anchors.fill: parent
                opacity: enabled? 0.8 : 0.2

                Rectangle {
                    id: corner1Handle
                    color: handleColor
                    border.color: borderColor
                    width: handleSize
                    height: handleSize
                    radius: handleSize/2
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        cursorShape: Qt.SizeFDiagCursor
                        drag.target: parent
                        onPositionChanged: {
                            corner1Handle.x = snapX(corner1Handle.x + handleOffset) - handleOffset
                            corner1Handle.y = snapY(corner1Handle.y + handleOffset) - handleOffset
                            updateFilterCorners(getPosition())
                        }
                    }
                    Text {
                        text: qsTr('1')
                        font.pixelSize: handleSize * 0.8
                        anchors.centerIn: parent
                    }
                }

                Rectangle {
                    id: corner2Handle
                    color: handleColor
                    border.color: borderColor
                    width: handleSize
                    height: handleSize
                    radius: handleSize/2
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        cursorShape: Qt.SizeBDiagCursor
                        drag.target: parent
                        onPositionChanged: {
                            corner2Handle.x = snapX(corner2Handle.x + handleOffset) - handleOffset
                            corner2Handle.y = snapY(corner2Handle.y + handleOffset) - handleOffset
                            updateFilterCorners(getPosition())
                        }
                    }
                    Text {
                        text: qsTr('2')
                        font.pixelSize: handleSize * 0.8
                        anchors.centerIn: parent
                    }
                }

                Rectangle {
                    id: corner3Handle
                    color: handleColor
                    border.color: borderColor
                    width: handleSize
                    height: handleSize
                    radius: handleSize/2
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        cursorShape: Qt.SizeFDiagCursor
                        drag.target: parent
                        onPositionChanged: {
                            corner3Handle.x = snapX(corner3Handle.x + handleOffset) - handleOffset
                            corner3Handle.y = snapY(corner3Handle.y + handleOffset) - handleOffset
                            updateFilterCorners(getPosition())
                        }
                    }
                    Text {
                        text: qsTr('3')
                        font.pixelSize: handleSize * 0.8
                        anchors.centerIn: parent
                    }
                }

                Rectangle {
                    id: corner4Handle
                    color: handleColor
                    border.color: borderColor
                    width: handleSize
                    height: handleSize
                    radius: handleSize/2
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        cursorShape: Qt.SizeBDiagCursor
                        drag.target: parent
                        onPositionChanged: {
                            corner4Handle.x = snapX(corner4Handle.x + handleOffset) - handleOffset
                            corner4Handle.y = snapY(corner4Handle.y + handleOffset) - handleOffset
                            updateFilterCorners(getPosition())
                        }
                    }
                    Text {
                        text: qsTr('4')
                        font.pixelSize: handleSize * 0.8
                        anchors.centerIn: parent
                    }
                }
            }
        }
    }

    Connections {
        target: filter
        onChanged: {
            setCornersControl()
            videoItem.enabled = filter.get('disable') !== '1'
        }
    }

    Connections {
        target: producer
        onPositionChanged: setCornersControl()
    }
    
    Connections {
        target: video
        onRectChanged: setCornersControl()
    }
}

