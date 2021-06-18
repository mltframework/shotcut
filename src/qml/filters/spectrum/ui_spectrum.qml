/*
 * Copyright (c) 2017-2021 Meltytech, LLC
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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    property string rectProperty: "rect"
    property rect filterRect: filter.getRect(rectProperty)
    property var defaultParameters: [rectProperty, 'type', 'color.1', 'color.2', 'color.3', 'color.4', 'color.5', 'color.6', 'color.7', 'color.8', 'color.9', 'color.10', 'bgcolor', 'thickness', 'fill', 'mirror', 'reverse', 'tension', 'bands', 'frequency_low', 'frequency_high', 'threshold']

    property int _minFreqDelta: 1000
    property bool _disableUpdate: true

    width: 350
    height: 425

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(rectProperty, '0/50%:50%x50%')
            filter.set('type', 'line')
            filter.set('color.1', '#ffffffff')
            filter.set('bgcolor', '#00ffffff')
            filter.set('thickness', '1')
            filter.set('fill', '0')
            filter.set('mirror', '0')
            filter.set('reverse', '0')
            filter.set('tension', '0.4')
            filter.set('bands', '31')
            filter.set('frequency_low', '20')
            filter.set('frequency_high', '20000')
            filter.set('threshold', '-60')
            filter.savePreset(defaultParameters)
        }
        setControls()
    }

    function setFilter() {
        var x = rectX.value
        var y = rectY.value
        var w = rectW.value
        var h = rectH.value
        if (x !== filterRect.x ||
            y !== filterRect.y ||
            w !== filterRect.width ||
            h !== filterRect.height) {
            filterRect.x = x
            filterRect.y = y
            filterRect.width = w
            filterRect.height = h
            filter.set(rectProperty, filterRect)
        }
    }

    function setControls() {
        _disableUpdate = true
        fgGradient.colors = filter.getGradient('color')
        bgColor.value = filter.get('bgcolor')
        thicknessSlider.value = filter.getDouble('thickness')
        fillCheckbox.checked = filter.get('fill') == 1
        mirrorCheckbox.checked = filter.get('mirror') == 1
        reverseCheckbox.checked = filter.get('reverse') == 1
        tensionSlider.value = filter.getDouble('tension')
        bandsSlider.value = filter.getDouble('bands')
        freqLowSlider.value = filter.getDouble('frequency_low')
        freqHighSlider.value = filter.getDouble('frequency_high')
        thresholdSlider.value = filter.getDouble('threshold')
        rectX.value = filterRect.x
        rectY.value = filterRect.y
        rectW.value = filterRect.width
        rectH.value = filterRect.height
        _disableUpdate = false
    }

    GridLayout {
        columns: 5
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            id: preset
            parameters: defaultParameters
            Layout.columnSpan: 4
            onPresetSelected: setControls()
            onBeforePresetLoaded: {
                // Clear all gradient colors before loading the new values
                filter.setGradient('color', [])
            }
        }

        Label {
            text: qsTr('Type')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            Layout.columnSpan: 4
            id: typeCombo
            model: [qsTr('Line'), qsTr('Bar')]
            property var values: ['line', 'bar']
            function valueToIndex() {
                var w = filter.get('type')
                for (var i = 0; i < values.length; ++i)
                    if (values[i] === w) break;
                if (i === values.length) i = 0;
                return i;
            }
            onActivated: filter.set('type', values[index])
        }

        Label {
            text: qsTr('Spectrum Color')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.GradientControl {
            Layout.columnSpan: 4
            id: fgGradient
            onGradientChanged: {
                 if (_disableUpdate) return
                 filter.setGradient('color', colors)
            }
        }

        Label {
            text: qsTr('Background Color')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ColorPicker {
            Layout.columnSpan: 4
            id: bgColor
            eyedropper: true
            alpha: true
            onValueChanged: filter.set('bgcolor', value)
        }

        Label {
            text: qsTr('Thickness')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            Layout.columnSpan: 3
            id: thicknessSlider
            minimumValue: 0
            maximumValue: 20
            decimals: 0
            suffix: ' px'
            onValueChanged: filter.set("thickness", value)
        }
        Shotcut.UndoButton {
            onClicked: thicknessSlider.value = 1
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            Shotcut.DoubleSpinBox {
                id: rectX
                value: filterRect.x
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.x !== value) setFilter()
            }
            Label { text: ','; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
            Shotcut.DoubleSpinBox {
                id: rectY
                value: filterRect.y
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.y !== value) setFilter()
            }
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            Shotcut.DoubleSpinBox {
                id: rectW
                value: filterRect.width
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.width !== value) setFilter()
            }
            Label { text: 'x'; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
            Shotcut.DoubleSpinBox {
                id: rectH
                value: filterRect.height
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.height !== value) setFilter()
            }
        }

        Label {
            text: qsTr('Fill')
            Layout.alignment: Qt.AlignRight
        }
        CheckBox {
            Layout.columnSpan: 4
            id: fillCheckbox
            text: qsTr('Fill the area under the spectrum.')
            onClicked: filter.set('fill', checked ? 1 : 0)
        }

        Label {
            text: qsTr('Mirror')
            Layout.alignment: Qt.AlignRight
        }
        CheckBox {
            Layout.columnSpan: 4
            id: mirrorCheckbox
            text: qsTr('Mirror the spectrum.')
            onClicked: filter.set('mirror', checked ? 1 : 0)
        }

        Label {
            text: qsTr('Reverse')
            Layout.alignment: Qt.AlignRight
        }
        CheckBox {
            Layout.columnSpan: 4
            id: reverseCheckbox
            text: qsTr('Reverse the spectrum.')
            onClicked: filter.set('reverse', checked ? 1 : 0)
        }

        Label {
            text: qsTr('Tension')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            Layout.columnSpan: 3
            id: tensionSlider
            minimumValue: 0.0
            maximumValue: 1.0
            decimals: 1
            onValueChanged: filter.set("tension", value)
        }
        Shotcut.UndoButton {
            onClicked: tensionSlider.value = 0.4
        }

        Label {
            text: qsTr('Bands')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            Layout.columnSpan: 3
            id: bandsSlider
            minimumValue: 5
            maximumValue: 100
            decimals: 0
            onValueChanged: filter.set("bands", value)
        }
        Shotcut.UndoButton {
            onClicked: bandsSlider.value = 31
        }

        Label {
            text: qsTr('Low Frequency')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('The low end of the frequency range of the spectrum.') }
        }
        Shotcut.SliderSpinner {
            Layout.columnSpan: 3
            id: freqLowSlider
            minimumValue: 20
            maximumValue: 20000 - _minFreqDelta
            decimals: 0
            suffix: ' Hz'
            onValueChanged: {
                filter.set("frequency_low", value)
                if (!_disableUpdate && (value + _minFreqDelta) > freqHighSlider.value) {
                    freqHighSlider.value = value + _minFreqDelta
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: freqLowSlider.value = 20
        }

        Label {
            text: qsTr('High Frequency')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('The high end of the frequency range of the spectrum.') }
        }
        Shotcut.SliderSpinner {
            Layout.columnSpan: 3
            id: freqHighSlider
            minimumValue: 20 + _minFreqDelta
            maximumValue: 20000
            decimals: 0
            suffix: ' Hz'
            onValueChanged: {
                filter.set("frequency_high", value)
                if (!_disableUpdate && (value - _minFreqDelta) < freqLowSlider.value) {
                    freqLowSlider.value = value - _minFreqDelta
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: freqHighSlider.value = 20000
        }

        Label {
            text: qsTr('Threshold')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            Layout.columnSpan: 3
            id: thresholdSlider
            minimumValue: -60
            maximumValue: 0
            decimals: 0
            suffix: ' dB'
            onValueChanged: filter.set("threshold", value)
        }
        Shotcut.UndoButton {
            onClicked: thresholdSlider.value = -60
        }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        onChanged: {
            var newValue = filter.getRect(rectProperty)
            if (filterRect !== newValue) {
                filterRect = newValue
                setControls()
            }
        }
    }
}
