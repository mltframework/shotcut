/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Amy Dennedy
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
    height: 150
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('line_width', 2)
            filter.set('num', 5)
            filter.set('darker', 40)
            filter.set('lighter', 40)
            filter.savePreset(preset.parameters)
            setControls()
        }
    }

    function setControls() {
        widthSlider.value = filter.get('line_width')
        amountSlider.value = filter.get('num')
        darkSlider.value = filter.get('darker')
        lightSlider.value = filter.get('lighter')
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 3

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: ['line_width', 'num', 'darker', 'lighter']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Width')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: widthSlider
            minimumValue: 1
            maximumValue: 100
            value: filter.get('line_width')
            onValueChanged: filter.set('line_width', value)
        }
        UndoButton {
            onClicked: widthSlider.value = 2
        }

        Label {
            text: qsTr('Amount')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: amountSlider
            minimumValue: 1
            maximumValue: 100
            value: filter.get('num')
            onValueChanged: filter.set('num', value)
            }
        UndoButton {
            onClicked: amountSlider.value = 5
        }

        Label {
            text: qsTr('Darkness')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: darkSlider
            minimumValue: 1
            maximumValue: 100
            value: filter.get('darker')
            onValueChanged: filter.set('darker', value)
        }
        UndoButton {
            onClicked: darkSlider.value = 40
        }

        Label {
            text: qsTr('Lightness')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: lightSlider
            minimumValue: 0
            maximumValue: 100
            value: filter.get('lighter')
            onValueChanged: filter.set('lighter', value)
        }
        UndoButton {
            onClicked: lightSlider.value = 40
        }

        Item {
            Layout.fillHeight: true;
        }
    }
  }
