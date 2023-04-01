/*
 * Copyright (c) 2017-2023 Meltytech, LLC
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

Item {
    property string paramShape: 'filter.0'
    property string paramHorizontal: 'filter.1'
    property string paramVertical: 'filter.2'
    property string paramWidth: 'filter.3'
    property string paramHeight: 'filter.4'
    property string paramRotation: 'filter.5'
    property string paramSoftness: 'filter.6'
    property string paramOperation: 'filter.9'
    property string rectProperty: 'shotcut:rect'
    property var defaultParameters: [paramHorizontal, paramVertical, paramWidth, paramHeight, paramShape, paramRotation, paramSoftness, paramOperation, rectProperty]
    property bool blockUpdate: true
    property var startValues: [0.5, 0.5, 0.1, 0.1, 0, 0.5]
    property var middleValues: [0.5, 0.5, 0.1, 0.1, 0, 0.5]
    property var endValues: [0.5, 0.5, 0.1, 0.1, 0, 0.5]
    property string startValueRect: '_shotcut:startValue'
    property string middleValueRect: '_shotcut:middleValue'
    property string endValueRect: '_shotcut:endValue'
    property rect filterRect

    function initSimpleAnimation() {
        middleValues = [filter.getDouble(paramHorizontal, filter.animateIn), filter.getDouble(paramVertical, filter.animateIn), filter.getDouble(paramWidth, filter.animateIn), filter.getDouble(paramHeight, filter.animateIn), 0, filter.getDouble(paramRotation, filter.animateIn)];
        filter.set(middleValueRect, filter.getRect(rectProperty, filter.animateIn + 1));
        if (filter.animateIn > 0) {
            startValues = [filter.getDouble(paramHorizontal, 0), filter.getDouble(paramVertical, 0), filter.getDouble(paramWidth, 0), filter.getDouble(paramHeight, 0), 0, filter.getDouble(paramRotation, 0)];
            filter.set(startValueRect, filter.getRect(rectProperty, 0));
        }
        if (filter.animateOut > 0) {
            endValues = [filter.getDouble(paramHorizontal, filter.duration - 1), filter.getDouble(paramVertical, filter.duration - 1), filter.getDouble(paramWidth, filter.duration - 1), filter.getDouble(paramHeight, filter.duration - 1), 0, filter.getDouble(paramRotation, filter.duration - 1)];
            filter.set(endValueRect, filter.getRect(rectProperty, filter.duration - 1));
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        motionTrackerCombo.currentIndex = motionTrackerCombo.indexOfValue(filter.get(motionTrackerModel.nameProperty));
        trackingOperationCombo.currentIndex = trackingOperationCombo.indexOfValue(filter.get(motionTrackerModel.operationProperty));
        blockUpdate = false;
        operationCombo.currentIndex = Math.round(filter.getDouble(paramOperation) * 4);
        shapeCombo.currentIndex = Math.round(filter.getDouble(paramShape) * 3);
        softnessSlider.value = filter.getDouble(paramSoftness) * 100;
    }

    function setKeyframedControls() {
        let position = getPosition();
        let newValue = filter.getRect(rectProperty, position);
        if (filterRect !== newValue) {
            filterRect = newValue;
            rectX.value = filterRect.x.toFixed();
            rectY.value = filterRect.y.toFixed();
            rectW.value = filterRect.width.toFixed();
            rectH.value = filterRect.height.toFixed();
        }
        let enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
        rectX.enabled = enabled;
        rectY.enabled = enabled;
        rectW.enabled = enabled;
        rectH.enabled = enabled;
        positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = true;
        rotationSlider.updateFromFilter();
        rotationKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(paramRotation) > 0;
        blockUpdate = false;
        rotationSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilterParam(parameter, value, position, button) {
        if (blockUpdate)
            return;
        let index = defaultParameters.indexOf(parameter);
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValues[index] = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValues[index] = value;
            else
                middleValues[index] = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(parameter);
            button.checked = false;
            if (filter.animateIn > 0) {
                filter.set(parameter, startValues[index], 0);
                filter.set(parameter, middleValues[index], filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set(parameter, middleValues[index], filter.duration - filter.animateOut);
                filter.set(parameter, endValues[index], filter.duration - 1);
            }
        } else if (!button.checked) {
            filter.resetProperty(parameter);
            filter.set(parameter, middleValues[index]);
        } else if (position !== null) {
            filter.set(parameter, value, position);
        }
    }

    function resetRectProperties() {
        filter.resetProperty(rectProperty);
        filter.resetProperty(paramHorizontal);
        filter.resetProperty(paramVertical);
        filter.resetProperty(paramWidth);
        filter.resetProperty(paramHeight);
    }

    function setRectProperties(rect, position) {
        if (position === null)
            position = -1;
        filter.set(rectProperty, rect, position);
        rect.width /= profile.width * 2;
        rect.height /= profile.height * 2;
        rect.x = rect.x / profile.width + rect.width;
        rect.y = rect.y / profile.height + rect.height;
        filter.blockSignals = true;
        filter.set(paramHorizontal, rect.x, position);
        filter.set(paramVertical, rect.y, position);
        filter.set(paramWidth, rect.width, position);
        filter.set(paramHeight, rect.height, position);
        filter.blockSignals = false;
    }

    function updateFilterRect(position) {
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
            resetRectProperties();
            positionKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                setRectProperties(filter.getRect(startValueRect), 0);
                setRectProperties(filter.getRect(middleValueRect), filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                setRectProperties(filter.getRect(middleValueRect), filter.duration - filter.animateOut);
                setRectProperties(filter.getRect(endValueRect), filter.duration - 1);
            }
        } else if (!positionKeyframesButton.checked) {
            resetRectProperties();
            setRectProperties(filter.getRect(middleValueRect));
        } else if (position !== null) {
            setRectProperties(filterRect, position);
        }
    }

    function updateFilter() {
        updateFilterParam(paramRotation, rotationSlider.filterValue(), null, rotationKeyframesButton);
        updateFilterRect(null);
    }

    function onKeyframesButtonClicked(checked, parameter, value) {
        if (checked) {
            blockUpdate = true;
            horizontalSlider.enabled = verticalSlider.enabled = widthSlider.enabled = heightSlider.enabled = true;
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                filter.resetProperty(paramHorizontal);
                filter.resetProperty(paramVertical);
                filter.resetProperty(paramWidth);
                filter.resetProperty(paramHeight);
                filter.resetProperty(paramRotation);
                filter.animateIn = filter.animateOut = 0;
            } else {
                filter.clearSimpleAnimation(parameter);
            }
            blockUpdate = false;
            filter.set(parameter, value, getPosition());
        } else {
            filter.resetProperty(parameter);
            filter.set(parameter, value);
        }
    }

    function updateSimpleAnimation() {
        setKeyframedControls();
        updateFilter();
    }

    function applyTracking() {
        if (motionTrackerCombo.currentIndex > 0 && trackingOperationCombo.currentIndex > 0) {
            const data = motionTrackerModel.trackingData(motionTrackerCombo.currentIndex);
            let previous = null;
            let frame = currentRadioButton.checked ? getPosition() : 0;
            let interval = motionTrackerModel.keyframeIntervalFrames(motionTrackerCombo.currentIndex);
            let interpolation = Shotcut.KeyframesModel.SmoothInterpolation;
            filter.blockSignals = true;

            // reset
            if (data.length > 0) {
                let params = [paramHorizontal, paramVertical, paramWidth, paramHeight];
                // Use a shotcut property to backup current values
                if (filter.get('shotcut:backup.' + paramHorizontal).length === 0) {
                    params.forEach(param => {
                            filter.set('shotcut:backup.' + param, filter.getDouble(param));
                        });
                    filter.set('shotcut:backup.rect', filter.getRect(rectProperty));
                } else {
                    params.forEach(param => {
                            filter.resetProperty(param);
                            filter.set(param, filter.getDouble('shotcut:backup.' + param));
                        });
                    filter.set(rectProperty, filter.getRect('shotcut:backup.rect'));
                }
            }
            filterRect = filter.getRect(rectProperty, frame);
            data.forEach(i => {
                    let current = Qt.rect(filter.getDouble(paramHorizontal, frame), filter.getDouble(paramVertical, frame), filter.getDouble(paramWidth, frame), filter.getDouble(paramHeight, frame));
                    let x = 0;
                    let y = 0;
                    if (previous !== null) {
                        x = i.x - previous.x;
                        y = i.y - previous.y;
                    }
                    switch (trackingOperationCombo.currentValue) {
                    case 'relativePos':
                        current.x += x / profile.width;
                        current.y += y / profile.height;
                        filter.set(paramHorizontal, current.x, frame, interpolation);
                        filter.set(paramVertical, current.y, frame, interpolation);
                        filterRect.x += x;
                        filterRect.y += y;
                        filter.set(rectProperty, filterRect, frame, interpolation);
                        break;
                    case 'offsetPos':
                        current.x -= x / profile.width;
                        current.y -= y / profile.height;
                        filter.set(paramHorizontal, current.x, frame, interpolation);
                        filter.set(paramVertical, current.y, frame, interpolation);
                        filterRect.x -= x;
                        filterRect.y -= y;
                        filter.set(rectProperty, filterRect, frame, interpolation);
                        break;
                    case 'absPos':
                        current.x = (i.x + i.width / 2) / profile.width;
                        current.y = (i.y + i.height / 2) / profile.height;
                        interpolation = Shotcut.KeyframesModel.LinearInterpolation;
                        filter.set(paramHorizontal, current.x, frame, interpolation);
                        filter.set(paramVertical, current.y, frame, interpolation);
                        filterRect.x = i.x + i.width / 2 - filterRect.width / 2;
                        filterRect.y = i.y + i.height / 2 - filterRect.height / 2;
                        filter.set(rectProperty, filterRect, frame, interpolation);
                        break;
                    case 'absSizePos':
                        current.x = (i.x + i.width / 2) / profile.width;
                        current.y = (i.y + i.height / 2) / profile.height;
                        current.width = i.width / profile.width / 2;
                        current.height = i.height / profile.height / 2;
                        interpolation = Shotcut.KeyframesModel.LinearInterpolation;
                        filter.set(paramHorizontal, current.x, frame, interpolation);
                        filter.set(paramVertical, current.y, frame, interpolation);
                        filter.set(paramWidth, current.width, frame, interpolation);
                        filter.set(paramHeight, current.height, frame, interpolation);
                        filterRect = i;
                        filter.set(rectProperty, filterRect, frame, interpolation);
                        break;
                    }
                    previous = i;
                    frame += interval;
                });
            filter.blockSignals = false;
            filter.changed();
            filter.animateInChanged();
            filter.animateOutChanged();
        }
    }

    width: 350
    height: 350
    Component.onCompleted: {
        filter.blockSignals = true;
        let rect = Qt.rect(0.4 * profile.width, 0.4 * profile.height, 0.2 * profile.width, 0.2 * profile.height);
        filter.set(startValueRect, rect);
        filter.set(middleValueRect, rect);
        filter.set(endValueRect, rect);
        if (filter.isNew) {
            // Set default parameter values
            filter.set('filter', 'frei0r.alphaspot');
            filter.set(paramOperation, 0);
            filter.set(paramShape, 0);
            filter.set(rectProperty, '40%/40%:20%x20%');
            filter.set(paramHorizontal, 0.5);
            filter.set(paramVertical, 0.5);
            filter.set(paramWidth, 0.1);
            filter.set(paramHeight, 0.1);
            filter.set(paramRotation, 0.5);
            filter.set(paramSoftness, 0.2);
            filter.savePreset(defaultParameters);
        } else {
            initSimpleAnimation();
        }
        filter.blockSignals = false;
        setControls();
        setKeyframedControls();
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
            Layout.columnSpan: 3
            parameters: defaultParameters
            onBeforePresetLoaded: {
                filterRect = Qt.rect(0, 0, 0, 0);
                resetRectProperties();
                filter.resetProperty(paramRotation);
            }
            onPresetSelected: {
                setControls();
                setKeyframedControls();
                filter.blockSignals = true;
                initSimpleAnimation();
                filter.blockSignals = false;
            }
        }

        Label {
            text: qsTr('Operation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: operationCombo

            implicitWidth: 180
            model: [qsTr('Overwrite'), qsTr('Maximum'), qsTr('Minimum'), qsTr('Add'), qsTr('Subtract')]
            onActivated: filter.set(paramOperation, currentIndex / 4)
        }

        Shotcut.UndoButton {
            Layout.columnSpan: 2
            onClicked: {
                filter.set(paramOperation, 0);
                operationCombo.currentIndex = 0;
            }
        }

        Label {
            text: qsTr('Shape')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: shapeCombo

            implicitWidth: 180
            model: [qsTr('Rectangle'), qsTr('Ellipse'), qsTr('Triangle'), qsTr('Diamond')]
            onActivated: filter.set(paramShape, currentIndex / 3)
        }

        Shotcut.UndoButton {
            Layout.columnSpan: 2
            onClicked: {
                filter.set(paramShape, 0);
                shapeCombo.currentIndex = 0;
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
                        updateFilterRect(getPosition());
                    }
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                rectX.value = 0.4 * profile.width;
                rectY.value = 0.4 * profile.height;
                filterRect.x = 0.4 * profile.width;
                filterRect.y = 0.4 * profile.height;
                updateFilterRect(getPosition());
            }
        }

        Shotcut.KeyframesButton {
            id: positionKeyframesButton

            Layout.rowSpan: 2
            onToggled: {
                filterRect = filter.getRect(rectProperty, getPosition());
                if (checked) {
                    filter.blockSignals = true;
                    filter.clearSimpleAnimation(rectProperty);
                    filter.clearSimpleAnimation(paramHorizontal);
                    filter.clearSimpleAnimation(paramVertical);
                    filter.clearSimpleAnimation(paramWidth);
                    filter.clearSimpleAnimation(paramHeight);
                    filter.blockSignals = false;
                    updateFilterRect(getPosition());
                } else {
                    updateFilterRect(null);
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
                        updateFilterRect(getPosition());
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
                        updateFilterRect(getPosition());
                    }
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                rectW.value = 0.2 * profile.width;
                rectH.value = 0.2 * profile.height;
                filterRect.width = 0.2 * profile.width;
                filterRect.height = 0.2 * profile.height;
                updateFilterRect(getPosition());
            }
        }

        Label {
            text: qsTr('Rotation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: rotationSlider

            function updateFromFilter() {
                value = (filter.getDouble(paramRotation, getPosition()) - 0.5) * 360;
            }

            function filterValue() {
                return 0.5 + value / 360;
            }

            minimumValue: -179.9
            maximumValue: 179.9
            decimals: 1
            spinnerWidth: 110
            suffix: qsTr(' deg', 'degrees')
            onValueChanged: updateFilterParam(paramRotation, filterValue(), getPosition(), rotationKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: rotationSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: rotationKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, paramRotation, rotationSlider.filterValue())
        }

        Label {
            id: softnessLabel

            text: qsTr('Softness')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: softnessSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            onValueChanged: filter.set(paramSoftness, value / 100)
        }

        Shotcut.UndoButton {
            Layout.columnSpan: 2
            onClicked: softnessSlider.value = 20
        }

        Label {
            text: qsTr('Motion tracker')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: motionTrackerCombo

            implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted
            textRole: 'display'
            valueRole: 'display'
            currentIndex: 0
            model: motionTrackerModel

            onActivated: {
                if (currentIndex > 0) {
                    enabled = false;
                    filter.set(motionTrackerModel.nameProperty, currentText);
                    if (trackingOperationCombo.currentIndex > 0) {
                        applyTracking();
                    }
                    enabled = true;
                }
            }
        }

        Shotcut.UndoButton {
            Layout.rowSpan: 2
            onClicked: {
                filter.blockSignals = true;
                filter.resetProperty(motionTrackerModel.nameProperty);
                filter.resetProperty(motionTrackerModel.operationProperty);
                let params = [paramHorizontal, paramVertical, paramWidth, paramHeight];
                params.forEach(param => {
                        filter.resetProperty(param);
                        filter.set(param, filter.getDouble('shotcut:backup.' + param));
                        filter.resetProperty('shotcut:backup.' + param);
                    });
                filter.resetProperty(rectProperty);
                filter.set(rectProperty, filter.getRect('shotcut:backup.rect'));
                filter.resetProperty('shotcut:backup.rect');
                motionTrackerCombo.currentIndex = 0;
                trackingOperationCombo.currentIndex = 0;
                startRadioButton.checked = true;
                filter.blockSignals = false;
                filter.changed();
                filter.animateInChanged();
                filter.animateOutChanged();
            }
        }

        Item {
            Layout.rowSpan: 2
            width: 1
        }

        Label {
            text: qsTr('Tracker adjusts')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.ComboBox {
                id: trackingOperationCombo

                implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted
                textRole: 'text'
                valueRole: 'value'
                model: [{
                        "text": '',
                        "value": ''
                    }, {
                        "text": qsTr('Relative Position'),
                        "value": 'relativePos'
                    }, {
                        "text": qsTr('Offset Position'),
                        "value": 'offsetPos'
                    }, {
                        "text": qsTr('Absolute Position'),
                        "value": 'absPos'
                    }, {
                        "text": qsTr('Size And Position'),
                        "value": 'absSizePos'
                    },]

                onActivated: {
                    if (currentIndex > 0) {
                        enabled = false;
                        filter.set(motionTrackerModel.operationProperty, currentValue);
                        if (trackingOperationCombo.currentIndex > 0) {
                            applyTracking();
                        }
                        enabled = true;
                    }
                }
            }

            RadioButton {
                id: startRadioButton
                text: qsTr('From start')
                checked: true
                onToggled: {
                    if (motionTrackerCombo.currentIndex > 0 && trackingOperationCombo.currentIndex > 0) {
                        applyTracking();
                    }
                }
            }
            RadioButton {
                id: currentRadioButton
                text: qsTr('Current position')
                onToggled: {
                    if (motionTrackerCombo.currentIndex > 0 && trackingOperationCombo.currentIndex > 0) {
                        applyTracking();
                    }
                }
            }
        }

        Shotcut.TipBox {
            Layout.columnSpan: parent.columns
            Layout.fillWidth: true
            Layout.margins: 10
            text: qsTr('Tip: Mask other video filters by adding filters after this one followed by <b>Mask: Apply</b>')
        }

        Label {
            Layout.fillHeight: true
        }
    }

    Connections {
        function onChanged() {
            setKeyframedControls();
        }

        function onInChanged() {
            updateSimpleAnimation();
        }

        function onOutChanged() {
            updateSimpleAnimation();
        }

        function onAnimateInChanged() {
            updateSimpleAnimation();
        }

        function onAnimateOutChanged() {
            updateSimpleAnimation();
        }

        function onPropertyChanged(name) {
            updateSimpleAnimation();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setKeyframedControls();
        }

        target: producer
    }
}
