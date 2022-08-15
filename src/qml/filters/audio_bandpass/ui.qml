/*
 * Copyright (c) 2015-2021 Meltytech, LLC
 * Author: Lauren Dennedy
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
        sliderCenter.value = filter.getDouble('0');
        sliderBandwidth.value = filter.getDouble('1');
        sliderStages.value = filter.get('2');
        sliderWetness.value = filter.getDouble('wetness') * sliderWetness.maximumValue;
    }

    width: 350
    height: 150
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('0', 322);
            filter.set('1', 322);
            filter.set('2', 1);
            filter.set('wetness', 1);
            filter.savePreset(preset.parameters);
        }
        setControls();
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

            parameters: ['0', '1', '2', 'wetness']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Center frequency')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderCenter

            minimumValue: 5
            maximumValue: 21600
            suffix: ' Hz'
            spinnerWidth: 100
            value: filter.getDouble('0')
            onValueChanged: {
                filter.set('0', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderCenter.value = 322
        }

        Label {
            text: qsTr('Bandwidth')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderBandwidth

            minimumValue: 5
            maximumValue: 21600
            suffix: ' Hz'
            spinnerWidth: 100
            value: filter.getDouble('1')
            onValueChanged: {
                filter.set('1', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderBandwidth.value = 322
        }

        Label {
            text: qsTr('Rolloff rate')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderStages

            minimumValue: 1
            maximumValue: 10
            spinnerWidth: 100
            value: filter.get('2')
            onValueChanged: {
                filter.set('2', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderStages.value = 1
        }

        Label {
            text: qsTr('Dry')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderWetness

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            spinnerWidth: 100
            label: qsTr('Wet')
            suffix: ' %'
            value: filter.getDouble('wetness') * maximumValue
            onValueChanged: {
                filter.set('wetness', value / maximumValue);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderWetness.value = sliderWetness.maximumValue
        }

        Item {
            Layout.fillHeight: true
        }

    }

}
