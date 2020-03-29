/*
 * Copyright (c) 2019-2020 Meltytech, LLC
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

import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0

KeyframableFilter {
    property string corner1x: '0'
    property string corner1y: '1'
    property string corner2x: '2'
    property string corner2y: '3'
    property string corner3x: '4'
    property string corner3y: '5'
    property string corner4x: '6'
    property string corner4y: '7'
    property string enablestretch: '8'
    property string stretchx: '9'
    property string stretchy: '10'
    property string interpolator: '11'
    property string transparentbackground: '12'
    property string featheralpha: '13'
    property string alphaoperation: '14'

    property double corner1xDefault: 1/3
    property double corner1yDefault: 1/3
    property double corner2xDefault: 2/3
    property double corner2yDefault: 1/3
    property double corner3xDefault: 2/3
    property double corner3yDefault: 2/3
    property double corner4xDefault: 1/3
    property double corner4yDefault: 2/3
    property double stretchxDefault: 0.5
    property double stretchyDefault: 0.5
    property double interpolatorDefault: 2/6
    property double featheralphaDefault: 0.01
    property double alphaoperationDefault: 0

    keyframableParameters: [corner1x, corner1y, corner2x, corner2y, corner3x, corner3y, corner4x, corner4y, stretchx, stretchy, featheralpha]
    startValues: [corner1xDefault, corner1yDefault, corner2xDefault, corner2yDefault, corner3xDefault, corner3yDefault, corner4xDefault, corner4yDefault, stretchxDefault, stretchyDefault, featheralphaDefault]
    middleValues: startValues
    endValues: startValues

    width: 350
    height: 450

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(corner1x, corner1xDefault)
            filter.set(corner1y, corner1yDefault)
            filter.set(corner2x, corner2xDefault)
            filter.set(corner2y, corner2yDefault)
            filter.set(corner3x, corner3xDefault)
            filter.set(corner3y, corner3yDefault)
            filter.set(corner4x, corner4xDefault)
            filter.set(corner4y, corner4yDefault)
            filter.set(enablestretch, 1)
            filter.set(stretchx, stretchxDefault)
            filter.set(stretchy, stretchyDefault)
            filter.set(interpolator, interpolatorDefault)
            filter.set(transparentbackground, 1)
            filter.set(alphaoperation, alphaoperationDefault)
            filter.set(featheralpha, featheralphaDefault)
            filter.savePreset(preset.parameters)
        } else {
            initializeSimpleKeyframes()
        }
        setControls()
    }

    function sliderValue(slider) {
        return (slider.value - slider.minimumValue) / (slider.maximumValue - slider.minimumValue)
    }

    function setSliderValue(slider, value) {
        slider.value = value * (slider.maximumValue - slider.minimumValue) + slider.minimumValue
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        setSliderValue(corner1xSlider, filter.getDouble(corner1x, position))
        setSliderValue(corner1ySlider, filter.getDouble(corner1y, position))
        setSliderValue(corner2xSlider, filter.getDouble(corner2x, position))
        setSliderValue(corner2ySlider, filter.getDouble(corner2y, position))
        setSliderValue(corner3xSlider, filter.getDouble(corner3x, position))
        setSliderValue(corner3ySlider, filter.getDouble(corner3y, position))
        setSliderValue(corner4xSlider, filter.getDouble(corner4x, position))
        setSliderValue(corner4ySlider, filter.getDouble(corner4y, position))
        stretchxSlider.value = (1.0 - filter.getDouble(stretchx, position)) * stretchxSlider.maximumValue
        stretchySlider.value = (1.0 - filter.getDouble(stretchy, position)) * stretchySlider.maximumValue
        interpolatorCombo.currentIndex = Math.round(filter.getDouble(interpolator) * 6)
        alphaoperationCombo.currentIndex = filter.get(transparentbackground) === '1'?
                    Math.round(filter.getDouble(alphaoperation) * 4) + 1 : 0
        featheralphaSlider.value = filter.getDouble(featheralpha, position) * featheralphaSlider.maximumValue
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        corner1xSlider.enabled = corner1ySlider.enabled = corner2xSlider.enabled = corner2ySlider.enabled = corner3xSlider.enabled = corner3ySlider.enabled = corner4xSlider.enabled = corner4ySlider.enabled = stretchxSlider.enabled = stretchySlider.enabled = featheralphaSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        updateFilter(corner1x, corner1xSlider.value / corner1xSlider.maximumValue, corner1xKeyframesButton)
        updateFilter(corner1y, corner1ySlider.value / corner1ySlider.maximumValue, corner1yKeyframesButton)
        updateFilter(corner2x, corner2xSlider.value / corner2xSlider.maximumValue, corner2xKeyframesButton)
        updateFilter(corner2y, corner2ySlider.value / corner2ySlider.maximumValue, corner2yKeyframesButton)
        updateFilter(corner3x, corner3xSlider.value / corner3xSlider.maximumValue, corner3xKeyframesButton)
        updateFilter(corner3y, corner3ySlider.value / corner3ySlider.maximumValue, corner3yKeyframesButton)
        updateFilter(corner4x, corner4xSlider.value / corner4xSlider.maximumValue, corner4xKeyframesButton)
        updateFilter(corner4y, corner4ySlider.value / corner4ySlider.maximumValue, corner4yKeyframesButton)
        updateFilter(stretchx, stretchxSlider.value / stretchxSlider.maximumValue, stretchxKeyframesButton)
        updateFilter(stretchy, stretchySlider.value / stretchySlider.maximumValue, stretchyKeyframesButton)
        updateFilter(featheralpha, featheralphaSlider.value / featheralphaSlider.maximumValue, featheralphaKeyframesButton)
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 4

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: [corner1x, corner1y, corner2x, corner2y, corner3x, corner3y, corner4x, corner4y, stretchx, stretchy, interpolator, transparentbackground, featheralpha, alphaoperation]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeysframes()
            }
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
            }
        }

        Label {
            text: qsTr('Corner 1 X')
            Layout.alignment: Qt.AlignRight

        }
        SliderSpinner {
            id: corner1xSlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(corner1x, sliderValue(corner1xSlider), corner1xKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: setSliderValue(corner1xSlider, corner1xDefault)
        }
        KeyframesButton {
            id: corner1xKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(corner1x) > 0
            onToggled: {
                toggleKeyframes(checked, corner1x, sliderValue(corner1xSlider))
                setControls()
            }
        }

        Label {
            text: qsTr('Corner 1 Y')
            Layout.alignment: Qt.AlignRight

        }
        SliderSpinner {
            id: corner1ySlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(corner1y, sliderValue(corner1ySlider), corner1yKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: setSliderValue(corner1ySlider, corner1yDefault)
        }
        KeyframesButton {
            id: corner1yKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(corner1y) > 0
            onToggled: {
                toggleKeyframes(checked, corner1y, sliderValue(corner1ySlider))
                setControls()
            }
        }

        Label {
            text: qsTr('Corner 2 X')
            Layout.alignment: Qt.AlignRight

        }
        SliderSpinner {
            id: corner2xSlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(corner2x, sliderValue(corner2xSlider), corner2xKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: setSliderValue(corner2xSlider, corner2xDefault)
        }
        KeyframesButton {
            id: corner2xKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(corner2x) > 0
            onToggled: {
                toggleKeyframes(checked, corner2x, sliderValue(corner2xSlider))
                setControls()
            }
        }

        Label {
            text: qsTr('Corner 2 Y')
            Layout.alignment: Qt.AlignRight

        }
        SliderSpinner {
            id: corner2ySlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(corner2y, sliderValue(corner2ySlider), corner2yKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: setSliderValue(corner2ySlider, corner2yDefault)
        }
        KeyframesButton {
            id: corner2yKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(corner2y) > 0
            onToggled: {
                toggleKeyframes(checked, corner2y, sliderValue(corner2ySlider))
                setControls()
            }
        }

 Label {
            text: qsTr('Corner 3 X')
            Layout.alignment: Qt.AlignRight

        }
        SliderSpinner {
            id: corner3xSlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(corner3x, sliderValue(corner3xSlider), corner3xKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: setSliderValue(corner3xSlider, corner3xDefault)
        }
        KeyframesButton {
            id: corner3xKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(corner3x) > 0
            onToggled: {
                toggleKeyframes(checked, corner3x, sliderValue(corner3xSlider))
                setControls()
            }
        }

        Label {
            text: qsTr('Corner 3 Y')
            Layout.alignment: Qt.AlignRight

        }
        SliderSpinner {
            id: corner3ySlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(corner3y, sliderValue(corner3ySlider), corner3yKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: setSliderValue(corner3ySlider, corner3yDefault)
        }
        KeyframesButton {
            id: corner3yKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(corner3y) > 0
            onToggled: {
                toggleKeyframes(checked, corner3y, sliderValue(corner3ySlider))
                setControls()
            }
        }

        Label {
            text: qsTr('Corner 4 X')
            Layout.alignment: Qt.AlignRight

        }
        SliderSpinner {
            id: corner4xSlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(corner4x, sliderValue(corner4xSlider), corner4xKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: setSliderValue(corner4xSlider, corner4xDefault)
        }
        KeyframesButton {
            id: corner4xKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(corner4x) > 0
            onToggled: {
                toggleKeyframes(checked, corner4x, sliderValue(corner4xSlider))
                setControls()
            }
        }

        Label {
            text: qsTr('Corner 4 Y')
            Layout.alignment: Qt.AlignRight

        }
        SliderSpinner {
            id: corner4ySlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(corner4y, sliderValue(corner4ySlider), corner4yKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: setSliderValue(corner4ySlider, corner4yDefault)
        }
        KeyframesButton {
            id: corner4yKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(corner4y) > 0
            onToggled: {
                toggleKeyframes(checked, corner4y, sliderValue(corner4ySlider))
                setControls()
            }
        }

        Label {
            text: qsTr('Stretch X')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: stretchxSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(stretchx, 1.0 - stretchxSlider.value / stretchxSlider.maximumValue, stretchxKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: stretchxSlider.value = stretchxDefault * stretchxSlider.maximumValue
        }
        KeyframesButton {
            id: stretchxKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(stretchx) > 0
            onToggled: {
                toggleKeyframes(checked, stretchx, 1.0 - stretchxSlider.value / stretchxSlider.maximumValue)
                setControls()
            }
        }

        Label {
            text: qsTr('Stretch Y')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: stretchySlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(stretchy, 1.0 - stretchySlider.value / stretchySlider.maximumValue, stretchyKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: stretchySlider.value = stretchyDefault * stretchySlider.maximumValue
        }
        KeyframesButton {
            id: stretchyKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(stretchy) > 0
            onToggled: {
                toggleKeyframes(checked, stretchy, 1.0 - stretchySlider.value / stretchySlider.maximumValue)
                setControls()
            }
        }

        Label {
            text: qsTr('Interpolator')
            Layout.alignment: Qt.AlignRight
        }
        ComboBox {
            id: interpolatorCombo
            implicitWidth: 180
            model: [qsTr('Nearest Neighbor'), qsTr('Bilinear'), qsTr('Bicubic Smooth'), qsTr('Bicubic Sharp'), qsTr('Spline 4x4'), qsTr('Spline 6x6'), qsTr('Lanzcos')]
            onActivated: {
                enabled = false
                filter.set(interpolator, index / 6)
                enabled = true
            }
        }
        UndoButton {
            onClicked: interpolatorCombo.currentIndex = interpolatorDefault * 6
            Layout.columnSpan: 2
        }

        Label {
            text: qsTr('Alpha Operation')
            Layout.alignment: Qt.AlignRight
        }
        ComboBox {
            id: alphaoperationCombo
            implicitWidth: 180
            model: [qsTr('Opaque'), qsTr('Overwrite'), qsTr('Maximum'), qsTr('Minimum'), qsTr('Add'), qsTr('Subtract')]
            onActivated: {
                enabled = false
                filter.set(transparentbackground, index > 0)
                filter.set(alphaoperation, (index - 1) / 4)
                enabled = true
            }
        }
        UndoButton {
            Layout.columnSpan: 2
            onClicked: alphaoperationCombo.currentIndex = filter.get(transparentBackground) === '1'?
                           Math.round(alphaoperationDefault * 4) + 1 : 0
        }

        Label {
            text: qsTr('Feathering')
            Layout.alignment: Qt.AlignRight

        }
        SliderSpinner {
            id: featheralphaSlider
            enabled: alphaoperationCombo.currentIndex > 0
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(featheralpha, featheralphaSlider.value / featheralphaSlider.maximumValue, featheralphaKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: featheralphaSlider.value = featheralphaDefault * featheralphaSlider.maximumValue
        }
        KeyframesButton {
            id: featheralphaKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(featheralpha) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, featheralpha, featheralphaSlider.value / featheralphaSlider.maximumValue)
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        onInChanged: updateSimpleKeyframes()
        onOutChanged: updateSimpleKeyframes()
        onAnimateInChanged: updateSimpleKeyframes()
        onAnimateOutChanged: updateSimpleKeyframes()
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
