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
    property var defaultParameters: ['radius','blur_mix','highlight_cutoff']
    width: 350
    height: 125
    Component.onCompleted: {
        filter.set('start', 1)
        if (filter.isNew) {
            // Set default parameter values
            filter.set('radius', 20.0)
            filter.set('blur_mix', 1.0)
            filter.set('highlight_cutoff', 0.2)
            filter.savePreset(defaultParameters)
            radiusslider.value = filter.getDouble("radius")
            blurslider.value = filter.getDouble("blur_mix")
            cutoffslider.value = filter.getDouble("highlight_cutoff")
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
                radiusslider.value = filter.getDouble("radius")
                blurslider.value = filter.getDouble("blur_mix")
                cutoffslider.value = filter.getDouble("highlight_cutoff")
            }
        }

        // Row 1
        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: radiusslider
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            value: filter.getDouble("radius")
            onValueChanged: filter.set("radius", value)
        }
        UndoButton {
            onClicked: radiusslider.value = 20
        }

        // Row 2
        Label { 
            text: qsTr('Highlight blurriness')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: blurslider
            minimumValue: 0.0
            maximumValue: 1.0
            decimals: 2
            value: filter.getDouble("blur_mix")
            onValueChanged: filter.set("blur_mix", value)
        }
        UndoButton {
            onClicked: blurslider.value = 1.0
        }

        // Row 3
        Label {
            text: qsTr('Highlight cutoff')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: cutoffslider
            minimumValue: 0.1
            maximumValue: 1.0
            decimals: 2
            value: filter.getDouble("highlight_cutoff")
            onValueChanged: filter.set("highlight_cutoff", value)
        }
        UndoButton {
            onClicked: cutoffslider.value = 0.2
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
