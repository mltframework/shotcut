/*
 * Copyright (c) 2015-2021 Meltytech, LLC
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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    function setControls() {
        sizeSlider.value = filter.get('maxdiameter');
        amountSlider.value = filter.get('maxcount');
    }

    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('maxdiameter', 2);
            filter.set('maxcount', 10);
            filter.savePreset(preset.parameters);
        }
        setControls();
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
            id: preset

            parameters: ['maxdiameter', 'maxcount']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sizeSlider

            minimumValue: 1
            maximumValue: 100
            suffix: ' %'
            value: filter.get('maxdiameter')
            onValueChanged: filter.set('maxdiameter', value)
        }

        Shotcut.UndoButton {
            onClicked: sizeSlider.value = 2
        }

        Label {
            text: qsTr('Amount')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: amountSlider

            minimumValue: 1
            maximumValue: 400
            value: filter.get('maxcount')
            onValueChanged: filter.set('maxcount', value)
        }

        Shotcut.UndoButton {
            onClicked: amountSlider.value = 10
        }

        Item {
            Layout.fillHeight: true
        }

    }

}
