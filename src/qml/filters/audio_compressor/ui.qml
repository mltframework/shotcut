/*
 * Copyright (c) 2015-2021 Meltytech, LLC
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
        sliderPeak.value = filter.getDouble('0') * sliderPeak.maximumValue;
        sliderAttack.value = filter.getDouble('1');
        sliderRelease.value = filter.getDouble('2');
        sliderThreshold.value = filter.getDouble('3');
        sliderRatio.value = filter.getDouble('4');
        sliderRadius.value = filter.getDouble('5');
        sliderGain.value = filter.getDouble('6');
    }

    width: 350
    height: 300
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('0', 0);
            filter.set('1', 100);
            filter.set('2', 400);
            filter.set('3', 0);
            filter.set('4', 1);
            filter.set('5', 3.25);
            filter.set('6', 0);
            filter.savePreset(preset.parameters);
        }
        setControls();
        timer.start();
    }

    Timer {
        id: timer

        interval: 100
        running: false
        repeat: true
        onTriggered: {
            grGauge.value = filter.getDouble('8[0]');
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 3

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: ['0', '1', '2', '3', '4', '5', '6']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('RMS')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The balance between the RMS and peak envelope followers. RMS is generally better for subtle, musical compression and peak is better for heavier, fast compression and percussion.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderPeak

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            label: qsTr('Peak')
            suffix: ' %'
            value: filter.getDouble('0') * maximumValue
            onValueChanged: {
                filter.set('0', value / maximumValue);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderPeak.value = sliderPeak.minimumValue
        }

        Label {
            text: qsTr('Attack')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderAttack

            minimumValue: 2
            maximumValue: 400
            suffix: ' ms'
            value: filter.getDouble('1')
            onValueChanged: {
                filter.set('1', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderAttack.value = 100
        }

        Label {
            text: qsTr('Release')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderRelease

            minimumValue: 2
            maximumValue: 800
            suffix: ' ms'
            value: filter.getDouble('2')
            onValueChanged: {
                filter.set('2', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderRelease.value = 400
        }

        Label {
            text: qsTr('Threshold')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The point at which the compressor will start to kick in.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderThreshold

            minimumValue: -30
            maximumValue: 0
            decimals: 1
            suffix: ' dB'
            value: filter.getDouble('3')
            onValueChanged: {
                filter.set('3', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderThreshold.value = 0
        }

        Label {
            text: qsTr('Ratio')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The gain reduction ratio used when the signal level exceeds the threshold.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderRatio

            minimumValue: 1
            maximumValue: 20
            prefix: ' 1:'
            value: filter.getDouble('4')
            onValueChanged: {
                filter.set('4', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderRatio.value = 1
        }

        Label {
            text: qsTr('Knee radius')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The distance from the threshold where the knee curve starts.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderRadius

            minimumValue: 1
            maximumValue: 10
            decimals: 1
            suffix: ' dB'
            value: filter.getDouble('5')
            onValueChanged: {
                filter.set('5', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderRadius.value = 3.25
        }

        Label {
            text: qsTr('Makeup gain')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The gain of the makeup input signal.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderGain

            minimumValue: 0
            maximumValue: 24
            decimals: 1
            suffix: ' dB'
            value: filter.getDouble('6')
            onValueChanged: {
                filter.set('6', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderGain.value = 0
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
            id: grLabel

            text: qsTr('Gain Reduction')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop

            Shotcut.HoverTip {
                text: qsTr('Status indicator showing the gain reduction applied by the compressor.')
            }
        }

        Shotcut.Gauge {
            id: grGauge

            Layout.columnSpan: 2
            Layout.fillWidth: true
            from: -24
            value: 0
            to: 0
            orientation: Qt.Horizontal
            decimals: 0
        }

        Label {
            id: link_Text

            Layout.columnSpan: 3
            text: qsTr('About dynamic range compression')
            font.underline: true

            MouseArea {
                anchors.fill: parent
                onClicked: Qt.openUrlExternally('https://en.wikipedia.org/wiki/Dynamic_range_compression')
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
