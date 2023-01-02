/*
 * Copyright (c) 2018-2021 Meltytech, LLC
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
    property string channelParam: '0'
    property string inputBlackParam: '1'
    property string inputWhiteParam: '2'
    property string gammaParam: '3'
    property string outputBlackParam: '4'
    property string outputWhiteParam: '5'
    property string showHistogramParam: '6'
    property string histogramPositionParam: '7'
    property bool blockUpdate: true
    property var startValues: [0, 1, 0.25] // inputBlackParam, inputWhiteParam, gammaParam
    property var middleValues: [0, 1, 0.25]
    property var endValues: [0, 1, 0.25]

    function initSimpleAnimation() {
        middleValues = [filter.getDouble(inputBlackParam, filter.animateIn), filter.getDouble(inputWhiteParam, filter.animateIn), filter.getDouble(gammaParam, filter.animateIn)];
        if (filter.animateIn > 0)
            startValues = [filter.getDouble(inputBlackParam, 0), filter.getDouble(inputWhiteParam, 0), filter.getDouble(gammaParam, 0)];
        if (filter.animateOut > 0)
            endValues = [filter.getDouble(inputBlackParam, filter.duration - 1), filter.getDouble(inputWhiteParam, filter.duration - 1), filter.getDouble(gammaParam, filter.duration - 1)];
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setKeyframedControls() {
        var position = getPosition();
        blockUpdate = true;
        inputBlackSlider.value = filter.getDouble(inputBlackParam, position) * inputBlackSlider.maximumValue;
        inputBlackKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(inputBlackParam) > 0;
        inputWhiteSlider.value = filter.getDouble(inputWhiteParam, position) * inputWhiteSlider.maximumValue;
        inputWhiteKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(inputWhiteParam) > 0;
        gammaSlider.value = filter.getDouble(gammaParam, position) * gammaSlider.maximumValue;
        gammaKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(gammaParam) > 0;
        blockUpdate = false;
        inputBlackSlider.enabled = inputWhiteSlider.enabled = gammaSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function setControls() {
        setKeyframedControls();
        channelCombo.currentIndex = Math.round(filter.getDouble(channelParam) * 10);
        histogramCombo.currentIndex = (filter.getDouble(showHistogramParam) === 1) ? Math.round(filter.getDouble(histogramPositionParam) * 10) : 4;
        outputBlackSlider.value = filter.getDouble(outputBlackParam) * outputBlackSlider.maximumValue;
        outputWhiteSlider.value = filter.getDouble(outputWhiteParam) * outputWhiteSlider.maximumValue;
    }

    function updateFilter(parameter, value, position, button) {
        if (blockUpdate)
            return;
        var index = preset.parameters.indexOf(parameter) - 1;
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
            inputBlackSlider.enabled = inputWhiteSlider.enabled = gammaSlider.enabled = true;
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                filter.resetProperty(inputBlackParam);
                filter.resetProperty(inputWhiteParam);
                filter.resetProperty(gammaParam);
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

    function updateSimpleAnimation() {
        setKeyframedControls();
        updateFilter(inputBlackParam, inputBlackSlider.value / inputBlackSlider.maximumValue, null, inputBlackKeyframesButton);
        updateFilter(inputWhiteParam, inputWhiteSlider.value / inputWhiteSlider.maximumValue, null, inputWhiteKeyframesButton);
        updateFilter(gammaParam, gammaSlider.value / gammaSlider.maximumValue, null, gammaKeyframesButton);
    }

    width: 350
    height: 225
    Component.onCompleted: {
        filter.set('threads', filter.getDouble(showHistogramParam) === 1);
        if (filter.isNew) {
            // Set default parameter values
            filter.set(channelParam, 3 / 10);
            filter.set(inputBlackParam, 0);
            filter.set(inputWhiteParam, 1);
            filter.set(gammaParam, 0.25);
            filter.set(outputBlackParam, 0);
            filter.set(outputWhiteParam, 1);
            filter.savePreset(preset.parameters);
            filter.set(showHistogramParam, 0);
            filter.set(histogramPositionParam, 3 / 10);
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
            id: preset

            Layout.columnSpan: parent.columns - 1
            parameters: [channelParam, inputBlackParam, inputWhiteParam, gammaParam, outputBlackParam, outputWhiteParam]
            onBeforePresetLoaded: {
                filter.resetProperty(inputBlackParam);
                filter.resetProperty(inputWhiteParam);
                filter.resetProperty(gammaParam);
            }
            onPresetSelected: {
                setControls();
                initSimpleAnimation();
            }
        }

        Label {
            text: qsTr('Channel')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: channelCombo

            model: [qsTr('Red'), qsTr('Green'), qsTr('Blue'), qsTr('Value')]
            onActivated: filter.set(channelParam, currentIndex / 10)
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(channelParam, 3 / 10);
                channelCombo.currentIndex = 3;
            }
        }

        Item {
            width: 1
        }

        Label {
            text: qsTr('Histogram')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: histogramCombo

            model: [qsTr('Top Left'), qsTr('Top Right'), qsTr('Bottom Left'), qsTr('Bottom Right'), qsTr('None')]
            onActivated: {
                filter.set(showHistogramParam, currentIndex < 4);
                filter.set('threads', filter.getDouble(showHistogramParam) === 1);
                if (currentIndex < 4)
                    filter.set(histogramPositionParam, currentIndex / 10);
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(showHistogramParam, 0);
                filter.set('threads', 0);
                histogramCombo.currentIndex = 4;
            }
        }

        Item {
            width: 1
        }

        Label {
            text: qsTr('Input Black')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: inputBlackSlider

            minimumValue: 0
            maximumValue: 255
            decimals: 1
            onValueChanged: updateFilter(inputBlackParam, value / maximumValue, getPosition(), inputBlackKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: inputBlackSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: inputBlackKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, inputBlackParam, inputBlackSlider.value / inputBlackSlider.maximumValue)
        }

        Label {
            text: qsTr('Input White')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: inputWhiteSlider

            minimumValue: 0
            maximumValue: 255
            decimals: 1
            onValueChanged: updateFilter(inputWhiteParam, value / maximumValue, getPosition(), inputWhiteKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: inputWhiteSlider.value = 255
        }

        Shotcut.KeyframesButton {
            id: inputWhiteKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, inputWhiteParam, inputWhiteSlider.value / inputWhiteSlider.maximumValue)
        }

        Label {
            text: qsTr('Gamma')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: gammaSlider

            minimumValue: 0.01
            maximumValue: 4
            decimals: 2
            onValueChanged: updateFilter(gammaParam, value / maximumValue, getPosition(), gammaKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: gammaSlider.value = 1
        }

        Shotcut.KeyframesButton {
            id: gammaKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, gammaParam, gammaSlider.value / gammaSlider.maximumValue)
        }

        Label {
            text: qsTr('Output Black')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: outputBlackSlider

            minimumValue: 0
            maximumValue: 255
            onValueChanged: filter.set(outputBlackParam, value / maximumValue)
        }

        Shotcut.UndoButton {
            onClicked: outputBlackSlider.value = 0
        }

        Item {
            width: 1
        }

        Label {
            text: qsTr('Output White')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: outputWhiteSlider

            minimumValue: 0
            maximumValue: 255
            onValueChanged: filter.set(outputWhiteParam, value / maximumValue)
        }

        Shotcut.UndoButton {
            onClicked: outputWhiteSlider.value = 255
        }

        Item {
            width: 1
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        function onChanged() {
            setKeyframedControls();
        }

        function onInChanged() {
            updateSimpleAnimation();
        }

        function onOutChanged() {
            updateSimpleAnimation();
        }

        function onAnimateInChanged() {
            updateSimpleAnimation();
        }

        function onAnimateOutChanged() {
            updateSimpleAnimation();
        }

        function onPropertyChanged() {
            setKeyframedControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setKeyframedControls();
        }

        target: producer
    }
}
