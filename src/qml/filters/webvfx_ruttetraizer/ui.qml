/*
 * Copyright (c) 2015-2019 Meltytech, LLC
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
import Shotcut.Controls 1.0 as Shotcut

Item {
    width: 400
    height: 250
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('resource', filter.path + 'ruttetraizer.html')
            // Set default parameter values
            filter.set('opacity', 1.0)
            filter.set('thickness', 3.0)
            filter.set('density', 10)
            filter.set('depth', 100)
            filter.set('scale', 3.0)
            filter.set('rotation_x', 0)
            filter.set('rotation_y', 0)
            filter.savePreset(preset.parameters)

            setControls();
        }
    }

    function setControls() {
        opacitySlider.value = filter.getDouble('opacity') * opacitySlider.maximumValue
        thicknessSlider.value = filter.getDouble('thickness')
        densitySlider.value = filter.getDouble('density')
        depthSlider.value = filter.getDouble('depth')
        scaleSlider.value = filter.getDouble('scale') / 3.0
        xRotationSlider.value = filter.getDouble('rotation_x') * xRotationSlider.maximumValue
        yRotationSlider.value = filter.getDouble('rotation_y') * yRotationSlider.maximumValue
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
            parameters: ['opacity', 'thickness', 'density', 'depth', 'scale',
                'rotation_x', 'rotation_y']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Brightness')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: opacitySlider
            minimumValue: 1
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble('opacity') * maximumValue
            onValueChanged: filter.set('opacity', value / maximumValue)
        }
        Shotcut.UndoButton {
            onClicked: opacitySlider.value = 100
        }

        Label {
            text: qsTr('Thickness')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: thicknessSlider
            minimumValue: 1
            maximumValue: 10
            decimals: 1
            value: filter.getDouble('thickness')
            onValueChanged: filter.set('thickness', value)
        }
        Shotcut.UndoButton {
            onClicked: thicknessSlider.value = 3.0
        }

        Label {
            text: qsTr('Density')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: densitySlider
            minimumValue: 1
            maximumValue: 100
            suffix: ' px'
            value: filter.getDouble('density')
            onValueChanged: filter.set('density', value)
        }
        Shotcut.UndoButton {
            onClicked: densitySlider.value = 10
        }

        Label {
            text: qsTr('Depth')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: depthSlider
            minimumValue: 1
            maximumValue: 500
            suffix: ' px'
            value: filter.getDouble('depth')
            onValueChanged: filter.set('depth', value)
        }
        Shotcut.UndoButton {
            onClicked: depthSlider.value = 100
        }

        Label {
            text: qsTr('Scale')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: scaleSlider
            minimumValue: 0.01
            maximumValue: 3.0
            decimals: 2
            value: filter.getDouble('scale') / 3.0
            onValueChanged: filter.set('scale', value * 3.0)
        }
        Shotcut.UndoButton {
            onClicked: scaleSlider.value = 1.0
        }

        Label {
            text: qsTr('X Axis Rotation')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: xRotationSlider
            minimumValue: 0
            maximumValue: 360
            suffix: qsTr(' deg', 'degrees')
            value: filter.getDouble('rotation_x') * maximumValue
            onValueChanged: filter.set('rotation_x', value / maximumValue)
        }
        Shotcut.UndoButton {
            onClicked: xRotationSlider.value = 0
        }

        Label {
            text: qsTr('Y Axis Rotation')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: yRotationSlider
            minimumValue: 0
            maximumValue: 360
            suffix: qsTr(' deg', 'degrees')
            value: filter.getDouble('rotation_y') * maximumValue
            onValueChanged: filter.set('rotation_y', value / maximumValue)
        }
        Shotcut.UndoButton {
            onClicked: yRotationSlider.value = 0
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
