/*
 * Copyright (c) 2019 Meltytech, LLC
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

import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0

Item {
    property string rectProperty: "rect"
    property rect filterRect: filter.getRect(rectProperty)
    property var defaultParameters: [rectProperty, 'color.1', 'color.2' ,'osc', 'frequency_low', 'frequency_high', 'threshold']
    property int _minFreqDelta: 100
    property bool _disableUpdate: false

    width: 350
    height: 425

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(rectProperty, '0/50%:50%x50%')
            filter.set('color.1', '#ffffffff')
            filter.set('color.2', '#00000000')
            filter.set('osc', '5')
            filter.set('frequency_low', '20')
            filter.set('frequency_high', '20000')
            filter.set('threshold', '-60')
            filter.savePreset(defaultParameters)
        }
        setControls()
    }

    function setFilter() {
        var x = parseFloat(rectX.text)
        var y = parseFloat(rectY.text)
        var w = parseFloat(rectW.text)
        var h = parseFloat(rectH.text)
        if (x !== filterRect.x ||
            y !== filterRect.y ||
            w !== filterRect.width ||
            h !== filterRect.height) {
            filterRect.x = x
            filterRect.y = y
            filterRect.width = w
            filterRect.height = h
            filter.set(rectProperty, '%L1%/%L2%:%L3%x%L4%'
                       .arg(x / profile.width * 100)
                       .arg(y / profile.height * 100)
                       .arg(w / profile.width * 100)
                       .arg(h / profile.height * 100))
        }
    }

    function setControls() {
        _disableUpdate = true
        fgColor.value = filter.get('color.1')
        bgColor.value = filter.get('color.2')
        oscSlider.value = filter.getDouble('osc')
        freqLowSlider.value = filter.getDouble('frequency_low')
        freqHighSlider.value = filter.getDouble('frequency_high')
        thresholdSlider.value = filter.getDouble('threshold')
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
        Preset {
            id: preset
            parameters: defaultParameters
            Layout.columnSpan: 4
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Foreground')
            Layout.alignment: Qt.AlignRight
        }
        ColorPicker {
            Layout.columnSpan: 4
            id: fgColor
            eyedropper: true
            alpha: true
            onValueChanged: filter.set('color.1', value)
        }

        Label {
            text: qsTr('Background')
            Layout.alignment: Qt.AlignRight
        }
        ColorPicker {
            Layout.columnSpan: 4
            id: bgColor
            eyedropper: true
            alpha: true
            onValueChanged: filter.set('color.2', value)
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            TextField {
                id: rectX
                text: filterRect.x
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
            Label { text: ',' }
            TextField {
                id: rectY
                text: filterRect.y
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            TextField {
                id: rectW
                text: filterRect.width
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
            Label { text: 'x' }
            TextField {
                id: rectH
                text: filterRect.height
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
        }

        Label {
            text: qsTr('Oscillation')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('Oscillation can be useful to make the light blink during long periods of sound.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: oscSlider
            minimumValue: 0
            maximumValue: 10
            decimals: 0
            suffix: ' Hz'
            onValueChanged: filter.set("osc", value)
        }
        UndoButton {
            onClicked: oscSlider.value = 5
        }

        Label {
            text: qsTr('Low Frequency')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The low end of the frequency range to be used to influence the light.') }
        }
        SliderSpinner {
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
        UndoButton {
            onClicked: freqLowSlider.value = 20
        }

        Label {
            text: qsTr('High Frequency')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The high end of the frequency range to be used to influence the light.') }
        }
        SliderSpinner {
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
        UndoButton {
            onClicked: freqHighSlider.value = 20000
        }

        Label {
            text: qsTr('Threshold')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The minimum amplitude of sound that must occur within the frequency range to cause the light to change.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: thresholdSlider
            minimumValue: -60
            maximumValue: 0
            decimals: 0
            suffix: ' dB'
            onValueChanged: filter.set("threshold", value)
        }
        UndoButton {
            onClicked: thresholdSlider.value = -60
        }
        
        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        onChanged: {
            var newValue = filter.getRect(rectProperty)
            if (filterRect !== newValue)
                filterRect = newValue
        }
    }
}