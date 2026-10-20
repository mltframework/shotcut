/*
 * Copyright (c) 2026 Meltytech, LLC
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
    property string rotationProperty: ''
    property bool rectKeyframes: false
    property bool rotationKeyframes: false
    property bool normalizedCoordinates: false
    property real zoom: (video.zoom > 0) ? video.zoom : 1
    property rect filterRect: Qt.rect(-1, -1, -1, -1)
    property bool blockUpdate: false
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue: '_shotcut:endValue'

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function rawRectString() {
        var value = filter.get(rectProperty);
        return (value === undefined || value === null) ? '' : String(value);
    }

    function rectStoredInPixels() {
        var flag = filter.get('shotcut:rectPixels');
        return flag === 1 || flag === '1';
    }

    function rectIsUnitless(rect) {
        return Math.abs(rect.x) <= 1 && Math.abs(rect.y) <= 1 && Math.abs(rect.width) <= 1
            && Math.abs(rect.height) <= 1;
    }

    // getRect already returns pixels for a '%' rect. A normalized 0..1 value has
    // no '%' until this control writes profile pixels and sets shotcut:rectPixels.
    function readRect(position) {
        var value = filter.getRect(rectProperty, position);
        if (normalizedCoordinates && !rectStoredInPixels() && rawRectString().indexOf('%') < 0
                && rectIsUnitless(value)) {
            value = Qt.rect(value.x * profile.width,
                            value.y * profile.height,
                            value.width * profile.width,
                            value.height * profile.height);
        }
        return value;
    }

    function displayRect(pixelRect) {
        if (pixelRect.width <= 0 || pixelRect.height <= 0)
            return Qt.rect(0, 0, profile.width, profile.height);
        return pixelRect;
    }

    function sameRect(a, b) {
        return Math.abs(a.x - b.x) < 0.5 && Math.abs(a.y - b.y) < 0.5
            && Math.abs(a.width - b.width) < 0.5 && Math.abs(a.height - b.height) < 0.5;
    }

    function markRectPixels() {
        if (rectStoredInPixels())
            return;
        filter.blockSignals = true;
        filter.set('shotcut:rectPixels', 1);
        filter.blockSignals = false;
    }

    function convertNormalizedKeyframes() {
        if (rectStoredInPixels())
            return;
        if (rawRectString().indexOf('%') >= 0) {
            filter.blockSignals = true;
            filter.removeRectPercents(rectProperty);
            filter.set('shotcut:rectPixels', 1);
            filter.blockSignals = false;
            return;
        }
        if (!normalizedCoordinates || filter.keyframeCount(rectProperty) < 1)
            return;
        var keys = [];
        var anyUnitless = false;
        var pos = filter.getNextKeyframePosition(rectProperty, -1);
        var guard = 0;
        while (pos >= 0 && guard < 10000) {
            var value = filter.getRect(rectProperty, pos);
            var scaled = value;
            if (rectIsUnitless(value)) {
                anyUnitless = true;
                scaled = Qt.rect(value.x * profile.width,
                                 value.y * profile.height,
                                 value.width * profile.width,
                                 value.height * profile.height);
            }
            keys.push(pos);
            keys.push(scaled);
            var next = filter.getNextKeyframePosition(rectProperty, pos);
            if (next === pos)
                break;
            pos = next;
            guard++;
        }
        if (!anyUnitless)
            return;
        filter.blockSignals = true;
        for (var i = 0; i < keys.length; i += 2)
            filter.set(rectProperty, keys[i + 1], keys[i]);
        filter.set('shotcut:rectPixels', 1);
        filter.blockSignals = false;
    }

    function writeRect(pixelRect, position) {
        if (position !== null)
            convertNormalizedKeyframes();
        markRectPixels();
        if (position === null)
            filter.set(rectProperty, pixelRect);
        else
            filter.set(rectProperty, pixelRect, position);
    }

    function rectangleIsEnabled(position) {
        if (!rectKeyframes)
            return true;
        return position <= 0
            || (position >= (filter.animateIn - 1)
                && position <= (filter.duration - filter.animateOut))
            || position >= (filter.duration - 1);
    }

    function rotationHandleEnabled(position) {
        if (!rotationProperty)
            return false;
        if (!rotationKeyframes)
            return true;
        return position <= 0
            || (position >= (filter.animateIn - 1)
                && position <= (filter.duration - filter.animateOut))
            || position >= (filter.duration - 1);
    }

    function setRectangleControl() {
        if (blockUpdate)
            return;
        var position = getPosition();
        var newValue = readRect(position);
        if (filterRect !== newValue) {
            filterRect = newValue;
            rectangle.setHandles(displayRect(filterRect));
        }
        if (rotationProperty)
            rectangle.rotation = filter.getDouble(rotationProperty, position);
        rectangle.enabled = rectangleIsEnabled(position);
        rectangle.withRotation = rotationHandleEnabled(position);
    }

    // The player installs this overlay after undo tracking has started.
    // These slots are a cache, so a load or zoom reload must not push a command.
    function seedRectSlot(name, rect) {
        var raw = filter.get(name);
        if (raw !== undefined && raw !== null && String(raw) !== ''
                && sameRect(filter.getRect(name), rect))
            return;
        filter.setRectNoUndo(name, rect);
    }

    function seedRectSlots() {
        if (!rectKeyframes)
            return;
        var middlePos = filter.animateIn > 0 ? Math.max(filter.animateIn - 1, 0) : 0;
        seedRectSlot(startValue, readRect(0));
        seedRectSlot(middleValue, readRect(middlePos));
        seedRectSlot(endValue, readRect(Math.max(filter.duration - 1, 0)));
    }

    function rewriteRectAnimation() {
        if (!rectKeyframes || blockUpdate)
            return;
        blockUpdate = true;
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(rectProperty);
            if (filter.animateIn > 0) {
                writeRect(filter.getRect(startValue), 0);
                writeRect(filter.getRect(middleValue), filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                writeRect(filter.getRect(middleValue), filter.duration - filter.animateOut);
                writeRect(filter.getRect(endValue), filter.duration - 1);
            }
        }
        blockUpdate = false;
    }

    function applyRectEdit(position) {
        var rect = rectangle.rectangle;
        var pixel = Qt.rect(rect.x / rectangle.widthScale,
                            rect.y / rectangle.heightScale,
                            rect.width / rectangle.widthScale,
                            rect.height / rectangle.heightScale);
        if ((filterRect.width <= 0 || filterRect.height <= 0)
                && sameRect(pixel, displayRect(filterRect)))
            return;
        blockUpdate = true;
        filterRect.x = pixel.x;
        filterRect.y = pixel.y;
        filterRect.width = pixel.width;
        filterRect.height = pixel.height;
        if (!rectKeyframes) {
            writeRect(filterRect, null);
            blockUpdate = false;
            return;
        }
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
                writeRect(filter.getRect(startValue), 0);
                writeRect(filter.getRect(middleValue), filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                writeRect(filter.getRect(middleValue), filter.duration - filter.animateOut);
                writeRect(filter.getRect(endValue), filter.duration - 1);
            }
        } else if (filter.keyframeCount(rectProperty) <= 0) {
            filter.resetProperty(rectProperty);
            writeRect(filter.getRect(middleValue), null);
        } else if (position !== null) {
            writeRect(filterRect, position);
        }
        blockUpdate = false;
    }

    function applyRotation(value, position) {
        if (!rotationProperty)
            return;
        blockUpdate = true;
        if (!rotationKeyframes) {
            filter.set(rotationProperty, value);
            blockUpdate = false;
            return;
        }
        filter.blockSignals = true;
        filter.set('_shotcut:vuiRotation', value);
        filter.set('_shotcut:vuiRotationPos', position === null ? -1 : position);
        filter.blockSignals = false;
        filter.changed();
        blockUpdate = false;
    }

    // A non-positive stored rect is drawn as the full frame. Wheel zoom starts from that frame.
    function zoomBaseRect() {
        if (filterRect.width <= 0 || filterRect.height <= 0)
            return displayRect(filterRect);
        return Qt.rect(filterRect.x, filterRect.y, filterRect.width, filterRect.height);
    }

    function updateScale(scale) {
        if (!(scale > 0.0001))
            return;
        var current = zoomBaseRect();
        if (Math.abs(scale - current.width / Math.max(profile.width, 1)) <= 0.01)
            return;
        var aspect = current.width / Math.max(current.height, 1);
        var centerX = current.x + current.width / 2;
        var centerY = current.y + current.height / 2;
        current.width = profile.width * Math.min(scale, 10);
        current.height = current.width / Math.max(aspect, 1e-06);
        current.x = centerX - current.width / 2;
        current.y = centerY - current.height / 2;
        filterRect = current;
        rectangle.setHandles(filterRect);
        applyRectEdit(getPosition());
    }

    function snapRotation(degrees, strength) {
        if ((Math.abs(degrees) + strength) % 90 < strength * 2)
            degrees = Math.round(degrees / 90) * 90;
        return degrees;
    }

    Component.onCompleted: {
        if (rotationProperty)
            application.showStatusMessage(qsTr('Click in rectangle + hold Shift to drag, Wheel to zoom, or %1+Wheel to rotate').arg(application.OS === 'macOS' ? 'Cmd' : 'Ctrl'));
        else
            application.showStatusMessage(qsTr('Click in rectangle + hold Shift to drag, Wheel to zoom'));
        seedRectSlots();
        setRectangleControl();
    }

    PinchArea {
        property real currentRotation: 0
        property bool noModifiers: true

        anchors.fill: parent
        pinch.minimumRotation: -360
        pinch.maximumRotation: 360
        pinch.minimumScale: 0.1
        pinch.maximumScale: 10
        Keys.onPressed: event => {
            noModifiers = event.modifiers === Qt.NoModifier;
        }
        Keys.onReleased: event => {
            noModifiers = event.modifiers === Qt.NoModifier;
        }
        onPinchStarted: {
            currentRotation = rectangle.rotation;
        }
        onPinchUpdated: {
            if (noModifiers && rotationProperty && Math.abs(pinch.rotation - 0) > 0.01) {
                var degrees = currentRotation + pinch.rotation;
                rectangle.rotation = snapRotation(degrees, 4);
                applyRotation(rectangle.rotation % 360, getPosition());
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            scrollGestureEnabled: true
            onWheel: wheel => {
                if (rotationProperty && (wheel.modifiers & Qt.ControlModifier)) {
                    var angleDelta = wheel.angleDelta.y;
                    if (!angleDelta && wheel.modifiers & Qt.AltModifier)
                        angleDelta = wheel.angleDelta.x;
                    var degrees = rectangle.rotation - angleDelta / 120 * 5;
                    if (!(wheel.modifiers & Qt.AltModifier))
                        degrees = snapRotation(degrees, 1.5);
                    rectangle.rotation = degrees;
                    applyRotation(rectangle.rotation % 360, getPosition());
                } else if (!blockUpdate) {
                    var scale = zoomBaseRect().width / Math.max(profile.width, 1);
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
                onWidthScaleChanged: setHandles(displayRect(filterRect))
                onHeightScaleChanged: setHandles(displayRect(filterRect))
                onRectChanged: applyRectEdit(getPosition())
                onRotated: (degrees, mouse) => {
                    if (!(mouse.modifiers & Qt.AltModifier))
                        degrees = snapRotation(degrees, 4) % 360;
                    applyRotation(degrees, getPosition());
                }
                onRotationReleased: {
                    if (rotationProperty)
                        rectangle.rotation = filter.getDouble(rotationProperty, getPosition());
                }
            }
        }
    }

    Connections {
        function onChanged() {
            setRectangleControl();
            videoItem.enabled = filter.get('disable') !== '1';
        }

        function onAnimateInChanged() {
            setRectangleControl();
        }

        function onAnimateOutChanged() {
            setRectangleControl();
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
