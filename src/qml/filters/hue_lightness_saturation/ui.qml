/*
 * Copyright (c) 2018-2021 Meltytech, LLC
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
    property double hueDegreeDefault: 100
    property double lightnessDefault: 100
    property double saturationDefault: 100

    property var defaultParameters: ["av.h", "av.b", "av.s"]

    width: 200
    height: 125
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set("av.h", (hueDegreeDefault - 100) * 360 / 100)
            filter.set("av.b", (lightnessDefault - 100) * 10 / 100)
            filter.set("av.s", saturationDefault / 100)
            filter.savePreset(defaultParameters)
        }
        setControls()
    }

    function setControls() {
        hueDegreeSlider.value = filter.getDouble("av.h") * 100 / 360 + 100
        lightnessSlider.value = filter.getDouble("av.b") * 100 / 10 + 100
        saturationSlider.value = filter.getDouble("av.s") * 100
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
            id: presetItem
            Layout.columnSpan: 2
            parameters: defaultParameters
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Hue')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hueDegreeSlider
            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: filter.set("av.h", (value - 100) * 360 / 100)
        }
        Shotcut.UndoButton {
            onClicked: hueDegreeSlider.value = hueDegreeDefault
        }

        Label {
            text: qsTr('Lightness')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lightnessSlider
            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: filter.set("av.b", (value - 100) * 10 / 100)
        }
        Shotcut.UndoButton {
            onClicked: lightnessSlider.value = lightnessDefault
        }
        
        Label {
            text: qsTr('Saturation')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: saturationSlider
            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: filter.set("av.s", value / 100.0)
        }
        Shotcut.UndoButton {
            onClicked: saturationSlider.value = saturationDefault
        }

        Item { Layout.fillHeight: true }
    }
}
