/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

Rectangle {
    width: 400
    height: 200
    color: 'transparent'
    property string gainParameter: 'gain'
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            slider.value = 100
        } else {
            // Initialize parameter values
            slider.value = filter.get(gainParameter) * 100
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            spacing: 8
            Label { text: qsTr('Gain') }
            Slider {
                id: slider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                minimumValue: 0
                maximumValue: 300
                onValueChanged: {
                    spinner.value = value
                    filter.set(gainParameter, value / 100)
                }
            }
            SpinBox {
                id: spinner
                Layout.minimumWidth: 70
                suffix: ' %'
                minimumValue: 0
                maximumValue: 300
                decimals: 1
                onValueChanged: slider.value = value
            }
            UndoButton {
                onClicked: slider.value = 100
            }
        }
        Item {
            Layout.fillHeight: true;
        }
    }
}
