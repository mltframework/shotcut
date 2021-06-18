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
    width: 350
    height: 125
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('0', 39)
            filter.set('1', 1)
            filter.set('wetness', 1.0)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        sliderCutoff.value = filter.getDouble('0')
        sliderStages.value = filter.get('1')
        sliderWetness.value = filter.getDouble('wetness') * sliderWetness.maximumValue
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

        Label { text: qsTr('Cutoff frequency')
        Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sliderCutoff
            minimumValue: 5
            maximumValue: 21600
            suffix: ' Hz'
            value: filter.getDouble('0')
            onValueChanged: {
                filter.set('0', value)
            }
        }
        Shotcut.UndoButton {
            onClicked: sliderCutoff.value = 39
        }

        Label { text: qsTr('Rolloff rate')
        Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sliderStages
            minimumValue: 1
            maximumValue: 10
            value: filter.get('1')
            onValueChanged: {
                filter.set('1', value)
            }
        }
        Shotcut.UndoButton {
            onClicked: sliderStages.value = 1
        }

        Label { text: qsTr('Dry')
        Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sliderWetness
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            label: qsTr('Wet')
            suffix: ' %'
            value: filter.getDouble('wetness') * maximumValue
            onValueChanged: {
                filter.set('wetness', value / maximumValue)
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
