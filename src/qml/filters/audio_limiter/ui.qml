/*
 * Copyright (c) 2015 Meltytech, LLC
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

import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    width: 350
    height: 125
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('0', 0)
            filter.set('1', 0)
            filter.set('2', .51)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        sliderInput.value = filter.getDouble('0')
        sliderLimit.value = filter.getDouble('1')
        sliderRelease.value = filter.get('2')
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 3

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: ['0', '1', '2']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Input gain')
            Layout.alignment: Qt.AlignRight
            ToolTip {text: qsTr('Gain that is applied to the input stage. Can be used to trim gain to bring it roughly under the limit or to push the signal against the limit.')}
        }
        SliderSpinner {
            id: sliderInput
            minimumValue: -20
            maximumValue: 20
            suffix: ' dB'
            decimals: 1
            spinnerWidth: 80
            value: filter.getDouble('0')
            onValueChanged: {
                filter.set('0', value)
            }
        }
        UndoButton {
            onClicked: sliderInput.value = 0
        }

        Label {
            text: qsTr('Limit')
            Layout.alignment: Qt.AlignRight
            ToolTip {text: qsTr('The maximum output amplitude. Peaks over this level will be attenuated as smoothly as possible to bring them as close as possible to this level.')}
        }
        SliderSpinner {
            id: sliderLimit
            minimumValue: -20
            maximumValue: 0
            suffix: ' dB'
            decimals: 1
            spinnerWidth: 80
            value: filter.getDouble('1')
            onValueChanged: {
                filter.set('1', value)
            }
        }
        UndoButton {
            onClicked: sliderLimit.value = 0
        }

        Label {
            text: qsTr('Release')
            Layout.alignment: Qt.AlignRight
            ToolTip {text: qsTr('The time taken for the limiter\'s attenuation to return to 0 dB\'s.')}
        }
        SliderSpinner {
            id: sliderRelease
            minimumValue: .01
            maximumValue: 2
            suffix: ' s'
            decimals: 2
            spinnerWidth: 80
            value: filter.get('2')
            onValueChanged: {
                filter.set('2', value)
            }
        }
        UndoButton {
            onClicked: sliderRelease.value = .51
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}
