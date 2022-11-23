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
import Shotcut.Controls as Shotcut

Shotcut.VuiBase {
    property string rectProperty
    property string fillProperty
    property string distortProperty
    property string halignProperty
    property string valignProperty
    property string rotationProperty
    property real zoom: (video.zoom > 0) ? video.zoom : 1
    property rect filterRect: Qt.rect(-1, -1, -1, -1)
    property bool blockUpdate: false
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue: '_shotcut:endValue'
    property string rotationStartValue: '_shotcut:rotationStartValue'
    property string rotationMiddleValue: '_shotcut:rotationMiddleValue'
    property string rotationEndValue: '_shotcut:rotationEndValue'

    function getAspectRatio() {
        return (filter.get(fillProperty) === '1' && filter.get(distortProperty) === '0') ? producer.displayAspectRatio : 0;
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setRectangleControl() {
        if (blockUpdate)
            return ;

        var position = getPosition();
        var newValue = filter.getRect(rectProperty, position);
        if (filterRect !== newValue) {
            filterRect = newValue;
            rectangle.setHandles(filterRect);
        }
        if (rotationProperty)
            rectangle.rotation = filter.getDouble(rotationProperty, position);

        rectangle.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function setFilter(position) {
        blockUpdate = true;
        var rect = rectangle.rectangle;
        filterRect.x = rect.x / rectangle.widthScale;
        filterRect.y = rect.y / rectangle.heightScale;
        filterRect.width = rect.width / rectangle.widthScale;
        filterRect.height = rect.height / rectangle.heightScale;
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
            filter.resetProperty(rectProperty);
            if (filter.animateIn > 0) {
                filter.set(rectProperty, filter.getRect(startValue), 0);
                filter.set(rectProperty, filter.getRect(middleValue), filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set(rectProperty, filter.getRect(middleValue), filter.duration - filter.animateOut);
                filter.set(rectProperty, filter.getRect(endValue), filter.duration - 1);
            }
        } else if (filter.keyframeCount(rectProperty) <= 0) {
            filter.resetProperty(rectProperty);
            filter.set(rectProperty, filter.getRect(middleValue));
        } else if (position !== null) {
            filter.set(rectProperty, filterRect, position);
        }
        blockUpdate = false;
    }

    function updateRotation(value) {
        var position = getPosition();
        if (position !== null) {
            filter.blockSignals = true;
            if (position <= 0 && filter.animateIn > 0)
                filter.set(rotationStartValue, value);
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                filter.set(rotationEndValue, value);
            else
                filter.set(rotationMiddleValue, value);
            filter.blockSignals = false;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(rotationProperty);
            if (filter.animateIn > 0) {
                filter.set(rotationProperty, filter.getDouble(rotationStartValue), 0);
                filter.set(rotationProperty, filter.getDouble(rotationMiddleValue), filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set(rotationProperty, filter.getDouble(rotationMiddleValue), filter.duration - filter.animateOut);
                filter.set(rotationProperty, filter.getDouble(rotationEndValue), filter.duration - 1);
            }
        } else if (filter.keyframeCount(rotationProperty) <= 0) {
            filter.resetProperty(rotationProperty);
            filter.set(rotationProperty, filter.getDouble(rotationMiddleValue));
        } else if (position !== null) {
            filter.set(rotationProperty, value, position);
        }
    }

    function updateScale(scale) {
        if (scale > 0.0001 && Math.abs(scale - filterRect.width / profile.width) > 0.01) {
            var aspectRatio = filterRect.width / Math.max(filterRect.height, 1);
            var align = filter.get(halignProperty);
            var centerX = filterRect.x + filterRect.width / 2;
            var rightX = filterRect.x + filterRect.width;
            filterRect.width = (profile.width * scale);
            if (align === 'center' || align === 'middle')
                filterRect.x = centerX - filterRect.width / 2;
            else if (align === 'right')
                filterRect.x = rightX - filterRect.width;
            var middleY = filterRect.y + filterRect.height / 2;
            var bottomY = filterRect.y + filterRect.height;
            align = filter.get(valignProperty);
            filterRect.height = filterRect.width / Math.max(aspectRatio, 1e-06);
            if (align === 'center' || align === 'middle')
                filterRect.y = middleY - filterRect.height / 2;
            else if (align === 'bottom')
                filterRect.y = bottomY - filterRect.height;
            rectangle.setHandles(filterRect);
            setFilter(getPosition());
        }
    }

    function snapRotation(degrees, strength) {
        if ((Math.abs(degrees) + strength) % 90 < strength * 2)
            degrees = Math.round(degrees / 90) * 90;

        return degrees;
    }

    Component.onCompleted: {
        application.showStatusMessage(qsTr('Click in rectangle + hold Shift to drag, Wheel to zoom, or %1+Wheel to rotate').arg(application.OS === 'OS X' ? 'Cmd' : 'Ctrl'));
        rectangle.aspectRatio = getAspectRatio();
        setRectangleControl();
    }

    PinchArea {
        // Pinch zoom conflicts too much with mouse wheel
        //            if (!blockUpdate) {
        //                var scale = currentScale + (pinch.scale - 2) / 3
        //                if (Math.abs(scale - 1.0) < 0.05)
        //                    scale = 1.0
        //                updateScale(Math.min(scale, 10))
        //            }

        property real currentScale: 1
        property real currentRotation: 0
        property bool noModifiers: true

        anchors.fill: parent
        pinch.minimumRotation: -360
        pinch.maximumRotation: 360
        pinch.minimumScale: 0.1
        pinch.maximumScale: 10
        Keys.onPressed: {
            noModifiers = event.modifiers === Qt.NoModifier;
        }
        Keys.onReleased: {
            noModifiers = event.modifiers === Qt.NoModifier;
        }
        onPinchStarted: {
            currentRotation = rectangle.rotation;
            currentScale = filterRect.width / profile.width;
        }
        onPinchUpdated: {
            if (noModifiers) {
                if (rotationProperty && Math.abs(pinch.rotation - 0) > 0.01) {
                    var degrees = currentRotation + pinch.rotation;
                    rectangle.rotation = snapRotation(degrees, 4);
                    blockUpdate = true;
                    updateRotation(rectangle.rotation % 360);
                    blockUpdate = false;
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            scrollGestureEnabled: true
            onWheel: (wheel)=> {
                if (rotationProperty && (wheel.modifiers & Qt.ControlModifier)) {
                    var degrees = rectangle.rotation - wheel.angleDelta.y / 120 * 5;
                    if (!(wheel.modifiers & Qt.AltModifier))
                        degrees = snapRotation(degrees, 1.5);

                    rectangle.rotation = degrees;
                    blockUpdate = true;
                    updateRotation(rectangle.rotation % 360);
                    blockUpdate = false;
                } else if (!blockUpdate) {
                    var scale = filterRect.width / profile.width;
                    scale += wheel.angleDelta.y / 120 / 10;
                    updateScale(Math.min(scale, 10));
                }
            }
        }

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

                withRotation: !!rotationProperty
                widthScale: video.rect.width / profile.width
                heightScale: video.rect.height / profile.height
                handleSize: Math.max(Math.round(8 / zoom), 4)
                borderSize: Math.max(Math.round(1.33 / zoom), 1)
                onWidthScaleChanged: setHandles(filterRect)
                onHeightScaleChanged: setHandles(filterRect)
                onRectChanged: setFilter(getPosition())
                onRotated: (mouse)=> {
                    if (!(mouse.modifiers & Qt.AltModifier))
                        degrees = snapRotation(degrees, 4) % 360;

                    blockUpdate = true;
                    updateRotation(degrees);
                    blockUpdate = false;
                }
                onRotationReleased: {
                    rectangle.rotation = filter.getDouble(rotationProperty, getPosition());
                }
            }

        }

    }

    Connections {
        function onChanged() {
            setRectangleControl();
            if (rectangle.aspectRatio !== getAspectRatio()) {
                rectangle.aspectRatio = getAspectRatio();
                rectangle.setHandles(filterRect);
            }
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
