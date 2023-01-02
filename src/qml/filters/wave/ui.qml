/*
 * Copyright (c) 2014-2021 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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
    height: 150
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('wave', 10);
            filter.set('speed', 5);
            filter.set('deformX', 1);
            filter.set('deformY', 1);
            filter.savePreset(preset.parameters);
        }
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

            Layout.columnSpan: 2
            parameters: ['wave', 'speed', 'deformX', 'deformX']
            onPresetSelected: {
                waveSlider.value = filter.getDouble('wave');
                speedSlider.value = filter.getDouble('speed');
                deformXCheckBox.checked = filter.get('deformX') === '1';
                deformYCheckBox.checked = filter.get('deformY') === '1';
            }
        }

        Label {
            text: qsTr('Amplitude')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: waveSlider

            minimumValue: 1
            maximumValue: 500
            value: filter.getDouble('wave')
            onValueChanged: filter.set('wave', value)
        }

        Shotcut.UndoButton {
            onClicked: waveSlider.value = 10
        }

        Label {
            text: qsTr('Speed')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: speedSlider

            minimumValue: 0
            maximumValue: 1000
            value: filter.getDouble('speed')
            onValueChanged: filter.set('speed', value)
        }

        Shotcut.UndoButton {
            onClicked: speedSlider.value = 5
        }

        Label {
        }

        CheckBox {
            id: deformXCheckBox

            property bool isReady: false

            text: qsTr('Deform horizontally?')
            Layout.columnSpan: 2
            checked: filter.get('deformX') === '1'
            Component.onCompleted: isReady = true
            onClicked: {
                if (isReady)
                    filter.set('deformX', checked);
            }
        }

        Label {
        }

        CheckBox {
            id: deformYCheckBox

            property bool isReady: false

            text: qsTr('Deform vertically?')
            Layout.columnSpan: 2
            checked: filter.get('deformY') === '1'
            Component.onCompleted: isReady = true
            onClicked: {
                if (isReady)
                    filter.set('deformY', checked);
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
