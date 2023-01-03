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
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

Item {
    property var defaultParameters: ['osc', 'initial_zoom', 'zoom', 'up', 'down', 'left', 'right', 'clockwise', 'counterclockwise', 'frequency_low', 'frequency_high', 'threshold']
    property int _minFreqDelta: 100
    property bool _disableUpdate: false

    function setControls() {
        _disableUpdate = true;
        oscSlider.value = filter.getDouble('osc');
        initialZoomSlider.value = filter.getDouble('initial_zoom');
        zoomSlider.value = filter.getDouble('zoom');
        upSlider.value = filter.getDouble('up');
        downSlider.value = filter.getDouble('down');
        leftSlider.value = filter.getDouble('left');
        rightSlider.value = filter.getDouble('right');
        clockwiseSlider.value = filter.getDouble('clockwise');
        counterclockwiseSlider.value = filter.getDouble('counterclockwise');
        freqLowSlider.value = filter.getDouble('frequency_low');
        freqHighSlider.value = filter.getDouble('frequency_high');
        thresholdSlider.value = filter.getDouble('threshold');
        _disableUpdate = false;
    }

    width: 350
    height: 350
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('osc', '5');
            filter.set('initial_zoom', '100');
            filter.set('zoom', '10');
            filter.set('up', '0');
            filter.set('down', '0');
            filter.set('left', '0');
            filter.set('right', '0');
            filter.set('clockwise', '0');
            filter.set('counterclockwise', '0');
            filter.set('frequency_low', '20');
            filter.set('frequency_high', '20000');
            filter.set('threshold', '-30');
            filter.savePreset(defaultParameters);
        }
        setControls();
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
        }

        Label {
            text: qsTr('Initial Zoom')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount to zoom the image before any motion occurs.')
            }
        }

        Shotcut.SliderSpinner {
            id: initialZoomSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 200
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("initial_zoom", value)
        }

        Shotcut.UndoButton {
            onClicked: initialZoomSlider.value = 100
        }

        Label {
            text: qsTr('Oscillation')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Oscillation can be useful to make the image move back and forth during long periods of sound.')
            }
        }

        Shotcut.SliderSpinner {
            id: oscSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 10
            decimals: 0
            suffix: ' Hz'
            onValueChanged: filter.set("osc", value)
        }

        Shotcut.UndoButton {
            onClicked: oscSlider.value = 5
        }

        Label {
            text: qsTr('Zoom')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount that the audio affects the zoom of the image.')
            }
        }

        Shotcut.SliderSpinner {
            id: zoomSlider

            Layout.columnSpan: 3
            minimumValue: -100
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("zoom", value)
        }

        Shotcut.UndoButton {
            onClicked: zoomSlider.value = 10
        }

        Label {
            text: qsTr('Up')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount that the audio affects the upward offset of the image.')
            }
        }

        Shotcut.SliderSpinner {
            id: upSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("up", value)
        }

        Shotcut.UndoButton {
            onClicked: upSlider.value = 0
        }

        Label {
            text: qsTr('Down')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount that the audio affects the downward offset of the image.')
            }
        }

        Shotcut.SliderSpinner {
            id: downSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("down", value)
        }

        Shotcut.UndoButton {
            onClicked: downSlider.value = 0
        }

        Label {
            text: qsTr('Left')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount that the audio affects the left offset of the image.')
            }
        }

        Shotcut.SliderSpinner {
            id: leftSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("left", value)
        }

        Shotcut.UndoButton {
            onClicked: leftSlider.value = 0
        }

        Label {
            text: qsTr('Right')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount that the audio affects the right offset of the image.')
            }
        }

        Shotcut.SliderSpinner {
            id: rightSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            suffix: ' %'
            onValueChanged: filter.set("right", value)
        }

        Shotcut.UndoButton {
            onClicked: rightSlider.value = 0
        }

        Label {
            text: qsTr('Clockwise')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount that the audio affects the clockwise rotation of the image.')
            }
        }

        Shotcut.SliderSpinner {
            id: clockwiseSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 360
            decimals: 0
            suffix: qsTr(' deg')
            onValueChanged: filter.set("clockwise", value)
        }

        Shotcut.UndoButton {
            onClicked: clockwiseSlider.value = 0
        }

        Label {
            text: qsTr('Counterclockwise')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount that the audio affects the counterclockwise rotation of the image.')
            }
        }

        Shotcut.SliderSpinner {
            id: counterclockwiseSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 360
            decimals: 0
            suffix: qsTr(' deg')
            onValueChanged: filter.set("counterclockwise", value)
        }

        Shotcut.UndoButton {
            onClicked: counterclockwiseSlider.value = 0
        }

        Label {
            text: qsTr('Low Frequency')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The low end of the frequency range to be used to influence the image motion.')
            }
        }

        Shotcut.SliderSpinner {
            id: freqLowSlider

            Layout.columnSpan: 3
            minimumValue: 20
            maximumValue: 20000 - _minFreqDelta
            decimals: 0
            suffix: ' Hz'
            onValueChanged: {
                filter.set("frequency_low", value);
                if (!_disableUpdate && (value + _minFreqDelta) > freqHighSlider.value)
                    freqHighSlider.value = value + _minFreqDelta;
            }
        }

        Shotcut.UndoButton {
            onClicked: freqLowSlider.value = 20
        }

        Label {
            text: qsTr('High Frequency')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The high end of the frequency range to be used to influence the image motion.')
            }
        }

        Shotcut.SliderSpinner {
            id: freqHighSlider

            Layout.columnSpan: 3
            minimumValue: 20 + _minFreqDelta
            maximumValue: 20000
            decimals: 0
            suffix: ' Hz'
            onValueChanged: {
                filter.set("frequency_high", value);
                if (!_disableUpdate && (value - _minFreqDelta) < freqLowSlider.value)
                    freqLowSlider.value = value - _minFreqDelta;
            }
        }

        Shotcut.UndoButton {
            onClicked: freqHighSlider.value = 20000
        }

        Label {
            text: qsTr('Threshold')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The minimum amplitude of sound that must occur within the frequency range to cause the image to move.')
            }
        }

        Shotcut.SliderSpinner {
            id: thresholdSlider

            Layout.columnSpan: 3
            minimumValue: -60
            maximumValue: 0
            decimals: 0
            suffix: ' dB'
            onValueChanged: filter.set("threshold", value)
        }

        Shotcut.UndoButton {
            onClicked: thresholdSlider.value = -30
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
