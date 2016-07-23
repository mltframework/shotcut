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

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    property var defaultParameters: ['gamma_r', 'gamma_g', 'gamma_b', 'gain_r', 'gain_g', 'gain_b']
    property double gammaFactor: 2.0
    property double gainFactor: 2.0
    width: 200
    height: 50
    
    function setControls() {
        contrastSlider.value = filter.getDouble("gain_r") / gainFactor * 100.0
    }
    
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set("gamma_r", 1.0);
            filter.set("gamma_g", 1.0);
            filter.set("gamma_b", 1.0);
            filter.set("gain_r", 1.0);
            filter.set("gain_g", 1.0);
            filter.set("gain_b", 1.0);
            filter.savePreset(defaultParameters)
        }
        setControls()
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
                setControls()
            }
        }

        Label {
            text: qsTr('Contrast')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: contrastSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: {
                var v = value / 100.0
                filter.set("gamma_r", (1.0 - v) * gammaFactor)
                filter.set("gamma_g", (1.0 - v) * gammaFactor)
                filter.set("gamma_b", (1.0 - v) * gammaFactor)
                filter.set("gain_r",  v * gainFactor)
                filter.set("gain_g",  v * gainFactor)
                filter.set("gain_b",  v * gainFactor)
            }
        }
        UndoButton {
            onClicked: contrastSlider.value = 50
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
