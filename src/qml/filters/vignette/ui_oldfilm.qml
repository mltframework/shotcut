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
            parameters: ['radius', 'smooth', 'opacity', 'mode']
            onPresetSelected: {
                radiusSlider.value = filter.get('radius') * 100
                smoothSlider.value = filter.get('smooth') * 100
                opacitySlider.value = (1.0 - filter.get('opacity')) * 100
                modeCheckBox.checked = filter.get('mode') === '1'
            }
        }

        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Radius') }
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
                onClicked: radiusSlider.value = 50
            }
        }
        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Feathering') }
            Slider {
                id: smoothSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('smooth') * 100
                minimumValue: 0
                maximumValue: 500
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        smoothSpinner.value = value
                        filter.set('smooth', value / 100)
                    }
                }
            }
            SpinBox {
                id: smoothSpinner
                Layout.minimumWidth: 70
                suffix: ' %'
                value: filter.get('smooth') * 100
                minimumValue: 0
                maximumValue: 500
                onValueChanged: smoothSlider.value = value
            }
            UndoButton {
                onClicked: smoothSlider.value = 80
            }
        }
        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Opacity') }
            Slider {
                id: opacitySlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: (1.0 - filter.get('opacity')) * 100
                minimumValue: 0
                maximumValue: 100
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        opacitySpinner.value = value
                        filter.set('opacity', 1.0 - value / 100)
                    }
                }
            }
            SpinBox {
                id: opacitySpinner
                Layout.minimumWidth: 70
                suffix: ' %'
                value: (1.0 - filter.get('opacity')) * 100
                minimumValue: 0
                maximumValue: 100
                onValueChanged: opacitySlider.value = value
            }
            UndoButton {
                onClicked: opacitySlider.value = 100
            }
        }
        CheckBox {
            id: modeCheckBox
            text: qsTr('Non-linear Feathering')
            checked: filter.get('mode') === '1'
            property bool isReady: false
            Component.onCompleted: isReady = true
            onClicked: {
                if (isReady)
                    filter.set('mode', checked)
            }
        }
        Item {
            Layout.fillHeight: true;
        }
    }
}
