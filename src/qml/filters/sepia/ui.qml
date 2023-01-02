/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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
    height: 100
    Component.onCompleted: {
        if (filter.isNew)
            filter.savePreset(preset.parameters);
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
            parameters: ['u', 'v']
            onPresetSelected: {
                sliderBlue.value = filter.getDouble('u');
                sliderRed.value = filter.getDouble('v');
            }
        }

        Label {
            text: qsTr('Yellow-Blue')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderBlue

            minimumValue: 0
            maximumValue: 255
            value: filter.getDouble('u')
            onValueChanged: filter.set('u', value)
        }

        Shotcut.UndoButton {
            onClicked: sliderBlue.value = 75
        }

        Label {
            text: qsTr('Cyan-Red')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderRed

            minimumValue: 0
            maximumValue: 255
            value: filter.getDouble('v')
            onValueChanged: filter.set('v', value)
        }

        Shotcut.UndoButton {
            onClicked: sliderRed.value = 150
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
