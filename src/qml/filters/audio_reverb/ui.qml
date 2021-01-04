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
    width: 350
    height: 250
    Component.onCompleted: {
        filter.blockSignals = true
        if (filter.isNew) {
            // Set preset parameter values
            filter.set('0', 40)
            filter.set('1', 4)
            filter.set('2', 0.9)
            filter.set('3', 0.75)
            filter.set('4', 0)
            filter.set('5', -22)
            filter.set('6', -28)
            filter.savePreset(preset.parameters, qsTr('Quick fix'))

            filter.set('0', 50)
            filter.set('1', 1.5)
            filter.set('2', 0.1)
            filter.set('3', 0.75)
            filter.set('4', -1.5)
            filter.set('5', -10)
            filter.set('6', -20)
            filter.savePreset(preset.parameters, qsTr('Small hall'))

            filter.set('0', 40)
            filter.set('1', 20)
            filter.set('2', 0.5)
            filter.set('3', 0.75)
            filter.set('4', 0)
            filter.set('5', -10)
            filter.set('6', -30)
            filter.savePreset(preset.parameters, qsTr('Large hall'))

            filter.set('0', 6)
            filter.set('1', 15)
            filter.set('2', 0.9)
            filter.set('3', 0.1)
            filter.set('4', -10)
            filter.set('5', -10)
            filter.set('6', -10)
            filter.savePreset(preset.parameters, qsTr('Sewer'))

            filter.set('0', 6)
            filter.set('1', 15)
            filter.set('2', 0.9)
            filter.set('3', .1)
            filter.set('4', -10)
            filter.set('5', -10)
            filter.set('6', -10)
            filter.savePreset(preset.parameters, qsTr('Church'))

            // Set default parameter values
            filter.set('0', 30)
            filter.set('1', 7.5)
            filter.set('2', 0.5)
            filter.set('3', 0.75)
            filter.set('4', 0)
            filter.set('5', -10)
            filter.set('6', -17.5)
            filter.savePreset(preset.parameters)
        }
        filter.blockSignals = false
        filter.changed()
        setControls()
    }

    function setControls() {
        sliderRoom.value = filter.getDouble('0')
        sliderTime.value = filter.getDouble('1')
        sliderDamp.value = filter.getDouble('2') * sliderDamp.maximumValue
        sliderInput.value = filter.getDouble('3') * sliderInput.maximumValue
        sliderDry.value = filter.getDouble('4')
        sliderReflection.value = filter.getDouble('5')
        sliderTail.value = filter.getDouble('6')
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
            text: qsTr('Room size')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {text: qsTr('The size of the room, in meters. Excessively large, and excessively small values will make it sound a bit unrealistic. Values of around 30 sound good.')}
        }
        Shotcut.SliderSpinner {
            id: sliderRoom
            minimumValue: 1
            maximumValue: 300
            suffix: ' m'
            value: filter.getDouble('0')
            onValueChanged: {
                filter.set('0', value)
            }
        }
        Shotcut.UndoButton {
            onClicked: sliderRoom.value = 30
        }

        Label {
            text: qsTr('Reverb time')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sliderTime
            minimumValue: .1
            maximumValue: 30
            decimals: 1
            suffix: ' s'
            value: filter.getDouble('1')
            onValueChanged: {
                filter.set('1', value)
            }
        }
        Shotcut.UndoButton {
            onClicked: sliderTime.value = 7.5
        }

        Label {
            text: qsTr('Damping')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {text: qsTr('This controls the high frequency damping (a lowpass filter), values near 1 will make it sound very bright, values near 0 will make it sound very dark.')}
        }
        Shotcut.SliderSpinner {
            id: sliderDamp
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble('2') * maximumValue
            onValueChanged: {
                filter.set('2', value / maximumValue)
            }
        }
        Shotcut.UndoButton {
            onClicked: sliderDamp.value = 0.5 * sliderDamp.maximumValue
        }

        Label {
            text: qsTr('Input bandwidth')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {text: qsTr('This is like a damping control for the input, it has a similar effect to the damping control, but is subtly different.')}
        }
        Shotcut.SliderSpinner {
            id: sliderInput
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble('3') * maximumValue
            onValueChanged: {
                filter.set('3', value / maximumValue)
            }
        }
        Shotcut.UndoButton {
            onClicked: sliderInput.value = 0.75 * sliderInput.maximumValue
        }

        Label {
            text: qsTr('Dry signal level')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {text: qsTr('The amount of dry signal to be mixed with the reverberated signal.')}
        }
        Shotcut.SliderSpinner {
            id: sliderDry
            minimumValue: -70
            maximumValue: 0
            suffix: ' dB'
            decimals: 1
            value: filter.getDouble('4')
            onValueChanged: {
                filter.set('4', value)
            }
        }
        Shotcut.UndoButton {
            onClicked: sliderDry.value = 0
        }

        Label {
            text: qsTr('Early reflection level')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {text: qsTr('The distance from the threshold where the knee curve starts.')}
        }
        Shotcut.SliderSpinner {
            id: sliderReflection
            minimumValue: -70
            maximumValue: 0
            suffix: ' dB'
            value: filter.getDouble('5')
            onValueChanged: {
                filter.set('5', value)
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderReflection.value = -10
        }
        Label {
            text: qsTr('Tail level')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {text: qsTr('The quantity of early reflections (scatter reflections directly from the source).')}
        }
        Shotcut.SliderSpinner {
            id: sliderTail
            minimumValue: -70
            maximumValue: 0
            decimals: 1
            suffix: ' dB'
            value: filter.getDouble('6')
            onValueChanged: {
                filter.set('6', value)
            }
        }
        Shotcut.UndoButton {
            onClicked: sliderTail.value = -17.5
        }

        Label {
            Layout.columnSpan: 3
            text: qsTr('About reverb')
            font.underline: true
            MouseArea {
                anchors.fill: parent
                onClicked: Qt.openUrlExternally('https://wiki.audacityteam.org/wiki/GVerb')
            }
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}
