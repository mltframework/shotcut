/*
 * Copyright (c) 2015-2022 Meltytech, LLC
 * Author: Amy Dennedy
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
        deltaSlider.value = filter.get('delta');
        amountSlider.value = filter.get('every');
        brightDeltaSlider.value = filter.get('brightnessdelta_up');
        darkDeltaSlider.value = filter.get('brightnessdelta_down');
        valueSlider.value = filter.get('brightnessdelta_every');
        highDevelopSlider.value = filter.get('unevendevelop_up');
        lowDevelopSlider.value = filter.get('unevendevelop_down');
        durationSlider.value = filter.get('unevendevelop_duration');
    }

    width: 350
    height: 250
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('delta', 14);
            filter.set('every', 20);
            filter.set('brightnessdelta_up', 20);
            filter.set('brightnessdelta_down', 30);
            filter.set('brightnessdelta_every', 70);
            filter.set('unevendevelop_up', 60);
            filter.set('unevendevelop_down', 20);
            filter.set('unevendevelop_duration', 70);
            filter.savePreset(preset.parameters);
        }
        setControls();
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: ['delta', 'every', 'brightnessdelta_up', 'brightnessdelta_down', 'brightnessdelta_every', 'unevendevelop_up', 'unevendevelop_duration']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Vertical amount')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: deltaSlider

            minimumValue: 0
            maximumValue: 200
            value: filter.get('delta')
            onValueChanged: filter.set('delta', value)
        }

        Shotcut.UndoButton {
            onClicked: deltaSlider.value = 14
        }

        Label {
            text: qsTr('Vertical frequency')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: amountSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            value: filter.get('every')
            onValueChanged: filter.set('every', value)
        }

        Shotcut.UndoButton {
            onClicked: amountSlider.value = 20
        }

        Label {
            text: qsTr('Brightness up')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: brightDeltaSlider

            minimumValue: 0
            maximumValue: 100
            value: filter.get('brightnessdelta_up')
            onValueChanged: filter.set('brightnessdelta_up', value)
        }

        Shotcut.UndoButton {
            onClicked: brightDeltaSlider.value = 20
        }

        Label {
            text: qsTr('Brightness down')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: darkDeltaSlider

            minimumValue: 0
            maximumValue: 100
            value: filter.get('brightnessdelta_down')
            onValueChanged: filter.set('brightnessdelta_down', value)
        }

        Shotcut.UndoButton {
            onClicked: darkDeltaSlider.value = 30
        }

        Label {
            text: qsTr('Brightness frequency')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: valueSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            value: filter.get('brightnessdelta_every')
            onValueChanged: filter.set('brightnessdelta_every', value)
        }

        Shotcut.UndoButton {
            onClicked: valueSlider.value = 70
        }

        Label {
            text: qsTr('Uneven develop up')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: highDevelopSlider

            minimumValue: 0
            maximumValue: 100
            value: filter.get('unevendevelop_up')
            onValueChanged: filter.set('unevendevelop_up', value)
        }

        Shotcut.UndoButton {
            onClicked: highDevelopSlider.value = 60
        }

        Label {
            text: qsTr('Uneven develop down')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: lowDevelopSlider

            minimumValue: 0
            maximumValue: 100
            value: filter.get('unevendevelop_down')
            onValueChanged: filter.set('unevendevelop_down', value)
        }

        Shotcut.UndoButton {
            onClicked: lowDevelopSlider.value = 20
        }

        Label {
            text: qsTr('Uneven develop duration')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: durationSlider

            minimumValue: 0
            maximumValue: 1000
            value: filter.get('unevendevelop_duration')
            onValueChanged: filter.set('unevendevelop_duration', value)
        }

        Shotcut.UndoButton {
            onClicked: durationSlider.value = 70
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
