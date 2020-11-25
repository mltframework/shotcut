/*
 * Copyright (c) 2014-2020 Meltytech, LLC
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
import QtQuick.Controls.Styles 1.1
import QtQuick.Controls 2.12 as Controls2
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import Shotcut.Controls 1.0

Item {
    width: 400
    height: 250
    Component.onCompleted: {
        filter.set('mlt_resolution_scale', 1)
        if (filter.isNew) {
            filter.set('resource', filter.path + 'threejs_text.html')
            // Set default parameter values
            textField.text = qsTr('3D Text')
            filter.set('text', textField.text)
            filter.set('color', '#CCCCCC')
            filter.set('x_rotation', 0.5)
            filter.set('font', 'droid sans')
            filter.set('weight', 'bold')
            filter.set('bevel', true)
            filter.set('depth', 20)
            filter.set('size', 70)
            filter.set('horizontal', 0.5)
            filter.set('vertical', 0.5)
            filter.savePreset(preset.parameters)

            setControls();
        }
    }

    function setControls() {
        colorSwatch.value = filter.get('color')
        tiltSlider.value = filter.getDouble('x_rotation') * tiltSlider.maximumValue
        fontCombo.currentIndex = fontCombo.valueToIndex()
        boldCheckBox.checked = filter.get('weight') === 'bold'
        bevelCheckBox.checked = filter.getDouble('bevel')
        depthSlider.value = filter.getDouble('depth') * 2
        sizeSlider.value = filter.getDouble('size')
        horizontalSlider.value = filter.getDouble('horizontal') * horizontalSlider.maximumValue
        verticalSlider.value = filter.getDouble('vertical') * verticalSlider.maximumValue
    }

    GridLayout {
        columns: 5
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: ['color', 'x_rotation', 'font', 'weight', 'bevel',
                'depth', 'size', 'horizontal', 'vertical']
            Layout.columnSpan: 4
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Text')
            Layout.alignment: Qt.AlignRight
        }
        TextField {
            id: textField
            Layout.columnSpan: 4
            text: filter.get('text')
            Layout.minimumWidth: sizeSlider.width
            Layout.maximumWidth: sizeSlider.width
            onTextChanged: filter.set('text', text)
        }

        Label {
            text: qsTr('Font')
            Layout.alignment: Qt.AlignRight
        }
        Controls2.ComboBox {
            id: fontCombo
            implicitWidth: 200
            model: ['Liberation Sans', 'Liberation Serif', 'Gentilis', 'Helvetiker', 'Optimer']
            property var values: ['liberation sans', 'liberation serif', 'gentilis', 'helvetiker', 'optimer']
            function valueToIndex() {
                var w = filter.get('font')
                if (w === 'droid sans')
                    w = 'liberation sans';
                else if (w === 'droid serif')
                    w = 'liberation serif';
                for (var i = 0; i < values.length; ++i)
                    if (values[i] === w) break;
                if (i === values.length) i = 0;
                return i;
            }
            currentIndex: valueToIndex()
            onActivated: filter.set('font', values[index])
        }
        CheckBox {
            id: boldCheckBox
            checked: filter.get('weight') === 'bold'
            text: qsTr('Bold')
            onCheckedChanged: filter.set('weight', checked? 'bold' : 'normal')
        }
        CheckBox {
            id: bevelCheckBox
            Layout.columnSpan: 2
            checked: filter.getDouble('bevel')
            text: qsTr('Beveled')
            onCheckedChanged: filter.set('bevel', checked)
        }

        Label {
            text: qsTr('Color')
            Layout.alignment: Qt.AlignRight
        }
        ColorPicker {
            id: colorSwatch
            Layout.columnSpan: 4
            value: filter.get('color')
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    filter.set('color', '' + value)
                    filter.set("disable", 0);
                }
            }
            onPickStarted: {
                filter.set("disable", 1);
            }
            onPickCancelled: filter.set('disable', 0)
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: sizeSlider
            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 200
            value: filter.getDouble('size')
            onValueChanged: filter.set('size', value)
        }
        UndoButton {
            onClicked: sizeSlider.value = 70
        }

        Label {
            text: qsTr('Depth')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: depthSlider
            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            value: filter.getDouble('depth') * 2
            onValueChanged: filter.set('depth', value * 0.5)
        }
        UndoButton {
            onClicked: depthSlider.value = 40
        }

        Label {
            text: qsTr('Tilt')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: tiltSlider
            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            value: filter.getDouble('x_rotation') * maximumValue
            onValueChanged: filter.set('x_rotation', value / maximumValue)
        }
        UndoButton {
            onClicked: tiltSlider.value = 50
        }

        Label {
            text: qsTr('Horizontal')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: horizontalSlider
            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            value: filter.getDouble('horizontal') * maximumValue
            onValueChanged: filter.set('horizontal', value / maximumValue)
        }
        UndoButton {
            onClicked: horizontalSlider.value = 50
        }

        Label {
            text: qsTr('Vertical')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: verticalSlider
            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            value: filter.getDouble('vertical') * maximumValue
            onValueChanged: filter.set('vertical', value / maximumValue)
        }
        UndoButton {
            onClicked: verticalSlider.value = 50
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
