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
    property var defaultParameters: ['radius','blur_mix','highlight_cutoff']
    width: 400
    height: 200
    color: 'transparent'
    Component.onCompleted: {
        filter.set('start', 1)
        if (filter.isNew) {
            // Set default parameter values
            filter.set('radius', 20.0)
            filter.set('blur_mix', 1.0)
            filter.set('highlight_cutoff', 0.2)
            filter.savePreset(defaultParameters)
        }
        radiusslider.value = filter.get("radius")
        radiusspinner.value = filter.get("radius")
        blurslider.value = filter.get("blur_mix")
        blurspinner.value = filter.get("blur_mix")
        cutoffslider.value = filter.get("highlight_cutoff")
        cutoffspinner.value = filter.get("highlight_cutoff")
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8
        
        Preset {
            Layout.columnSpan: 4
            parameters: defaultParameters
            onPresetSelected: {
                radiusslider.value = filter.get("radius")
                blurslider.value = filter.get("blur_mix")
                cutoffslider.value = filter.get("highlight_cutoff")
            }
        }

        // Row 1
        Label { text: qsTr('Radius') }
        Slider {
            id: radiusslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: filter.get("radius")
            minimumValue: 0
            maximumValue: 100
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    radiusspinner.value = value
                    filter.set("radius", value)
                }
            }
        }
        SpinBox {
            id: radiusspinner
            Layout.minimumWidth: 80
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            stepSize: 0.1
            onValueChanged: radiusslider.value = value
        }
        UndoButton {
            onClicked: radiusslider.value = 20
        }
        

        // Row 2
        Label { text: qsTr('Highlight Blurriness') }
        Slider {
            id: blurslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: filter.get("blur_mix")
            minimumValue: 0.0
            maximumValue: 1.0
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    blurspinner.value = value
                    filter.set("blur_mix", value)
                }
            }
        }
        SpinBox {
            id: blurspinner
            Layout.minimumWidth: 80
            minimumValue: 0.0
            maximumValue: 1.0
            decimals: 2
            stepSize: 0.01
            onValueChanged: blurslider.value = value
        }
        UndoButton {
            onClicked: blurslider.value = 1.0
        }

        // Row 3
        Label { text: qsTr('Highlight Cutoff') }
        Slider {
            id: cutoffslider
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            value: filter.get("highlight_cutoff")
            minimumValue: 0.1
            maximumValue: 1.0
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    cutoffspinner.value = value
                    filter.set("highlight_cutoff", value)
                }
            }
        }
        SpinBox {
            id: cutoffspinner
            Layout.minimumWidth: 80
            minimumValue: 0.0
            maximumValue: 1.0
            decimals: 2
            stepSize: 0.01
            onValueChanged: cutoffslider.value = value
        }
        UndoButton {
            onClicked: cutoffslider.value = 0.2
        }

        Item {
            Layout.fillHeight: true;
            Layout.columnSpan: 4
        }
    }
}
