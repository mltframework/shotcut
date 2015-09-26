/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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

import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    width: 350
    height: 50
    property string saturationParameter: '0'
    property real frei0rMaximum: 800
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set(saturationParameter, 100 / frei0rMaximum)
            slider.value = filter.getDouble(saturationParameter) * frei0rMaximum
            filter.savePreset(preset.parameters)
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
        Preset {
            id: preset
            Layout.columnSpan: 2
            parameters: [saturationParameter]
            onPresetSelected: {
                slider.value = filter.getDouble(saturationParameter) * frei0rMaximum
            }
        }

        Label {
            text: qsTr('Saturation')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: slider
            minimumValue: 0
            maximumValue: 300
            suffix: ' %'
            value: filter.getDouble(saturationParameter) * frei0rMaximum
            onValueChanged: filter.set(saturationParameter, value / frei0rMaximum)
        }
        UndoButton {
            onClicked: slider.value = 100
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
