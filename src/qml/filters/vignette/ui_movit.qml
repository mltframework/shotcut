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

        Preset {
            parameters: ['radius', 'inner_radius']
            onPresetSelected: {
                radiusSlider.value = filter.get('radius') * 100
                innerSlider.value = filter.get('inner_radius') * 100
            }
        }

        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Outer Radius') }
            Slider {
                id: radiusSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('radius') * 100
                minimumValue: 0
                maximumValue: 100
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        radiusSpinner.value = value
                        filter.set('radius', value / 100)
                    }
                }
            }
            SpinBox {
                id: radiusSpinner
                Layout.minimumWidth: 70
                suffix: ' %'
                value: filter.get('radius') * 100
                minimumValue: 0
                maximumValue: 100
                onValueChanged: radiusSlider.value = value
            }
            UndoButton {
                onClicked: radiusSlider.value = 30
            }
        }
        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Inner Radius') }
            Slider {
                id: innerSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('inner_radius') * 100
                minimumValue: 0
                maximumValue: 100
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        innerSpinner.value = value
                        filter.set('inner_radius', value / 100)
                    }
                }
            }
            SpinBox {
                id: innerSpinner
                Layout.minimumWidth: 70
                suffix: ' %'
                value: filter.get('inner_radius') * 100
                minimumValue: 0
                maximumValue: 100
                onValueChanged: innerSlider.value = value
            }
            UndoButton {
                onClicked: innerSlider.value = 30
            }
        }
        Item {
            Layout.fillHeight: true;
        }
    }
}
