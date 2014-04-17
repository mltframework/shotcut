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

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('wave', 10)
            filter.set('speed', 5)
            filter.set('deformX', 1)
            filter.set('deformY', 1)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        Preset {
            parameters: ['wave', 'speed', 'deformX', 'deformX']
            onPresetSelected: {
                waveSlider.value = filter.get('wave')
                speedSlider.value = filter.get('speed')
                deformXCheckBox.checked = filter.get('deformX') === '1'
                deformYCheckBox.checked = filter.get('deformY') === '1'
            }
        }
        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Amplitude') }
            Slider {
                id: waveSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('wave')
                minimumValue: 1
                maximumValue: 500
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        waveSpinner.value = value
                        filter.set('wave', value)
                    }
                }
            }
            SpinBox {
                id: waveSpinner
                Layout.minimumWidth: 70
                value: filter.get('wave')
                minimumValue: 1
                maximumValue: 500
                onValueChanged: waveSlider.value = value
            }
            UndoButton {
                onClicked: waveSlider.value = 10
            }
        }
        RowLayout {
            spacing: 8
    
            Label { text: qsTr('Speed') }
            Slider {
                id: speedSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('speed')
                minimumValue: 0
                maximumValue: 1000
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        speedSpinner.value = value
                        filter.set('speed', value)
                    }
                }
            }
            SpinBox {
                id: speedSpinner
                Layout.minimumWidth: 70
                value: filter.get('speed')
                minimumValue: 0
                maximumValue: 1000
                onValueChanged: speedSlider.value = value
            }
            UndoButton {
                onClicked: speedSlider.value = 5
            }
        }
        CheckBox {
            id: deformXCheckBox
            text: qsTr('Deform horizontally?')
            checked: filter.get('deformX') === '1'
            property bool isReady: false
            Component.onCompleted: isReady = true
            onClicked: {
                if (isReady)
                    filter.set('deformX', checked)
            }
        }
        CheckBox {
            id: deformYCheckBox
            text: qsTr('Deform vertically?')
            checked: filter.get('deformY') === '1'
            property bool isReady: false
            Component.onCompleted: isReady = true
            onClicked: {
                if (isReady)
                    filter.set('deformY', checked)
            }
        }
        Item {
            Layout.fillHeight: true;
        }
    }
}
