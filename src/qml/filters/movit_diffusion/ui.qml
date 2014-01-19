/*
 * Copyright (c) 2014 Meltytech, LLC
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Radius') }
            Slider {
                id: radiusSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('radius') * 100
                minimumValue: 0
                maximumValue: 2000
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        radiusSpinner.value = value / 100
                        filter.set('radius', value / 100)
                    }
                }
            }
            SpinBox {
                id: radiusSpinner
                Layout.minimumWidth: 70
                value: filter.get('radius')
                decimals: 2
                minimumValue: 0
                maximumValue: 20
                onValueChanged: radiusSlider.value = value * 100
            }
            UndoButton {
                onClicked: radiusSlider.value = 300
            }
        }
        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Blurriness') }
            Slider {
                id: mixSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('mix') * 100
                minimumValue: 0
                maximumValue: 100
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        mixSpinner.value = value
                        filter.set('mix', value / 100)
                    }
                }
            }
            SpinBox {
                id: mixSpinner
                Layout.minimumWidth: 70
                suffix: ' %'
                value: filter.get('mix') * 100
                minimumValue: 0
                maximumValue: 100
                onValueChanged: mixSlider.value = value
            }
            UndoButton {
                onClicked: mixSlider.value = 30
            }
        }
        Item {
            Layout.fillHeight: true;
        }
    }
}
