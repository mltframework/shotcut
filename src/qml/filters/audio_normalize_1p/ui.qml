/*
 * Copyright (c) 2016-2021 Meltytech, LLC
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
    function setControls() {
        programSlider.value = filter.getDouble('target_loudness');
        windowSlider.value = filter.getDouble('window');
        maxgainSlider.value = filter.getDouble('max_gain');
        mingainSlider.value = filter.getDouble('min_gain');
        maxrateSlider.value = filter.getDouble('max_rate');
        discResetCheckbox.checked = parseInt(filter.get('discontinuity_reset'));
    }

    width: 480
    height: 325
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('target_loudness', -23);
            filter.set('window', 10);
            filter.set('max_gain', 15);
            filter.set('min_gain', -15);
            filter.set('max_rate', 3);
            filter.set('discontinuity_reset', 1);
            filter.savePreset(preset.parameters);
        }
        setControls();
        timer.start();
    }

    SystemPalette {
        id: activePalette
    }

    Timer {
        id: timer

        property int _prevResetVal: 0
        property int _resetCountDown: 0

        interval: 200
        running: false
        repeat: true
        onTriggered: {
            loudnessGauge.value = filter.getDouble('in_loudness');
            gainGauge.value = filter.getDouble('out_gain');
            if (filter.get('reset_count') != _prevResetVal) {
                _prevResetVal = filter.get('reset_count');
                _resetCountDown = 1000 / timer.interval;
                resetIndicator.active = true;
            } else if (_resetCountDown != 0) {
                _resetCountDown--;
            } else {
                resetIndicator.active = false;
            }
        }
    }

    GridLayout {
        id: grid

        anchors.fill: parent
        anchors.margins: 8
        columns: 3
        columnSpacing: 8
        rowSpacing: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: ['target_loudness', 'window', 'max_gain', 'min_gain', 'max_rate']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Target Loudness')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The target loudness of the output in LUFS.')
            }

        }

        Shotcut.SliderSpinner {
            id: programSlider

            minimumValue: -50
            maximumValue: -10
            decimals: 1
            suffix: ' LUFS'
            spinnerWidth: 100
            value: filter.getDouble('target_loudness')
            onValueChanged: filter.set('target_loudness', value)
        }

        Shotcut.UndoButton {
            onClicked: programSlider.value = -23
        }

        Label {
            text: qsTr('Analysis Window')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount of history to use to calculate the input loudness.')
            }

        }

        Shotcut.SliderSpinner {
            id: windowSlider

            minimumValue: 2
            maximumValue: 600
            decimals: 0
            suffix: ' s'
            spinnerWidth: 100
            value: filter.getDouble('window')
            onValueChanged: filter.set('window', value)
        }

        Shotcut.UndoButton {
            onClicked: windowSlider.value = 20
        }

        Label {
            text: qsTr('Maximum Gain')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The maximum that the gain can be increased.')
            }

        }

        Shotcut.SliderSpinner {
            id: maxgainSlider

            minimumValue: 0
            maximumValue: 30
            decimals: 1
            suffix: ' dB'
            spinnerWidth: 100
            value: filter.getDouble('max_gain')
            onValueChanged: filter.set('max_gain', value)
        }

        Shotcut.UndoButton {
            onClicked: maxgainSlider.value = 15
        }

        Label {
            text: qsTr('Minimum Gain')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The maximum that the gain can be decreased.')
            }

        }

        Shotcut.SliderSpinner {
            id: mingainSlider

            minimumValue: -30
            maximumValue: 0
            decimals: 1
            suffix: ' dB'
            spinnerWidth: 100
            value: filter.getDouble('min_gain')
            onValueChanged: filter.set('min_gain', value)
        }

        Shotcut.UndoButton {
            onClicked: mingainSlider.value = -15
        }

        Label {
            text: qsTr('Maximum Rate')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The maximum rate that the gain can be changed.')
            }

        }

        Shotcut.SliderSpinner {
            id: maxrateSlider

            minimumValue: 0.5
            maximumValue: 9
            decimals: 1
            stepSize: 0.1
            suffix: ' dB/s'
            spinnerWidth: 100
            value: filter.getDouble('max_rate')
            onValueChanged: filter.set('max_rate', value)
        }

        Shotcut.UndoButton {
            onClicked: maxrateSlider.value = 3
        }

        Label {
        }

        CheckBox {
            id: discResetCheckbox

            text: qsTr('Reset on discontinuity')
            onCheckedChanged: filter.set('discontinuity_reset', checked)

            Shotcut.HoverTip {
                text: qsTr('Reset the measurement if a discontinuity is detected - such as seeking or clip change.')
            }
        }

        Shotcut.UndoButton {
            id: discResetUndo
            onClicked: discResetCheckbox.checked = true
        }

        Rectangle {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.minimumHeight: 12
            color: 'transparent'

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 2
                radius: 2
                color: activePalette.text
            }

        }

        Label {
            text: qsTr('Input Loudness')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop

            Shotcut.HoverTip {
                text: qsTr('Status indicator showing the loudness measured on the input.')
            }

        }

        Shotcut.Gauge {
            id: loudnessGauge

            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.rightMargin: 4
            from: -50
            value: -50
            to: 0
            orientation: Qt.Horizontal
            decimals: 0
        }

        Label {
            text: qsTr('Output Gain')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop

            Shotcut.HoverTip {
                text: qsTr('Status indicator showing the gain being applied.')
            }

        }

        Shotcut.Gauge {
            id: gainGauge

            Layout.columnSpan: 2
            Layout.fillWidth: true
            from: Math.floor(mingainSlider.value)
            value: 0
            to: Math.ceil(maxgainSlider.value)
            orientation: Qt.Horizontal
            decimals: 1
        }

        Label {
            text: qsTr('Reset')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Status indicator showing when the loudness measurement is reset.')
            }

        }

        Rectangle {
            id: resetIndicator

            property bool active: false

            Layout.columnSpan: 2
            width: 20
            height: width
            radius: 10
            color: activePalette.alternateBase

            Rectangle {
                x: 3
                y: 3
                width: 14
                height: width
                radius: 7
                color: parent.active ? activePalette.highlight : activePalette.base
            }

        }

        Item {
            Layout.fillHeight: true
        }

    }

}
