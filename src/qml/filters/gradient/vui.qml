/*
 * Copyright (c) 2019-2022 Meltytech, LLC
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
    property string patternProperty: '0'
    property string startXProperty: '5'
    property string startYProperty: '6'
    property string endXProperty: '7'
    property string endYProperty: '8'
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

    function resetFilterRect() {
        filter.resetProperty(rectProperty);
        filter.resetProperty(startXProperty);
        filter.resetProperty(startYProperty);
        filter.resetProperty(endXProperty);
        filter.resetProperty(endYProperty);
    }

    function updateFilterRect(rect, position) {
        if (position === null)
            position = -1;
        filter.set(rectProperty, rect, position);
        if (filter.get(patternProperty) === 'gradient_linear') {
            filter.set(startXProperty, rect.x / profile.width, position);
            filter.set(startYProperty, rect.y / profile.height, position);
            filter.set(endXProperty, (rect.x + rect.width) / profile.width, position);
            filter.set(endYProperty, (rect.y + rect.height) / profile.height, position);
        } else {
            filter.set(startXProperty, (rect.x + rect.width / 2) / profile.width, position);
            filter.set(startYProperty, (rect.y + rect.height / 2) / profile.height, position);
            filter.set(endXProperty, (rect.x + rect.width) / profile.width, position);
            filter.set(endYProperty, (rect.y + rect.height) / profile.height, position);
        }
    }

    function setFilter(position) {
        blockUpdate = true;
        var rect = rectangle.rectangle;
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
        resetFilterRect();
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            if (filter.animateIn > 0) {
                updateFilterRect(filter.getRect(startValue), 0);
                updateFilterRect(filter.getRect(middleValue), filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                updateFilterRect(filter.getRect(middleValue), filter.duration - filter.animateOut);
                updateFilterRect(filter.getRect(endValue), filter.duration - 1);
            }
        } else {
            updateFilterRect(filter.getRect(middleValue));
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
                onRectChanged: setFilter(getPosition())
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
