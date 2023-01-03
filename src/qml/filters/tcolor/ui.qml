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
        bySlider.value = filter.get('oversaturate_cr');
        rgSlider.value = filter.get('oversaturate_cb');
    }

    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('oversaturate_cr', 190);
            filter.set('oversaturate_cb', 190);
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

            parameters: ['oversaturate_cr', 'oversaturate_cb']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Green')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: bySlider

            minimumValue: -300
            maximumValue: 300
            value: filter.get('oversaturate_cr')
            label: qsTr(' Red')
            onValueChanged: filter.set('oversaturate_cr', value)
        }

        Shotcut.UndoButton {
            onClicked: bySlider.value = 190
        }

        Label {
            text: qsTr('Yellow')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: rgSlider

            minimumValue: -300
            maximumValue: 300
            value: filter.get('oversaturate_cb')
            label: qsTr('Blue')
            onValueChanged: filter.set('oversaturate_cb', value)
        }

        Shotcut.UndoButton {
            onClicked: rgSlider.value = 190
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
