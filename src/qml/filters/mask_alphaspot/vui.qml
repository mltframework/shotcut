/*
 * Copyright (c) 2023 Meltytech, LLC
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
import Shotcut.Controls as Shotcut

Shotcut.VuiBase {
    property string rectProperty: 'shotcut:rect'
    property string paramHorizontal: 'filter.1'
    property string paramVertical: 'filter.2'
    property string paramWidth: 'filter.3'
    property string paramHeight: 'filter.4'
    property real zoom: (video.zoom > 0) ? video.zoom : 1
    property rect filterRect: Qt.rect(-1, -1, -1, -1)
    property bool blockUpdate: false
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue: '_shotcut:endValue'

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setRectangleControl() {
        if (blockUpdate)
            return;
        var position = getPosition();
        var newValue = filter.getRect(rectProperty, position);
        if (filterRect !== newValue) {
            filterRect = newValue;
            rectangle.setHandles(filterRect);
        }
        rectangle.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function resetRectProperty() {
        filter.resetProperty(rectProperty);
        filter.resetProperty(paramHorizontal);
        filter.resetProperty(paramVertical);
        filter.resetProperty(paramWidth);
        filter.resetProperty(paramHeight);
    }

    function setFilterRectProperties(rect, position) {
        if (position === null)
            position = -1;
        filter.set(rectProperty, rect, position);
        rect.width /= profile.width * 2;
        rect.height /= profile.height * 2;
        rect.x = rect.x / profile.width + rect.width;
        rect.y = rect.y / profile.height + rect.height;
        filter.set(paramHorizontal, rect.x, position);
        filter.set(paramVertical, rect.y, position);
        filter.set(paramWidth, rect.width, position);
        filter.set(paramHeight, rect.height, position);
    }

    function updateFilterRect(position) {
        blockUpdate = true;
        let rect = rectangle.rectangle;
        filterRect.x = Math.round(rect.x / rectangle.widthScale);
        filterRect.y = Math.round(rect.y / rectangle.heightScale);
        filterRect.width = Math.round(rect.width / rectangle.widthScale);
        filterRect.height = Math.round(rect.height / rectangle.heightScale);
        if (position !== null) {
            filter.blockSignals = true;
            if (position <= 0 && filter.animateIn > 0)
                filter.set(startValue, filterRect);
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                filter.set(endValue, filterRect);
            else
                filter.set(middleValue, filterRect);
            filter.blockSignals = false;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            resetRectProperty();
            if (filter.animateIn > 0) {
                setFilterRectProperties(filter.getRect(startValue), 0);
                setFilterRectProperties(filter.getRect(middleValue), filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                setFilterRectProperties(filter.getRect(middleValue), filter.duration - filter.animateOut);
                setFilterRectProperties(filter.getRect(endValue), filter.duration - 1);
            }
        } else if (filter.keyframeCount(rectProperty) <= 0) {
            resetRectProperty();
            setFilterRectProperties(filter.getRect(middleValue));
        } else if (position !== null) {
            setFilterRectProperties(filterRect, position);
        }
        blockUpdate = false;
    }

    Component.onCompleted: {
        application.showStatusMessage(qsTr('Click in rectangle + hold Shift to drag'));
        setRectangleControl();
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
                onRectChanged: updateFilterRect(getPosition())
            }
        }
    }

    Connections {
        function onChanged() {
            setRectangleControl();
            videoItem.enabled = filter.get('disable') !== '1';
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setRectangleControl();
        }

        target: producer
    }
}
