/*
 * Copyright (c) 2015-2021 Meltytech, LLC
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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    property string keyColorParam: '0'
    property string keyColorDefault: '#00cc00'
    property string invertParam: '1'
    property bool invertDefault: true
    property string deltaRParam: '2'
    property double deltaRDefault: 0.2
    property string deltaGParam: '3'
    property double deltaGDefault: 0.2
    property string deltaBParam: '4'
    property double deltaBDefault: 0.2
    property string slopeParam: '5'
    property double slopeDefault: 0
    property string colorspaceParam: '6'
    property double colorspaceDefault: 0
    property string shapeParam: '7'
    property double shapeDefault: 0.5
    property string edgeParam: '8'
    property double edgeDefault: 0.9
    property string operationParam: '9'
    property double operationDefault: 0
    property var defaultParameters: [keyColorParam, invertParam, deltaRParam, deltaGParam, deltaBParam, slopeParam, colorspaceParam, shapeParam, edgeParam, operationParam]

    function setControls() {
        keyColorPicker.value = filter.get(keyColorParam);
        if (filter.getDouble(colorspaceParam) === 0)
            rgbRadioButton.checked = true;
        else
            hciRadioButton.checked = true;
        deltaRSlider.value = filter.getDouble(deltaRParam) * 100;
        deltaGSlider.value = filter.getDouble(deltaGParam) * 100;
        deltaBSlider.value = filter.getDouble(deltaBParam) * 100;
        var currentShape = filter.getDouble(shapeParam);
        for (var i = 0; i < shapeModel.count; ++i) {
            if (shapeModel.get(i).value === currentShape) {
                shapeCombo.currentIndex = i;
                break;
            }
        }
        var currentEdge = filter.getDouble(edgeParam);
        for (var i = 0; i < edgeModel.count; ++i) {
            if (edgeModel.get(i).value === currentEdge) {
                edgeCombo.currentIndex = i;
                break;
            }
        }
        slopeSlider.value = filter.getDouble(slopeParam) * 100;
        var currentOp = filter.getDouble(operationParam);
        for (var i = 0; i < operationModel.count; ++i) {
            if (operationModel.get(i).value === currentOp) {
                operationCombo.currentIndex = i;
                break;
            }
        }
        invertCheckbox.checked = !parseInt(filter.get(invertParam));
    }

    width: 200
    height: 300
    Component.onCompleted: {
        filter.set('threads', 0);
        if (filter.isNew) {
            filter.set(keyColorParam, keyColorDefault);
            filter.set(invertParam, invertDefault);
            filter.set(deltaRParam, deltaRDefault);
            filter.set(deltaGParam, deltaGDefault);
            filter.set(deltaBParam, deltaBDefault);
            filter.set(slopeParam, slopeDefault);
            filter.set(colorspaceParam, colorspaceDefault);
            filter.set(shapeParam, shapeDefault);
            filter.set(edgeParam, edgeDefault);
            filter.set(operationParam, operationDefault);
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
            text: qsTr('Color space')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            ButtonGroup {
                id: colorspaceGroup
            }

            RadioButton {
                id: rgbRadioButton

                text: qsTr('Red-Green-Blue')
                ButtonGroup.group: colorspaceGroup
                onCheckedChanged: {
                    if (checked)
                        filter.set(colorspaceParam, 0);

                }
            }

            RadioButton {
                id: hciRadioButton

                text: qsTr('Hue-Chroma-Intensity')
                ButtonGroup.group: colorspaceGroup
                onCheckedChanged: {
                    if (checked)
                        filter.set(colorspaceParam, 1);

                }
            }

        }

        Shotcut.UndoButton {
            onClicked: rgbRadioButton.checked = true
        }

        Label {
            text: rgbRadioButton.checked ? qsTr('Red delta') : qsTr('Hue delta')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: deltaRSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(deltaRDefault) * 100
            onValueChanged: filter.set(deltaRParam, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: deltaRSlider.value = deltaRDefault * 100
        }

        Label {
            text: rgbRadioButton.checked ? qsTr('Green delta') : qsTr('Chroma delta')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: deltaGSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(deltaGDefault) * 100
            onValueChanged: filter.set(deltaGParam, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: deltaGSlider.value = deltaGDefault * 100
        }

        Label {
            text: rgbRadioButton.checked ? qsTr('Blue delta') : qsTr('Intensity delta')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: deltaBSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(deltaBDefault) * 100
            onValueChanged: filter.set(deltaBParam, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: deltaBSlider.value = deltaBDefault * 100
        }

        Label {
            text: qsTr('Shape')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: shapeCombo

            implicitWidth: 180
            textRole: 'text'
            onActivated: filter.set(shapeParam, shapeModel.get(currentIndex).value)

            model: ListModel {
                id: shapeModel

                ListElement {
                    text: qsTr('Box')
                    value: 0
                }

                ListElement {
                    text: qsTr('Ellipsoid')
                    value: 0.5
                }

                ListElement {
                    text: qsTr('Diamond')
                    value: 1
                }

            }

        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(shapeParam, shapeModel.get(1).value);
                shapeCombo.currentIndex = 1;
            }
        }

        Label {
            text: qsTr('Edge')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: edgeCombo

            implicitWidth: 180
            textRole: 'text'
            onActivated: filter.set(edgeParam, edgeModel.get(currentIndex).value)

            model: ListModel {
                id: edgeModel

                ListElement {
                    text: qsTr('Hard', 'Chroma Key Advanced filter')
                    value: 0
                }

                ListElement {
                    text: qsTr('Fat')
                    value: 0.35
                }

                ListElement {
                    text: qsTr('Normal')
                    value: 0.6
                }

                ListElement {
                    text: qsTr('Thin')
                    value: 0.7
                }

                ListElement {
                    text: qsTr('Slope')
                    value: 0.9
                }

            }

        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(edgeParam, edgeModel.get(4).value);
                edgeCombo.currentIndex = 4;
            }
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
            text: qsTr('Operation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: operationCombo

            implicitWidth: 180
            textRole: 'text'
            onActivated: filter.set(operationParam, operationModel.get(currentIndex).value)

            model: ListModel {
                id: operationModel

                ListElement {
                    text: qsTr('Overwrite')
                    value: 0
                }

                ListElement {
                    text: qsTr('Maximum')
                    value: 0.3
                }

                ListElement {
                    text: qsTr('Minimum')
                    value: 0.5
                }

                ListElement {
                    text: qsTr('Add')
                    value: 0.7
                }

                ListElement {
                    text: qsTr('Subtract')
                    value: 1
                }

            }

        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(operationParam, operationModel.get(0).value);
                operationCombo.currentIndex = 0;
            }
        }

        Rectangle {
            width: 100
            color: 'transparent'
        }

        CheckBox {
            id: invertCheckbox

            text: qsTr('Invert')
            onCheckedChanged: filter.set(invertParam, !checked)
        }

        Shotcut.UndoButton {
            onClicked: invertCheckbox.checked = !invertDefault
        }

        Item {
            Layout.fillHeight: true
        }

    }

}
