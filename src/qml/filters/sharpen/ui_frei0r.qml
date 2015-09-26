/*
 * Copyright (c) 2014-2015 Meltytech, LLC
 * Author: Brian Matherly <pez4brian@yahoo.com>
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
    property string paramAmount: '0'
    property string paramSize: '1'
    property var defaultParameters: [paramAmount, paramSize]
    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set(paramAmount, 0.5)
            filter.set(paramSize, 0.5)
            filter.savePreset(defaultParameters)
            aslider.value = filter.getDouble(paramAmount) * 100.0
            sslider.value = filter.getDouble(paramSize) * 100.0
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
            Layout.columnSpan: 2
            parameters: defaultParameters
            onPresetSelected: {
                aslider.value = filter.getDouble(paramAmount) * 100.0
                sslider.value = filter.getDouble(paramSize) * 100.0
            }
        }

        Label {
            text: qsTr('Amount')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: aslider
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            decimals: 1
            value: filter.getDouble(paramAmount) * 100.0
            onValueChanged: filter.set(paramAmount, value / 100.0)
        }
        UndoButton {
            onClicked: aslider.value = 50
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: sslider
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            decimals: 1
            value: filter.getDouble(paramSize) * 100.0
            onValueChanged: filter.set(paramSize, value / 100.0)
        }
        UndoButton {
            onClicked: sslider.value = 50
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
