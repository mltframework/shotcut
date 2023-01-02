/*
 * Copyright (c) 2017-2021 Meltytech, LLC
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
    property string paramShape: '0'
    property string paramHorizontal: '1'
    property string paramVertical: '2'
    property string paramWidth: '3'
    property string paramHeight: '4'
    property string paramRotation: '5'
    property string paramSoftness: '6'
    property string paramOperation: '9'
    property var defaultParameters: [paramHorizontal, paramShape, paramWidth, paramHeight, paramVertical, paramRotation, paramSoftness, paramOperation]
    property bool blockUpdate: true
    property var startValues: [0.5, 0.5, 0, 0]
    property var middleValues: [0.5, 0.5, 0.1, 0.1]
    property var endValues: [0.5, 0.5, 0, 0]

    function initSimpleAnimation() {
        middleValues = [filter.getDouble(paramHorizontal, filter.animateIn), filter.getDouble(paramVertical, filter.animateIn), filter.getDouble(paramWidth, filter.animateIn), filter.getDouble(paramHeight, filter.animateIn)];
        if (filter.animateIn > 0)
            startValues = [filter.getDouble(paramHorizontal, 0), filter.getDouble(paramVertical, 0), filter.getDouble(paramWidth, 0), filter.getDouble(paramHeight, 0)];
        if (filter.animateOut > 0)
            endValues = [filter.getDouble(paramHorizontal, filter.duration - 1), filter.getDouble(paramVertical, filter.duration - 1), filter.getDouble(paramWidth, filter.duration - 1), filter.getDouble(paramHeight, filter.duration - 1)];
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        horizontalSlider.value = filter.getDouble(paramHorizontal, position) * 100;
        horizontalKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(paramHorizontal) > 0;
        verticalSlider.value = filter.getDouble(paramVertical, position) * 100;
        verticalKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(paramVertical) > 0;
        widthSlider.value = filter.getDouble(paramWidth, position) * 100;
        widthKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(paramWidth) > 0;
        heightSlider.value = filter.getDouble(paramHeight, position) * 100;
        heightKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(paramHeight) > 0;
        blockUpdate = false;
        horizontalSlider.enabled = verticalSlider.enabled = widthSlider.enabled = heightSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
        operationCombo.currentIndex = Math.round(filter.getDouble(paramOperation) * 4);
        shapeCombo.currentIndex = Math.round(filter.getDouble(paramShape) * 3);
        rotationSlider.value = (filter.getDouble(paramRotation) - 0.5) * 360;
        softnessSlider.value = filter.getDouble(paramSoftness) * 100;
    }

    function updateFilter(parameter, value, position, button) {
        if (blockUpdate)
            return;
        var index = parseInt(parameter - 1);
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValues[index] = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValues[index] = value;
            else
                middleValues[index] = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(parameter);
            button.checked = false;
            if (filter.animateIn > 0) {
                filter.set(parameter, startValues[index], 0);
                filter.set(parameter, middleValues[index], filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set(parameter, middleValues[index], filter.duration - filter.animateOut);
                filter.set(parameter, endValues[index], filter.duration - 1);
            }
        } else if (!button.checked) {
            filter.resetProperty(parameter);
            filter.set(parameter, middleValues[index]);
        } else if (position !== null) {
            filter.set(parameter, value, position);
        }
    }

    function onKeyframesButtonClicked(checked, parameter, value) {
        if (checked) {
            blockUpdate = true;
            horizontalSlider.enabled = verticalSlider.enabled = widthSlider.enabled = heightSlider.enabled = true;
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                filter.resetProperty(paramHorizontal);
                filter.resetProperty(paramVertical);
                filter.resetProperty(paramWidth);
                filter.resetProperty(paramHeight);
                filter.animateIn = filter.animateOut = 0;
            } else {
                filter.clearSimpleAnimation(parameter);
            }
            blockUpdate = false;
            filter.set(parameter, value, getPosition());
        } else {
            filter.resetProperty(parameter);
            filter.set(parameter, value);
        }
    }

    function updatedSimpleAnimation() {
        updateFilter(paramHorizontal, horizontalSlider.value / 100, null, horizontalKeyframesButton);
        updateFilter(paramVertical, verticalSlider.value / 100, null, verticalKeyframesButton);
        updateFilter(paramWidth, widthSlider.value / 100, null, widthKeyframesButton);
        updateFilter(paramHeight, heightSlider.value / 100, null, heightKeyframesButton);
    }

    width: 350
    height: 250
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set(paramOperation, 0);
            filter.set(paramShape, 0);
            filter.set(paramHorizontal, 0.5);
            filter.set(paramVertical, 0.5);
            filter.set(paramWidth, 0.1);
            filter.set(paramHeight, 0.1);
            filter.set(paramRotation, 0.5);
            filter.set(paramSoftness, 0.2);
            filter.savePreset(defaultParameters);
        } else {
            initSimpleAnimation();
        }
        setControls();
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            Layout.columnSpan: 3
            parameters: defaultParameters
            onBeforePresetLoaded: {
                filter.resetProperty(paramHorizontal);
                filter.resetProperty(paramVertical);
                filter.resetProperty(paramWidth);
                filter.resetProperty(paramHeight);
            }
            onPresetSelected: {
                setControls();
                initSimpleAnimation();
            }
        }

        Label {
            text: qsTr('Operation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: operationCombo

            implicitWidth: 180
            model: [qsTr('Overwrite'), qsTr('Maximum'), qsTr('Minimum'), qsTr('Add'), qsTr('Subtract')]
            onActivated: filter.set(paramOperation, currentIndex / 4)
        }

        Shotcut.UndoButton {
            Layout.columnSpan: 2
            onClicked: {
                filter.set(paramOperation, 0);
                operationCombo.currentIndex = 0;
            }
        }

        Label {
            text: qsTr('Shape')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: shapeCombo

            implicitWidth: 180
            model: [qsTr('Rectangle'), qsTr('Ellipse'), qsTr('Triangle'), qsTr('Diamond')]
            onActivated: filter.set(paramShape, currentIndex / 3)
        }

        Shotcut.UndoButton {
            Layout.columnSpan: 2
            onClicked: {
                filter.set(paramShape, 0);
                shapeCombo.currentIndex = 0;
            }
        }

        Label {
            text: qsTr('Horizontal')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: horizontalSlider

            minimumValue: -100
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(paramHorizontal, value / 100, getPosition(), horizontalKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: horizontalSlider.value = 50
        }

        Shotcut.KeyframesButton {
            id: horizontalKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, paramHorizontal, horizontalSlider.value / 100)
        }

        Label {
            text: qsTr('Vertical')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: verticalSlider

            minimumValue: -100
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(paramVertical, value / 100, getPosition(), verticalKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: verticalSlider.value = 50
        }

        Shotcut.KeyframesButton {
            id: verticalKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, paramVertical, verticalSlider.value / 100)
        }

        Label {
            text: qsTr('Width')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: widthSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(paramWidth, value / 100, getPosition(), widthKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: widthSlider.value = 10
        }

        Shotcut.KeyframesButton {
            id: widthKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, paramWidth, widthSlider.value / 100)
        }

        Label {
            text: qsTr('Height')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: heightSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(paramHeight, value / 100, getPosition(), heightKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: heightSlider.value = 10
        }

        Shotcut.KeyframesButton {
            id: heightKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, paramHeight, heightSlider.value / 100)
        }

        Label {
            text: qsTr('Rotation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: rotationSlider

            minimumValue: -179.9
            maximumValue: 179.9
            decimals: 1
            spinnerWidth: 110
            suffix: qsTr(' deg', 'degrees')
            onValueChanged: filter.set(paramRotation, 0.5 + value / 360)
        }

        Shotcut.UndoButton {
            Layout.columnSpan: 2
            onClicked: rotationSlider.value = 0
        }

        Label {
            text: qsTr('Softness')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: softnessSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 2
            suffix: ' %'
            onValueChanged: filter.set(paramSoftness, value / 100)
        }

        Shotcut.UndoButton {
            Layout.columnSpan: 2
            onClicked: softnessSlider.value = 20
        }

        Item {
            Layout.columnSpan: 2
            Layout.fillHeight: true
        }
    }

    Connections {
        function onChanged() {
            setControls();
        }

        function onInChanged() {
            updatedSimpleAnimation();
        }

        function onOutChanged() {
            updatedSimpleAnimation();
        }

        function onAnimateInChanged() {
            updatedSimpleAnimation();
        }

        function onAnimateOutChanged() {
            updatedSimpleAnimation();
        }

        function onPropertyChanged(name) {
            setControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                setControls();
            } else {
                blockUpdate = true;
                horizontalSlider.value = filter.getDouble(paramHorizontal, getPosition()) * 100;
                verticalSlider.value = filter.getDouble(paramVertical, getPosition()) * 100;
                widthSlider.value = filter.getDouble(paramWidth, getPosition()) * 100;
                heightSlider.value = filter.getDouble(paramHeight, getPosition()) * 100;
                blockUpdate = false;
                horizontalSlider.enabled = verticalSlider.enabled = widthSlider.enabled = heightSlider.enabled = true;
            }
        }

        target: producer
    }
}
