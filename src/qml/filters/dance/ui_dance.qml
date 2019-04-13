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
    property var defaultParameters: ['osc', 'initial_zoom', 'zoom', 'up', 'down', 'left', 'right', 'clockwise', 'counterclockwise', 'frequency_low', 'frequency_high', 'threshold']
    property int _minFreqDelta: 100
    property bool _disableUpdate: false

    width: 350
    height: 425

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('osc', '5')
            filter.set('initial_zoom', '100')
            filter.set('zoom', '10')
            filter.set('up', '0')
            filter.set('down', '0')
            filter.set('left', '0')
            filter.set('right', '0')
            filter.set('clockwise', '0')
            filter.set('counterclockwise', '0')
            filter.set('frequency_low', '20')
            filter.set('frequency_high', '20000')
            filter.set('threshold', '-30')
            filter.savePreset(defaultParameters)
        }
        setControls()
    }

    function setControls() {
        _disableUpdate = true
        oscSlider.value = filter.getDouble('osc')
        initialZoomSlider.value = filter.getDouble('initial_zoom')
        zoomSlider.value = filter.getDouble('zoom')
        upSlider.value = filter.getDouble('up')
        downSlider.value = filter.getDouble('down')
        leftSlider.value = filter.getDouble('left')
        rightSlider.value = filter.getDouble('right')
        clockwiseSlider.value = filter.getDouble('clockwise')
        counterclockwiseSlider.value = filter.getDouble('counterclockwise')
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
            text: qsTr('Initial Zoom')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The amount to zoom the image before any motion occurs.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: initialZoomSlider
            minimumValue: 0
            maximumValue: 200
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("initial_zoom", value)
        }
        UndoButton {
            onClicked: initialZoomSlider.value = 10
        }

        Label {
            text: qsTr('Oscillation')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('Oscillation can be useful to make the image move back and forth during long periods of sound.') }
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
            text: qsTr('Zoom')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The amount that the audio affects the zoom of the image.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: zoomSlider
            minimumValue: -100
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("zoom", value)
        }
        UndoButton {
            onClicked: zoomSlider.value = 50
        }

        Label {
            text: qsTr('Up')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The amount that the audio affects the upward offset of the image.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: upSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("up", value)
        }
        UndoButton {
            onClicked: upSlider.value = 0
        }

        Label {
            text: qsTr('Down')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The amount that the audio affects the downward offset of the image.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: downSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("down", value)
        }
        UndoButton {
            onClicked: downSlider.value = 0
        }

        Label {
            text: qsTr('Left')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The amount that the audio affects the left offset of the image.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: leftSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("left", value)
        }
        UndoButton {
            onClicked: leftSlider.value = 0
        }

        Label {
            text: qsTr('Right')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The amount that the audio affects the right offset of the image.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: rightSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("right", value)
        }
        UndoButton {
            onClicked: rightSlider.value = 0
        }

        Label {
            text: qsTr('Clockwise')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The amount that the audio affects the clockwise rotation of the image.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: clockwiseSlider
            minimumValue: 0
            maximumValue: 360
            decimals: 0
            suffix: qsTr(' deg')
            onValueChanged: filter.set("clockwise", value)
        }
        UndoButton {
            onClicked: clockwiseSlider.value = 0
        }

        Label {
            text: qsTr('Counterclockwise')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The amount that the audio affects the counterclockwise rotation of the image.') }
        }
        SliderSpinner {
            Layout.columnSpan: 3
            id: counterclockwiseSlider
            minimumValue: 0
            maximumValue: 360
            decimals: 0
            suffix: qsTr(' deg')
            onValueChanged: filter.set("counterclockwise", value)
        }
        UndoButton {
            onClicked: counterclockwiseSlider.value = 0
        }

        Label {
            text: qsTr('Low Frequency')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('The low end of the frequency range to be used to influence the image motion.') }
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
            ToolTip { text: qsTr('The high end of the frequency range to be used to influence the image motion.') }
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
            ToolTip { text: qsTr('The minimum amplitude of sound that must occur within the frequency range to cause the image to move.') }
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
            onClicked: thresholdSlider.value = -30
        }
        
        Item { Layout.fillHeight: true }
    }
}