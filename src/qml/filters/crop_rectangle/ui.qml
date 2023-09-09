/*
 * Copyright (c) 2020-2023 Meltytech, LLC
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
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut
import org.shotcut.qml as Shotcut

Shotcut.KeyframableFilter {
    property string startValueRect: '_shotcut:startValue'
    property string middleValueRect: '_shotcut:middleValue'
    property string endValueRect: '_shotcut:endValue'
    property string rectProperty: 'rect'
    property rect filterRect

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        slider.value = filter.getDouble('radius', position) * slider.maximumValue;
        colorSwatch.value = filter.getColor('color', position);
        radiusKeyframesButton.checked = filter.keyframeCount('radius') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        colorKeyframesButton.checked = filter.keyframeCount('color') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
        slider.enabled = isSimpleKeyframesActive();
    }

    function setRectControls() {
        var position = getPosition();
        var newValue = filter.getRect(rectProperty, position);
        if (filterRect !== newValue) {
            filterRect = newValue;
            rectX.value = filterRect.x.toFixed();
            rectY.value = filterRect.y.toFixed();
            rectW.value = filterRect.width.toFixed();
            rectH.value = filterRect.height.toFixed();
        }
        var enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
        rectX.enabled = enabled;
        rectY.enabled = enabled;
        rectW.enabled = enabled;
        rectH.enabled = enabled;
        positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
    }

    function updateFilterRect(position) {
        var rect;
        if (position !== null) {
            filter.blockSignals = true;
            if (position <= 0 && filter.animateIn > 0)
                filter.set(startValueRect, filterRect);
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                filter.set(endValueRect, filterRect);
            else
                filter.set(middleValueRect, filterRect);
            filter.blockSignals = false;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(rectProperty);
            positionKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                rect = filter.getRect(startValueRect);
                filter.set(rectProperty, rect, 0);
                rect = filter.getRect(middleValueRect);
                filter.set(rectProperty, rect, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                rect = filter.getRect(middleValueRect);
                filter.set(rectProperty, rect, filter.duration - filter.animateOut);
                rect = filter.getRect(endValueRect);
                filter.set(rectProperty, rect, filter.duration - 1);
            }
        } else if (!positionKeyframesButton.checked) {
            filter.resetProperty(rectProperty);
            rect = filter.getRect(middleValueRect);
            filter.set(rectProperty, rect);
        } else if (position !== null) {
            filter.set(rectProperty, filterRect, position);
        }
    }

    function initSimpleKeyframes() {
        middleValues[0] = filter.getDouble('radius', filter.animateIn);
        middleValues[1] = filter.getColor('color', filter.animateIn);
        if (filter.animateIn > 0) {
            startValues[0] = filter.getDouble('radius', 0);
            startValues[1] = filter.getColor('color', 0);
        }
        if (filter.animateOut > 0) {
            endValues[0] = filter.getDouble('radius', filter.duration - 1);
            endValues[1] = filter.getColor('color', filter.duration - 1);
        }
    }

    function updateParameters() {
        updateFilterRect(null);
        updateFilter('radius', slider.value / 100, radiusKeyframesButton, null);
        updateFilter('color', colorSwatch.value, colorKeyframesButton, null);
    }

    function applyTracking(motionTrackerRow, operation, frame) {
        motionTrackerModel.reset(filter, rectProperty, motionTrackerRow);
        const data = motionTrackerModel.trackingData(motionTrackerRow);
        let previous = null;
        let interval = motionTrackerModel.keyframeIntervalFrames(motionTrackerRow);
        let interpolation = Shotcut.KeyframesModel.SmoothInterpolation;
        filter.blockSignals = true;
        data.forEach(i => {
                let current = filter.getRect(rectProperty, frame);
                let x = 0;
                let y = 0;
                if (previous !== null) {
                    x = i.x - previous.x;
                    y = i.y - previous.y;
                }
                switch (operation) {
                case 'relativePos':
                    current.x += x;
                    current.y += y;
                    break;
                case 'offsetPos':
                    current.x -= x;
                    current.y -= y;
                    break;
                case 'absPos':
                    current.x = i.x + i.width / 2 - current.width / 2;
                    current.y = i.y + i.height / 2 - current.height / 2;
                    interpolation = Shotcut.KeyframesModel.LinearInterpolation;
                    break;
                case 'absSizePos':
                    current.x = i.x;
                    current.y = i.y;
                    current.width = i.width;
                    current.height = i.height;
                    interpolation = Shotcut.KeyframesModel.LinearInterpolation;
                    break;
                }
                previous = i;
                filter.set(rectProperty, current, frame, interpolation);
                frame += interval;
            });
        filter.blockSignals = false;
        parameters.reload();
    }

    keyframableParameters: ['radius', 'color']
    startValues: [0, Qt.rgba(0, 0, 0, 1)]
    middleValues: [0, Qt.rgba(0, 0, 0, 1)]
    endValues: [0, Qt.rgba(0, 0, 0, 1)]
    width: 400
    height: 180
    Component.onCompleted: {
        filter.blockSignals = true;
        filter.set(startValueRect, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(middleValueRect, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(endValueRect, Qt.rect(0, 0, profile.width, profile.height));
        if (filter.isNew) {
            // Set default parameter values
            filter.set('color', Qt.rgba(0, 0, 0, 1));
            filter.set('radius', 0);
            filter.set(rectProperty, '0%/0%:100%x100%');
            filter.savePreset(preset.parameters);
        } else {
            initSimpleKeyframes();
            filter.set(middleValueRect, filter.getRect(rectProperty, filter.animateIn + 1));
            if (filter.animateIn > 0) {
                filter.set(startValueRect, filter.getRect(rectProperty, 0));
            }
            if (filter.animateOut > 0) {
                filter.set(endValueRect, filter.getRect(rectProperty, filter.duration - 1));
            }
        }
        filter.blockSignals = false;
        setControls();
        setRectControls();
        if (filter.isNew)
            filter.set(rectProperty, filter.getRect(rectProperty));
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: [rectProperty, 'radius', 'color']
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filterRect = Qt.rect(0, 0, 0, 0);
                filter.resetProperty(rectProperty);
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                setControls();
                setRectControls();
                initSimpleKeyframes();
                filter.blockSignals = true;
                filter.set(middleValueRect, filter.getRect(rectProperty, filter.animateIn + 1));
                if (filter.animateIn > 0) {
                    filter.set(startValueRect, filter.getRect(rectProperty, 0));
                }
                if (filter.animateOut > 0) {
                    filter.set(endValueRect, filter.getRect(rectProperty, filter.duration - 1));
                }
                filter.blockSignals = false;
            }
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.DoubleSpinBox {
                id: rectX

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.x !== value) {
                        filterRect.x = value;
                        updateFilterRect(getPosition(), true);
                    }
                }
            }

            Label {
                text: ','
                Layout.minimumWidth: 20
                horizontalAlignment: Qt.AlignHCenter
            }

            Shotcut.DoubleSpinBox {
                id: rectY

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.y !== value) {
                        filterRect.y = value;
                        updateFilterRect(getPosition(), true);
                    }
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                rectX.value = rectY.value = 0;
                filterRect.x = filterRect.y = 0;
                updateFilterRect(getPosition(), true);
            }
        }

        Shotcut.KeyframesButton {
            id: positionKeyframesButton

            Layout.rowSpan: 2
            onToggled: {
                if (checked) {
                    filter.blockSignals = true;
                    filter.clearSimpleAnimation(rectProperty);
                    filter.blockSignals = false;
                    filter.set(rectProperty, filterRect, getPosition());
                } else {
                    filter.blockSignals = true;
                    filter.resetProperty(rectProperty);
                    filter.blockSignals = false;
                    filter.set(rectProperty, filterRect);
                }
                checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
            }
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.DoubleSpinBox {
                id: rectW

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.width !== value) {
                        filterRect.width = value;
                        updateFilterRect(getPosition(), true);
                    }
                }
            }

            Label {
                text: 'x'
                Layout.minimumWidth: 20
                horizontalAlignment: Qt.AlignHCenter
            }

            Shotcut.DoubleSpinBox {
                id: rectH

                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.height !== value) {
                        filterRect.height = value;
                        updateFilterRect(getPosition(), true);
                    }
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                rectW.value = profile.width;
                rectH.value = profile.height;
                filterRect.width = profile.width;
                filterRect.height = profile.height;
                updateFilterRect(getPosition(), true);
            }
        }

        Label {
            text: qsTr('Corner radius')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter('radius', value / 100, radiusKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: slider.value = 0
        }

        Shotcut.KeyframesButton {
            id: radiusKeyframesButton

            onToggled: {
                toggleKeyframes(checked, 'radius', slider.value / 100);
                setControls();
            }
        }

        Label {
            text: qsTr('Padding color')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.ColorPicker {
                id: colorSwatch

                property bool isReady: false

                alpha: true
                enabled: slider.enabled
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        filter.set('disable', 0);
                        updateFilter('color', Qt.color(value), colorKeyframesButton, getPosition());
                    }
                }
                onPickStarted: {
                    filter.set('disable', 1);
                }
                onPickCancelled: filter.set('disable', 0)
            }

            Shotcut.Button {
                text: qsTr('Transparent')
                onClicked: colorSwatch.value = Qt.rgba(0, 0, 0, 0)
            }
        }

        Shotcut.UndoButton {
            onClicked: colorSwatch.value = Qt.rgba(0, 0, 0, 1)
        }

        Shotcut.KeyframesButton {
            id: colorKeyframesButton
            onToggled: toggleKeyframes(checked, 'color', colorSwatch.value)
        }

        Item {
            width: 1
        }

        Shotcut.Button {
            Layout.columnSpan: parent.columns - 1
            text: motionTrackerDialog.title
            onClicked: motionTrackerDialog.show()
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Shotcut.MotionTrackerDialog {
        id: motionTrackerDialog
        onAccepted: (motionTrackerRow, operation, startFrame) => applyTracking(motionTrackerRow, operation, startFrame)
        onReset: if (filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0) {
            motionTrackerModel.undo(filter, rectProperty);
            filterRect = filter.getRect(rectProperty, getPosition());
            rectX.value = filterRect.x;
            rectY.value = filterRect.y;
            rectW.value = filterRect.width;
            rectH.value = filterRect.height;
            updateFilter(getPosition());
        }
    }

    Connections {
        function onChanged() {
            setRectControls();
        }

        function onInChanged() {
            updateParameters();
        }

        function onOutChanged() {
            updateParameters();
        }

        function onAnimateInChanged() {
            updateParameters();
        }

        function onAnimateOutChanged() {
            updateParameters();
        }

        function onPropertyChanged(name) {
            setRectControls();
            setControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setControls();
            setRectControls();
        }

        target: producer
    }
}
