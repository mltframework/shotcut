/*
 * Copyright (c) 2018-2020 Meltytech, LLC
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
    property string rectProperty: 'rect'
    property real zoom: (video.zoom > 0)? video.zoom : 1.0
    property rect filterRect
    property bool blockUpdate: false
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue:  '_shotcut:endValue'

    Component.onCompleted: {
        application.showStatusMessage(qsTr('Click in rectangle + hold Shift to drag'))
        filterRect = filter.getRect(rectProperty, getPosition())
        rectangle.setHandles(filterRect)
        setRectangleControl()
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setRectangleControl() {
        if (blockUpdate) return
        var position = getPosition()
        var newValue = filter.getRect(rectProperty, position)
        if (filterRect !== newValue) {
            filterRect = newValue
            rectangle.setHandles(filterRect)
        }
        rectangle.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
    }

    function setFilter(position) {
        blockUpdate = true
        var rect = rectangle.rectangle
        filterRect.x = Math.round(rect.x / rectangle.widthScale)
        filterRect.y = Math.round(rect.y / rectangle.heightScale)
        filterRect.width = Math.round(rect.width / rectangle.widthScale)
        filterRect.height = Math.round(rect.height / rectangle.heightScale)

        if (position !== null) {
            filter.blockSignals = true
            if (position <= 0 && filter.animateIn > 0)
                filter.set(startValue, filterRect)
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                filter.set(endValue, filterRect)
            else
                filter.set(middleValue, filterRect)
            filter.blockSignals = false
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(rectProperty)
            if (filter.animateIn > 0) {
                filter.set(rectProperty, filter.getRect(startValue), 0)
                filter.set(rectProperty, filter.getRect(middleValue), filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set(rectProperty, filter.getRect(middleValue), filter.duration - filter.animateOut)
                filter.set(rectProperty, filter.getRect(endValue), filter.duration - 1)
            }
        } else if (filter.keyframeCount(rectProperty) <= 0) {
            filter.resetProperty(rectProperty)
            filter.set(rectProperty, filter.getRect(middleValue))
        } else if (position !== null) {
            filter.set(rectProperty, filterRect, position)
        }
        blockUpdate = false
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

            Shotcut.RectangleControl {
                id: rectangle
                widthScale: video.rect.width / profile.width
                heightScale: video.rect.height / profile.height
                handleSize: Math.max(Math.round(8 / zoom), 4)
                borderSize: Math.max(Math.round(1.33 / zoom), 1)
                onWidthScaleChanged: setHandles(filterRect)
                onHeightScaleChanged: setHandles(filterRect)
                onRectChanged: setFilter(getPosition())
            }
        }
    }

    Connections {
        target: filter
        function onChanged() {
            setRectangleControl()
            videoItem.enabled = filter.get('disable') !== '1'
        }
    }

    Connections {
        target: producer
        function onPositionChanged() { setRectangleControl() }
    }
}
