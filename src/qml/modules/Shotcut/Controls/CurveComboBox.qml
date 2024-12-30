/*
 * Copyright (c) 2024 Meltytech, LLC
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
import QtQml.Models
import Shotcut.Controls as Shotcut

Shotcut.ComboBox {
    id: curveCombo

    function setCurrentValue(value) {
        var modelIndex = indexOfValue(value);
        if (modelIndex >= 0) {
            currentIndex = modelIndex;
        } else {
            console.log("Invalid value for curve", value);
            currentIndex = 0;
        }
    }

    model: ListModel {
        id: curveModel
        ListElement {
            text: qsTr('Linear')
            value: 1
        }
        ListElement {
            text: qsTr('Smooth')
            value: 4
        }
        ListElement {
            text: qsTr('Ease In Sinusoidal')
            value: 5
        }
        ListElement {
            text: qsTr('Ease Out Sinusoidal')
            value: 6
        }
        ListElement {
            text: qsTr('Ease In/Out Sinusoidal')
            value: 7
        }
        ListElement {
            text: qsTr('Ease In Quadratic')
            value: 8
        }
        ListElement {
            text: qsTr('Ease Out Quadratic')
            value: 9
        }
        ListElement {
            text: qsTr('Ease In/Out Quadratic')
            value: 10
        }
        ListElement {
            text: qsTr('Ease In Cubic')
            value: 11
        }
        ListElement {
            text: qsTr('Ease Out Cubic')
            value: 12
        }
        ListElement {
            text: qsTr('Ease In/Out Cubic')
            value: 13
        }
        ListElement {
            text: qsTr('Ease In Quartic')
            value: 14
        }
        ListElement {
            text: qsTr('Ease Out Quartic')
            value: 15
        }
        ListElement {
            text: qsTr('Ease In/Out Quartic')
            value: 16
        }
        ListElement {
            text: qsTr('Ease In Exponential')
            value: 17
        }
        ListElement {
            text: qsTr('Ease Out Exponential')
            value: 18
        }
        ListElement {
            text: qsTr('Ease In/Out Exponential')
            value: 19
        }
        ListElement {
            text: qsTr('Ease In Circular')
            value: 20
        }
        ListElement {
            text: qsTr('Ease Out Circular')
            value: 21
        }
        ListElement {
            text: qsTr('Ease In/Out Circular')
            value: 22
        }
    }

    textRole: "text"
    valueRole: "value"
}
