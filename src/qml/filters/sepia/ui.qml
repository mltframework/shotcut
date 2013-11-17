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
    Component.onCompleted: {
        // Initialize parameter values
        sliderBlue.value = filter.get('u')
        sliderRed.value = filter.get('v')
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        Preset {
            parameters: ['u', 'v']
            onPresetSelected: {
                sliderBlue.value = filter.get('u')
                sliderRed.value = filter.get('v')
            }
        }

        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Yellow-Blue') }
            Slider {
                id: sliderBlue
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                minimumValue: 0
                maximumValue: 255
                onValueChanged: {
                    spinnerBlue.value = value
                    filter.set('u', value)
                }
            }
            SpinBox {
                id: spinnerBlue
                Layout.minimumWidth: 70
                minimumValue: 0
                maximumValue: 255
                onValueChanged: sliderBlue.value = value
            }
            Button {
                iconName: 'edit-undo'
                tooltip: qsTr('Reset to default')
                onClicked: sliderBlue.value = 75
                implicitWidth: 20
                implicitHeight: 20
            }
        }

        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Cyan-Red') }
            Slider {
                id: sliderRed
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                minimumValue: 0
                maximumValue: 255
                onValueChanged: {
                    spinnerRed.value = value
                    filter.set('v', value)
                }
            }
            SpinBox {
                id: spinnerRed
                Layout.minimumWidth: 70
                minimumValue: 0
                maximumValue: 255
                onValueChanged: sliderRed.value = value
            }
            Button {
                iconName: 'edit-undo'
                tooltip: qsTr('Reset to default')
                onClicked: sliderRed.value = 150
                implicitWidth: 20
                implicitHeight: 20
            }
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}
