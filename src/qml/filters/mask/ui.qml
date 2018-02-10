/*
 * Copyright (c) 2017-2018 Meltytech, LLC
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
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    property string paramShape: '0'
    property string paramHorizontal: '1'
    property string paramVertical: '2'
    property string paramWidth: '3'
    property string paramHeight: '4'
    property string paramRotation: '5'
    property string paramSoftness: '6'
    property string paramOperation: '9'
    property var defaultParameters: [paramHorizontal, paramShape, paramWidth, paramVertical, paramRotation, paramSoftness, paramOperation]
    width: 350
    height: 250

    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set(paramOperation, 0)
            filter.set(paramShape, 0)
            filter.set(paramHorizontal, 0.5)
            filter.set(paramVertical, 0.5)
            filter.set(paramWidth, 0.1)
            filter.set(paramHeight, 0.1)
            filter.set(paramRotation, 0)
            filter.set(paramSoftness, 0.2)
            filter.savePreset(defaultParameters)
        }
        setControls()
    }

    function setControls() {
        operationCombo.currentIndex = Math.round(filter.getDouble(paramOperation) * 4)
        shapeCombo.currentIndex = Math.round(filter.getDouble(paramShape) * 3)
        horizontalSlider.value = filter.getDouble(paramHorizontal) * 100
        verticalSlider.value = filter.getDouble(paramVertical) * 100
        widthSlider.value = filter.getDouble(paramWidth) * 100
        heightSlider.value = filter.getDouble(paramHeight) * 100
        rotationSlider.value = filter.getDouble(paramRotation) * 100
        softnessSlider.value = filter.getDouble(paramSoftness) * 100
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            Layout.columnSpan: 2
            parameters: defaultParameters
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Operation')
            Layout.alignment: Qt.AlignRight
        }
        ComboBox {
            id: operationCombo
            implicitWidth: 180
            model: [qsTr('Write on Clear'), qsTr('Maximum'), qsTr('Minimum'), qsTr('Add'), qsTr('Subtract')]
            onCurrentIndexChanged: filter.set(paramOperation, currentIndex / 4)
        }
        UndoButton {
            onClicked: operationCombo.currentIndex = 0
        }

        Label {
            text: qsTr('Shape')
            Layout.alignment: Qt.AlignRight
        }
        ComboBox {
            id: shapeCombo
            implicitWidth: 180
            model: [qsTr('Rectangle'), qsTr('Ellipse'), qsTr('Triangle'), qsTr('Diamond')]
            onCurrentIndexChanged: filter.set(paramShape, currentIndex / 3)
        }
        UndoButton {
            onClicked: shapeCombo.currentIndex = 0
        }

        Label {
            text: qsTr('Horizontal')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: horizontalSlider
            minimumValue: -100
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            value: filter.get(paramHorizontal) * 100
            onValueChanged: filter.set(paramHorizontal, value / 100)
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
            minimumValue: -100
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            value: filter.get(paramVertical) * 100
            onValueChanged: filter.set(paramVertical, value / 100)
        }
        UndoButton {
            onClicked: verticalSlider.value = 50
        }

        Label {
            text: qsTr('Width')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: widthSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            value: filter.get(paramWidth) * 100
            onValueChanged: filter.set(paramWidth, value / 100)
        }
        UndoButton {
            onClicked: widthSlider.value = 10
        }

        Label {
            text: qsTr('Height')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: heightSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            value: filter.get(paramHeight) * 100
            onValueChanged: filter.set(paramHeight, value / 100)
        }
        UndoButton {
            onClicked: heightSlider.value = 10
        }

        Label {
            text: qsTr('Rotation')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: rotationSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            value: filter.getDouble(paramRotation) * 100
            onValueChanged: filter.set(paramRotation, value / 100)
        }
        UndoButton {
            onClicked: rotationSlider.value = 0
        }

        Label {
            text: qsTr('Softness')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: softnessSlider
            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            value: filter.getDouble(paramSoftness) * 100
            onValueChanged: filter.set(paramSoftness, value / 100)
        }
        UndoButton {
            onClicked: softnessSlider.value = 20
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}
