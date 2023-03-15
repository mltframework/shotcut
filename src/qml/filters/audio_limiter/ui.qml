/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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
    function setControls() {
        sliderInput.value = filter.getDouble('0');
        sliderLimit.value = filter.getDouble('1');
        sliderRelease.value = filter.getDouble('2');
    }

    width: 350
    height: 125
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('0', 0);
            filter.set('1', 0);
            filter.set('2', 0.51);
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
            grGauge.value = filter.getDouble('3[0]') * -1;
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

            parameters: ['0', '1', '2']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Input gain')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Gain that is applied to the input stage. Can be used to trim gain to bring it roughly under the limit or to push the signal against the limit.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderInput

            minimumValue: -20
            maximumValue: 20
            suffix: ' dB'
            decimals: 1
            value: filter.getDouble('0')
            onValueChanged: {
                filter.set('0', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderInput.value = 0
        }

        Label {
            text: qsTr('Limit')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The maximum output amplitude. Peaks over this level will be attenuated as smoothly as possible to bring them as close as possible to this level.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderLimit

            minimumValue: -20
            maximumValue: 0
            suffix: ' dB'
            decimals: 1
            value: filter.getDouble('1')
            onValueChanged: {
                filter.set('1', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderLimit.value = 0
        }

        Label {
            text: qsTr('Release')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The time taken for the limiter\'s attenuation to return to 0 dB\'s.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderRelease

            minimumValue: 0.01
            maximumValue: 2
            suffix: ' s'
            decimals: 2
            value: filter.getDouble('2')
            onValueChanged: {
                filter.set('2', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderRelease.value = 0.51
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

        Item {
            Layout.fillHeight: true
        }
    }
}
