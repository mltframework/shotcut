/*
 * Copyright (c) 2018 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    property string xSize: '0'
    property string ySize: '1'
    property real maxFilterPercent: 50.0
    property real maxUserPercent: 20.0
    property real defaultValue: 2.5 / maxFilterPercent
    property var defaultParameters: [xSize, ySize]

    width: 200
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set(xSize, defaultValue)
            filter.set(ySize, defaultValue)
            filter.savePreset(defaultParameters)
        }
        wslider.value = filter.getDouble(xSize) * maxFilterPercent
        hslider.value = filter.getDouble(ySize) * maxFilterPercent
    }

    function setControls() {
        wslider.value = filter.getDouble(xSize) * maxFilterPercent
        hslider.value = filter.getDouble(ySize) * maxFilterPercent
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
            parameters: defaultParameters
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Width')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: wslider
            minimumValue: 0.1
            maximumValue: maxUserPercent
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: filter.set(xSize, value / maxFilterPercent)
        }
        UndoButton {
            onClicked: wslider.value = defaultValue * maxFilterPercent
        }

        Label {
            text: qsTr('Height')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: hslider
            minimumValue: 0.1
            maximumValue: maxUserPercent
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: filter.set(ySize, value / maxFilterPercent)
        }
        UndoButton {
            onClicked: hslider.value = defaultValue * maxFilterPercent
        }
        
        Item {
            Layout.fillHeight: true
        }
    }
}
