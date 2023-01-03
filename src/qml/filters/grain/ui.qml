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
        noiseSlider.value = filter.get('noise');
        brightnessSlider.value = filter.get('brightness');
    }

    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('noise', 40);
            filter.set('contrast', 100);
            filter.set('brightness', 83);
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

            parameters: ['noise', 'brightness']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Noise')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: noiseSlider

            minimumValue: 1
            maximumValue: 200
            suffix: ' %'
            value: filter.get('noise')
            onValueChanged: filter.set('noise', value)
        }

        Shotcut.UndoButton {
            onClicked: noiseSlider.value = 40
        }

        Label {
            text: qsTr('Brightness')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: brightnessSlider

            minimumValue: 0
            maximumValue: 400
            value: filter.get('brightness')
            onValueChanged: filter.set('brightness', value)
        }

        Shotcut.UndoButton {
            onClicked: brightnessSlider.value = 83
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
