/*
 * Copyright (c) 2016 Meltytech, LLC
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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0
import QtQuick.Controls.Styles 1.1

Item {
    property string xCent: '0'
    property double xCentd: 0.5
    property string yCent: '1'
    property double yCentd: 0.5
    property string centCor: '2'
    property double centCord: 0.5
    property string edgeCor: '3'
    property double edgeCord: 0.5
    
    property var defaultParameters: [xCent, yCent, centCor, edgeCor]
   
    width: 300
    height: 150
    Component.onCompleted: {
        presetItem.parameters = defaultParameters
        if (filter.isNew) {
            // Set default parameter values
            filter.set(xCent, xCentd)
            filter.set(yCent, yCentd)
            filter.set(centCor, centCord)
            filter.set(edgeCor, edgeCord)
            filter.savePreset(defaultParameters)
        }
        xSlider.value = filter.getDouble(xCent)
        ySlider.value = filter.getDouble(yCent)
        cCorSlider.value = filter.getDouble(centCor)
        eCorSlider.value = filter.getDouble(edgeCor)
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
            id: presetItem
            Layout.columnSpan: 2
            onPresetSelected: {
                xSlider.value = filter.getDouble(xCent)
                ySlider.value = filter.getDouble(yCent)
                cCorSlider.value = filter.getDouble(centCor)
                eCorSlider.value = filter.getDouble(edgeCor)
            }
        }

        // Row 1
        Label {
            text: qsTr('X Center')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: xSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            value: filter.getDouble(xCent)
            onValueChanged: filter.set(xCent, value)
        }
        UndoButton {
            onClicked: xSlider.value = xCentd
        }

		// Row 2
        Label {
            text: qsTr('Y Center')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: ySlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            value: filter.getDouble(yCent)
            onValueChanged: filter.set(yCent, value)

        }
        UndoButton {
            onClicked: ySlider.value = yCentd
        }

        // Row 3
        Label {
            text: qsTr('Correction at Center')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: cCorSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            value: filter.getDouble(centCor)
            onValueChanged: filter.set(centCor, value)

        }
        UndoButton {
            onClicked: cCorSlider.value = centCord
        }

        // Row 4
        Label {
            text: qsTr('Correction at Edges')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: eCorSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            value: filter.getDouble(edgeCor)
            onValueChanged: filter.set(edgeCor, value)

        }
        UndoButton {
            onClicked: eCorSlider.value = edgeCord
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
