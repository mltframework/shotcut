/*
 * Copyright (c) 2021 Meltytech, LLC
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
    property int qualityDefault: 5
    property int qpDefault: 10
    property int strengthDefault: 5
    property int strengthMin: -15
    property int strengthMax: 32

    function setControls() {
        qpSlider.value = filter.get('av.qp');
        strengthSlider.value = Math.round((filter.get('av.strength') - strengthMin) / (strengthMax - strengthMin) * 100);
    }

    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('av.quality', qualityDefault);
            filter.set('av.qp', qpDefault);
            filter.set('av.strength', strengthDefault);
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

            parameters: ['av.qp', 'av.strength']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Quantization')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: qpSlider

            minimumValue: 0
            maximumValue: 64
            stepSize: 1
            onValueChanged: filter.set('av.qp', qpSlider.value)
        }

        Shotcut.UndoButton {
            onClicked: qpSlider.value = qpDefault
        }

        Label {
            text: qsTr('Strength')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: strengthSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 1
            suffix: ' %'
            onValueChanged: filter.set('av.strength', strengthSlider.value / 100 * (strengthMax - strengthMin) + strengthMin)
        }

        Shotcut.UndoButton {
            onClicked: strengthSlider.value = Math.round((strengthDefault - strengthMin) / (strengthMax - strengthMin) * 100)
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
