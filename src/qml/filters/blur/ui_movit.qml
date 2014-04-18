/*
 * Copyright (c) 2013 Meltytech, LLC
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
    width: 400
    height: 200
    color: 'transparent'

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            spacing: 8
            
            Label { text: qsTr('Radius') }
            Slider {
                id: slider
                Layout.fillWidth: true
                value: filter.get('radius')
                Layout.minimumWidth: 100
                minimumValue: 0
                maximumValue: 99.99
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        spinner.value = value
                        filter.set('radius', value)
                    }
                }
            }
            SpinBox {
                id: spinner
                Layout.minimumWidth: 80
                value: filter.get('radius')
                minimumValue: 0
                maximumValue: 99.99
                decimals: 2
                onValueChanged: slider.value = value
            }
            UndoButton {
                onClicked: slider.value = 3.0
            }
        }
        
        Item {
            Layout.fillHeight: true;
        }
    }
}
