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
    property var defaultParameters: ['circle_radius','gaussian_radius', 'correlation', 'noise']
    width: 350
    height: 150
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('circle_radius', 2.0)
            filter.set('gaussian_radius', 0.0)
            filter.set('correlation', 0.95)
            filter.set('noise', 0.01)
            filter.savePreset(defaultParameters)
            cradiusslider.value = filter.getDouble("circle_radius")
            gradiusslider.value = filter.getDouble("gaussian_radius")
            corrslider.value = filter.getDouble("correlation")
            noiseslider.value = filter.getDouble("noise")
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
                cradiusslider.value = filter.getDouble("circle_radius")
                gradiusslider.value = filter.getDouble("gaussian_radius")
                corrslider.value = filter.getDouble("correlation")
                noiseslider.value = filter.getDouble("noise")
            }
        }

        // Row 2
        Label {
            text: qsTr('Circle radius')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: cradiusslider
            minimumValue: 0
            maximumValue: 99.99
            decimals: 2
            stepSize: 0.1
            value: filter.getDouble("circle_radius")
            onValueChanged: filter.set("circle_radius", value)
        }
        UndoButton {
            onClicked: cradiusslider.value = 2
        }
        
        // Row 3
        Label {
            text: qsTr('Gaussian radius')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: gradiusslider
            minimumValue: 0
            maximumValue: 99.99
            decimals: 2
            stepSize: 0.1
            value: filter.getDouble("gaussian_radius")
            onValueChanged: filter.set("gaussian_radius", value)
        }
        UndoButton {
            onClicked: gradiusslider.value = 0
        }

        // Row 4
        Label {
            text: qsTr('Correlation')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: corrslider
            minimumValue: 0.0
            maximumValue: 1.0
            decimals: 2
            value: filter.getDouble("correlation")
            onValueChanged: filter.set("correlation", value)
        }
        UndoButton {
            onClicked: corrslider.value = 0.95
        }

        // Row 5
        Label {
            text: qsTr('Noise')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: noiseslider
            minimumValue: 0.0
            maximumValue: 1.0
            decimals: 2
            value: filter.getDouble("noise")
            onValueChanged: filter.set("noise", value)
        }
        UndoButton {
            onClicked: noiseslider.value = 0.01
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
