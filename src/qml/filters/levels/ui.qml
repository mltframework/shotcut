/*
 * Copyright (c) 2018-2020 Meltytech, LLC
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
import QtQuick.Controls 2.12 as Controls2
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    width: 350
    height: 225
    property string channelParam: '0'
    property string inputBlackParam: '1'
    property string inputWhiteParam: '2'
    property string gammaParam: '3'
    property string outputBlackParam: '4'
    property string outputWhiteParam: '5'
    property string showHistogramParam: '6'
    property string histogramPositionParam: '7'
    property bool blockUpdate: true
    property var startValues:  [0, 1, 0.25] // inputBlackParam, inputWhiteParam, gammaParam
    property var middleValues: [0, 1, 0.25]
    property var endValues:    [0, 1, 0.25]

    Component.onCompleted: {
        filter.set('threads', filter.getDouble(showHistogramParam) === 1)
        if (filter.isNew) {
            // Set default parameter values
            filter.set(channelParam, 3/10)
            filter.set(inputBlackParam, 0)
            filter.set(inputWhiteParam, 1)
            filter.set(gammaParam, 0.25)
            filter.set(outputBlackParam, 0)
            filter.set(outputWhiteParam, 1)
            filter.savePreset(preset.parameters)
            filter.set(showHistogramParam, 0)
            filter.set(histogramPositionParam, 3 / 10)
        } else {
            initSimpleAnimation()
        }
        setControls()
    }

    function initSimpleAnimation() {
        middleValues = [filter.getDouble(inputBlackParam, filter.animateIn),
                        filter.getDouble(inputWhiteParam, filter.animateIn),
                        filter.getDouble(gammaParam, filter.animateIn)]
        if (filter.animateIn > 0) {
            startValues = [filter.getDouble(inputBlackParam, 0),
                           filter.getDouble(inputWhiteParam, 0),
                           filter.getDouble(gammaParam, 0)]
        }
        if (filter.animateOut > 0) {
            endValues = [filter.getDouble(inputBlackParam, filter.duration - 1),
                         filter.getDouble(inputWhiteParam, filter.duration - 1),
                         filter.getDouble(gammaParam, filter.duration - 1)]
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setKeyframedControls() {
        var position = getPosition()
        blockUpdate = true
        inputBlackSlider.value = filter.getDouble(inputBlackParam, position) * inputBlackSlider.maximumValue
        inputWhiteSlider.value = filter.getDouble(inputWhiteParam, position) * inputWhiteSlider.maximumValue
        gammaSlider.value = filter.getDouble(gammaParam, position) * gammaSlider.maximumValue
        blockUpdate = false
        inputBlackSlider.enabled = inputWhiteSlider.enabled = gammaSlider.enabled
            = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
    }

    function setControls() {
        setKeyframedControls()
        channelCombo.currentIndex = Math.round(filter.getDouble(channelParam) * 10)
        histogramCombo.currentIndex = (filter.getDouble(showHistogramParam) === 1) ? Math.round(filter.getDouble(histogramPositionParam) * 10) : 4
        outputBlackSlider.value = filter.getDouble(outputBlackParam) * outputBlackSlider.maximumValue
        outputWhiteSlider.value = filter.getDouble(outputWhiteParam) * outputWhiteSlider.maximumValue
    }

    function updateFilter(parameter, value, position, button) {
        if (blockUpdate) return
        var index = preset.parameters.indexOf(parameter) - 1

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValues[index] = value
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValues[index] = value
            else
                middleValues[index] = value
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(parameter)
            button.checked = false
            if (filter.animateIn > 0) {
                filter.set(parameter, startValues[index], 0)
                filter.set(parameter, middleValues[index], filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set(parameter, middleValues[index], filter.duration - filter.animateOut)
                filter.set(parameter, endValues[index], filter.duration - 1)
            }
        } else if (!button.checked) {
            filter.resetProperty(parameter)
            filter.set(parameter, middleValues[index])
        } else if (position !== null) {
            filter.set(parameter, value, position)
        }
    }

    function onKeyframesButtonClicked(checked, parameter, value) {
        if (checked) {
            blockUpdate = true
            inputBlackSlider.enabled = inputWhiteSlider.enabled = gammaSlider.enabled = true
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                filter.resetProperty(inputBlackParam)
                filter.resetProperty(inputWhiteParam)
                filter.resetProperty(gammaParam)
                filter.animateIn = filter.animateOut = 0
            } else {
                filter.clearSimpleAnimation(parameter)
            }
            blockUpdate = false
            filter.set(parameter, value, getPosition())
        } else {
            filter.resetProperty(parameter)
            filter.set(parameter, value)
        }
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            Layout.columnSpan: parent.columns - 1
            parameters: [channelParam, inputBlackParam, inputWhiteParam, gammaParam, outputBlackParam, outputWhiteParam]
            onBeforePresetLoaded: {
                filter.resetProperty(inputBlackParam)
                filter.resetProperty(inputWhiteParam)
                filter.resetProperty(gammaParam)
            }
            onPresetSelected: {
                setControls()
                initSimpleAnimation()
            }
        }

        Label {
            text: qsTr('Channel')
            Layout.alignment: Qt.AlignRight
        }
        Controls2.ComboBox {
            id: channelCombo
            model: [qsTr('Red'), qsTr('Green'), qsTr('Blue'), qsTr('Value')]
            onActivated: filter.set(channelParam, currentIndex / 10)
        }
        UndoButton {
            onClicked: {
                filter.set(channelParam, 3 / 10)
                channelCombo.currentIndex = 3
            }
        }
        Item { width: 1 }

        Label {
            text: qsTr('Histogram')
            Layout.alignment: Qt.AlignRight
        }
        Controls2.ComboBox {
            id: histogramCombo
            model: [qsTr('Top Left'), qsTr('Top Right'), qsTr('Bottom Left'), qsTr('Bottom Right'), qsTr('None')]
            onActivated: {
                filter.set(showHistogramParam, currentIndex < 4)
                filter.set('threads', filter.getDouble(showHistogramParam) === 1)
                if (currentIndex < 4)
                    filter.set(histogramPositionParam, currentIndex / 10)
            }
        }
        UndoButton {
            onClicked: {
                filter.set(showHistogramParam, 0)
                filter.set('threads', 0)
                histogramCombo.currentIndex = 4
            }
        }
        Item { width: 1 }

        Label {
            text: qsTr('Input Black')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: inputBlackSlider
            minimumValue: 0
            maximumValue: 255
            decimals: 1
            onValueChanged: updateFilter(inputBlackParam, value / maximumValue, getPosition(), inputBlackKeyframesButton)
        }
        UndoButton {
            onClicked: inputBlackSlider.value = 0
        }
        KeyframesButton {
            id: inputBlackKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(inputBlackParam) > 0
            onToggled: onKeyframesButtonClicked(checked, inputBlackParam, inputBlackSlider.value / inputBlackSlider.maximumValue)
        }

        Label {
            text: qsTr('Input White')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: inputWhiteSlider
            minimumValue: 0
            maximumValue: 255
            decimals: 1
            onValueChanged: updateFilter(inputWhiteParam, value / maximumValue, getPosition(), inputWhiteKeyframesButton)
        }
        UndoButton {
            onClicked: inputWhiteSlider.value = 255
        }
        KeyframesButton {
            id: inputWhiteKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(inputWhiteParam) > 0
            onToggled: onKeyframesButtonClicked(checked, inputWhiteParam, inputWhiteSlider.value / inputWhiteSlider.maximumValue)
        }

        Label {
            text: qsTr('Gamma')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: gammaSlider
            minimumValue: 0.01
            maximumValue: 4
            decimals: 2
            onValueChanged: updateFilter(gammaParam, value / maximumValue, getPosition(), gammaKeyframesButton)
        }
        UndoButton {
            onClicked: gammaSlider.value = 1
        }
        KeyframesButton {
            id: gammaKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(gammaParam) > 0
            onToggled: onKeyframesButtonClicked(checked, gammaParam, gammaSlider.value / gammaSlider.maximumValue)
        }

        Label {
            text: qsTr('Output Black')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: outputBlackSlider
            minimumValue: 0
            maximumValue: 255
            onValueChanged: filter.set(outputBlackParam, value / maximumValue)
        }
        UndoButton {
            onClicked: outputBlackSlider.value = 0
        }
        Item { width: 1 }

        Label {
            text: qsTr('Output White')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: outputWhiteSlider
            minimumValue: 0
            maximumValue: 255
            onValueChanged: filter.set(outputWhiteParam, value / maximumValue)
        }
        UndoButton {
            onClicked: outputWhiteSlider.value = 255
        }
        Item { width: 1 }

        Item {
            Layout.fillHeight: true
        }
    }

    function updateSimpleAnimation() {
        updateFilter(inputBlackParam, inputBlackSlider.value / inputBlackSlider.maximumValue, getPosition(), inputBlackKeyframesButton)
        updateFilter(inputWhiteParam, inputWhiteSlider.value / inputWhiteSlider.maximumValue, getPosition(), inputWhiteKeyframesButton)
        updateFilter(gammaParam, gammaSlider.value / gammaSlider.maximumValue, getPosition(), gammaKeyframesButton)
    }

    Connections {
        target: filter
        onInChanged: updateSimpleAnimation()
        onOutChanged: updateSimpleAnimation()
        onAnimateInChanged: updateSimpleAnimation()
        onAnimateOutChanged: updateSimpleAnimation()
    }

    Connections {
        target: producer
        onPositionChanged: setKeyframedControls()
    }
}
