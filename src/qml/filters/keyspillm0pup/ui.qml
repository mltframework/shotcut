/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

Item {
    property string keyColorParam: '0'
    property string keyColorDefault: '#19cc19'
    property string targetColorParam: '1'
    property string targetColorDefault: '#c67f66'
    property string maskTypeParam: '2'
    property string maskTypeDefault: '0'
    property string toleranceParam: '3'
    property double toleranceDefault: 0.24
    property string slopeParam: '4'
    property double slopeDefault: 0.4
    property string hueGateParam: '5'
    property double hueGateDefault: 0.25
    property string saturationParam: '6'
    property double saturationDefault: 0.15
    property string operation1Param: '7'
    property string operation1Default: '1'
    property string amount1Param: '8'
    property double amount1Default: 0.5
    property string operation2Param: '9'
    property string operation2Default: '0'
    property string amount2Param: '10'
    property double amount2Default: 0.5
    property string showMaskParam: '11'
    property bool showMaskDefault: false
    property string maskAlphaParam: '12'
    property bool maskAlphaDefault: false
    property var defaultParameters: [keyColorParam, targetColorParam, maskTypeParam, toleranceParam, slopeParam, hueGateParam, saturationParam, operation1Param, amount1Param, operation2Param, amount2Param]

    function setControls() {
        keyColorPicker.value = filter.get(keyColorParam);
        targetColorPicker.value = filter.get(targetColorParam);
        maskTypeCombo.currentIndex = filter.get(maskTypeParam);
        toleranceSlider.value = filter.getDouble(toleranceParam) * 100;
        slopeSlider.value = filter.getDouble(slopeParam) * 100;
        hueGateSlider.value = filter.getDouble(hueGateParam) * 100;
        saturationSlider.value = filter.getDouble(saturationParam) * 100;
        operation1Combo.currentIndex = filter.get(operation1Param);
        amount1Slider.value = filter.getDouble(amount1Param) * 100;
        operation2Combo.currentIndex = filter.get(operation2Param);
        amount2Slider.value = filter.getDouble(amount2Param) * 100;
        showMaskCheckbox.checked = parseInt(filter.get(showMaskParam));
        maskAlphaCheckbox.checked = parseInt(filter.get(maskAlphaParam));
    }

    width: 200
    height: 380
    Component.onCompleted: {
        filter.set('threads', 0);
        if (filter.isNew) {
            filter.set(keyColorParam, keyColorDefault);
            filter.set(targetColorParam, targetColorDefault);
            filter.set(maskTypeParam, maskTypeDefault);
            filter.set(toleranceParam, toleranceDefault);
            filter.set(slopeParam, slopeDefault);
            filter.set(hueGateParam, hueGateDefault);
            filter.set(saturationParam, saturationDefault);
            filter.set(operation1Param, operation1Default);
            filter.set(amount1Param, amount1Default);
            filter.set(operation2Param, operation2Default);
            filter.set(amount2Param, amount2Default);
            filter.set(showMaskParam, showMaskDefault);
            filter.set(maskAlphaParam, maskAlphaDefault);
            filter.savePreset(defaultParameters);
        }
        setControls();
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
            id: presetItem

            Layout.columnSpan: 2
            parameters: defaultParameters
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Key color')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ColorPicker {
            id: keyColorPicker

            property bool isReady: false

            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    filter.set(keyColorParam, value);
                    filter.set("disable", 0);
                }
            }
            onPickStarted: {
                filter.set("disable", 1);
            }
            onPickCancelled: filter.set('disable', 0)
        }

        Shotcut.UndoButton {
            onClicked: keyColorPicker.value = keyColorDefault
        }

        Label {
            text: qsTr('Target color')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ColorPicker {
            id: targetColorPicker

            property bool isReady: false

            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    filter.set(targetColorParam, value);
                    filter.set("disable", 0);
                }
            }
            onPickStarted: {
                filter.set("disable", 1);
            }
            onPickCancelled: filter.set('disable', 0)
        }

        Shotcut.UndoButton {
            onClicked: targetColorPicker.value = targetColorDefault
        }

        Label {
            text: qsTr('Mask type')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: maskTypeCombo

            implicitWidth: 180
            model: [qsTr('Color Distance'), qsTr('Transparency'), qsTr('Edge Inwards'), qsTr('Edge Outwards')]
            onActivated: filter.set(maskTypeParam, currentIndex)
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(maskTypeParam, maskTypeDefault);
                maskTypeCombo.currentIndex = maskTypeDefault;
            }
        }

        Label {
            text: qsTr('Tolerance')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: toleranceSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(toleranceParam) * 100
            onValueChanged: filter.set(toleranceParam, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: toleranceSlider.value = toleranceDefault * 100
        }

        Label {
            text: qsTr('Slope')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slopeSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(slopeParam) * 100
            onValueChanged: filter.set(slopeParam, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: slopeSlider.value = slopeDefault * 100
        }

        Label {
            text: qsTr('Hue gate')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: hueGateSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(hueGateParam) * 100
            onValueChanged: filter.set(hueGateParam, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: hueGateSlider.value = hueGateDefault * 100
        }

        Label {
            text: qsTr('Saturation threshold')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: saturationSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(saturationParam) * 100
            onValueChanged: filter.set(saturationParam, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: saturationSlider.value = saturationDefault * 100
        }

        Label {
            text: qsTr('Operation 1')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: operation1Combo

            implicitWidth: 180
            model: [qsTr('None'), qsTr('De-Key'), qsTr('Desaturate'), qsTr('Adjust Luma')]
            onActivated: filter.set(operation1Param, currentIndex)
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(operation1Param, operation1Default);
                operation1Combo.currentIndex = operation1Default;
            }
        }

        Label {
            text: qsTr('Amount 1')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: amount1Slider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(amount1Param) * 100
            onValueChanged: filter.set(amount1Param, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: amount1Slider.value = amount1Default * 100
        }

        Label {
            text: qsTr('Operation 2')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: operation2Combo

            implicitWidth: 180
            model: [qsTr('None'), qsTr('De-Key'), qsTr('Desaturate'), qsTr('Adjust Luma')]
            onActivated: filter.set(operation2Param, currentIndex)
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(operation2Param, operation2Default);
                operation2Combo.currentIndex = operation2Default;
            }
        }

        Label {
            text: qsTr('Amount 2')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: amount2Slider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(amount2Param) * 100
            onValueChanged: filter.set(amount2Param, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: amount2Slider.value = amount2Default * 100
        }

        Label {}

        CheckBox {
            id: showMaskCheckbox

            text: qsTr('Show mask')
            onCheckedChanged: filter.set(showMaskParam, checked)
        }

        Shotcut.UndoButton {
            onClicked: showMaskCheckbox.checked = showMaskDefault
        }

        Label {}

        CheckBox {
            id: maskAlphaCheckbox

            text: qsTr('Send mask to alpha channel')
            onCheckedChanged: filter.set(maskAlphaParam, checked)
        }

        Shotcut.UndoButton {
            onClicked: maskAlphaCheckbox.checked = maskAlphaDefault
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
