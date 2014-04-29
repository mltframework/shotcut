/*
 * Copyright (c) 2014 Meltytech, LLC
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

Rectangle {
    property var defaultParameters: ['circle_radius','gaussian_radius', 'correlation', 'noise']
    width: 400
    height: 200
    color: 'transparent'
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('circle_radius', 2.0)
            filter.set('gaussian_radius', 0.0)
            filter.set('correlation', 0.95)
            filter.set('noise', 0.01)
            filter.savePreset(defaultParameters)
        }
        cradiusslider.value = filter.get("circle_radius")
        cradiusspinner.value = filter.get("circle_radius")
        gradiusslider.value = filter.get("gaussian_radius")
        gradiusspinner.value = filter.get("gaussian_radius")
        corrslider.value = filter.get("correlation")
        corrspinner.value = filter.get("correlation")
        noiseslider.value = filter.get("noise")
        noisespinner.value = filter.get("noise")
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8
        
        Preset {
            Layout.columnSpan: 4
            parameters: defaultParameters
            onPresetSelected: {
                cradiusslider.value = filter.get("circle_radius")
                gradiusslider.value = filter.get("gaussian_radius")
                corrslider.value = filter.get("correlation")
                noiseslider.value = filter.get("noise")
            }
        }

        // Row 1
        Label { text: qsTr('Circle Radius') }
        Slider {
            id: cradiusslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: filter.get("circle_radius")
            minimumValue: 0
            maximumValue: 99.99
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    cradiusspinner.value = value
                    filter.set("circle_radius", value)
                }
            }
        }
        SpinBox {
            id: cradiusspinner
            Layout.minimumWidth: 80
            minimumValue: 0
            maximumValue: 99.99
            decimals: 2
            stepSize: 0.1
            onValueChanged: cradiusslider.value = value
        }
        UndoButton {
            onClicked: cradiusslider.value = 2
        }
        
        // Row 2
        Label { text: qsTr('Gaussian Radius') }
        Slider {
            id: gradiusslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: filter.get("gaussian_radius")
            minimumValue: 0
            maximumValue: 99.99
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    gradiusspinner.value = value
                    filter.set("gaussian_radius", value)
                }
            }
        }
        SpinBox {
            id: gradiusspinner
            Layout.minimumWidth: 80
            minimumValue: 0
            maximumValue: 99.99
            decimals: 2
            stepSize: 0.1
            onValueChanged: gradiusslider.value = value
        }
        UndoButton {
            onClicked: gradiusslider.value = 0
        }

        // Row 3
        Label { text: qsTr('Correlation') }
        Slider {
            id: corrslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: filter.get("correlation")
            minimumValue: 0.0
            maximumValue: 1.0
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    corrspinner.value = value
                    filter.set("correlation", value)
                }
            }
        }
        SpinBox {
            id: corrspinner
            Layout.minimumWidth: 80
            minimumValue: 0.0
            maximumValue: 1.0
            decimals: 2
            stepSize: 0.01
            onValueChanged: corrslider.value = value
        }
        UndoButton {
            onClicked: corrslider.value = 0.95
        }

        // Row 4
        Label { text: qsTr('Noise') }
        Slider {
            id: noiseslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: filter.get("noise")
            minimumValue: 0.0
            maximumValue: 1.0
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    noisespinner.value = value
                    filter.set("noise", value)
                }
            }
        }
        SpinBox {
            id: noisespinner
            Layout.minimumWidth: 80
            minimumValue: 0.0
            maximumValue: 1.0
            decimals: 2
            stepSize: 0.01
            onValueChanged: noiseslider.value = value
        }
        UndoButton {
            onClicked: noiseslider.value = 0.01
        }

        Item {
            Layout.fillHeight: true;
            Layout.columnSpan: 4
        }
    }
}
