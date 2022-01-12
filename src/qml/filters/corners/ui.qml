/*
 * Copyright (c) 2019-2021 Meltytech, LLC
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

Shotcut.KeyframableFilter {
    property string corner1xProperty: '0'
    property string corner1yProperty: '1'
    property string corner2xProperty: '2'
    property string corner2yProperty: '3'
    property string corner3xProperty: '4'
    property string corner3yProperty: '5'
    property string corner4xProperty: '6'
    property string corner4yProperty: '7'
    property string enablestretchProperty: '8'
    property string stretchxProperty: '9'
    property string stretchyProperty: '10'
    property string interpolatorProperty: '11'
    property string transparentProperty: '12'
    property string featherProperty: '13'
    property string alphaOpProperty: '14'

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

    property var cornerProperties: ['shotcut:corner1', 'shotcut:corner2', 'shotcut:corner3', 'shotcut:corner4']
    property var corners: [
        Qt.rect(corner1xDefault, corner1yDefault, 0, 0),
        Qt.rect(corner2xDefault, corner2yDefault, 0, 0),
        Qt.rect(corner3xDefault, corner3yDefault, 0, 0),
        Qt.rect(corner4xDefault, corner4yDefault, 0, 0)]
    property var cornerStartValues: ['_shotcut:corner1StartValue', '_shotcut:corner2StartValue', '_shotcut:corner3StartValue', '_shotcut:corner4StartValue']
    property var cornerMiddleValues: ['_shotcut:corner1MiddleValue', '_shotcut:corner2MiddleValue', '_shotcut:corner3MiddleValue', '_shotcut:corner4MiddleValue']
    property var cornerEndValues: ['_shotcut:corner1EndValue', '_shotcut:corner2EndValue', '_shotcut:corner3EndValue', '_shotcut:corner4EndValue']

    keyframableParameters: [corner1xProperty, corner1yProperty, corner2xProperty, corner2yProperty, corner3xProperty, corner3yProperty, corner4xProperty, corner4yProperty, stretchxProperty, stretchyProperty, featherProperty]
    startValues: [corner1xDefault, corner1yDefault, corner2xDefault, corner2yDefault, corner3xDefault, corner3yDefault, corner4xDefault, corner4yDefault, stretchxDefault, stretchyDefault, featheralphaDefault]
    middleValues: [corner1xDefault, corner1yDefault, corner2xDefault, corner2yDefault, corner3xDefault, corner3yDefault, corner4xDefault, corner4yDefault, stretchxDefault, stretchyDefault, featheralphaDefault]
    endValues: [corner1xDefault, corner1yDefault, corner2xDefault, corner2yDefault, corner3xDefault, corner3yDefault, corner4xDefault, corner4yDefault, stretchxDefault, stretchyDefault, featheralphaDefault]

    width: 350
    height: 450

    Component.onCompleted: {
        filter.blockSignals = true
        var cornersString = JSON.stringify(corners)
        for (var i in cornerStartValues)
            filter.set(cornerStartValues[i], corners[i])
        for (i in cornerMiddleValues)
            filter.set(cornerMiddleValues[i], corners[i])
        for (i in cornerEndValues)
            filter.set(cornerEndValues[i], corners[i])
        if (filter.isNew) {
            filter.set(corner1xProperty, corner1xDefault)
            filter.set(corner1yProperty, corner1yDefault)
            filter.set(corner2xProperty, corner2xDefault)
            filter.set(corner2yProperty, corner2yDefault)
            filter.set(corner3xProperty, corner3xDefault)
            filter.set(corner3yProperty, corner3yDefault)
            filter.set(corner4xProperty, corner4xDefault)
            filter.set(corner4yProperty, corner4yDefault)
            filter.set(enablestretchProperty, 1)
            filter.set(stretchxProperty, stretchxDefault)
            filter.set(stretchyProperty, stretchyDefault)
            filter.set(interpolatorProperty, interpolatorDefault)
            filter.set(transparentProperty, 1)
            filter.set(alphaOpProperty, alphaoperationDefault)
            filter.set(featherProperty, featheralphaDefault)
            for (i in corners)
                filter.set(cornerProperties[i], '' + corners[i].x + ' ' + corners[i].y)
            filter.savePreset(preset.parameters)
        } else {
            initializeSimpleKeyframes()

            var position = getPosition()
            cornersString = filter.get(cornerProperties[0])
            if (cornersString) {
                for (i in corners)
                    corners[i] = filter.getRect(cornerProperties[i], position)
            } else {
                corners[0].x = filter.getDouble(corner1xProperty, position)
                corners[0].y = filter.getDouble(corner1yProperty, position)
                corners[1].x = filter.getDouble(corner2xProperty, position)
                corners[1].y = filter.getDouble(corner2yProperty, position)
                corners[2].x = filter.getDouble(corner3xProperty, position)
                corners[2].y = filter.getDouble(corner3yProperty, position)
                corners[3].x = filter.getDouble(corner4xProperty, position)
                corners[3].y = filter.getDouble(corner4yProperty, position)
                for (i in cornerProperties)
                    filter.set(cornerProperties[i], corners[i])
            }
            for (i in cornerMiddleValues)
                filter.set(cornerMiddleValues[i], filter.getRect(cornerProperties[i], filter.animateIn + 1))
            if (filter.animateIn > 0) {
                for (i in cornerStartValues)
                    filter.set(cornerStartValues[i], filter.getRect(cornerProperties[i], 0))
            }
            if (filter.animateOut > 0) {
                for (i in cornerEndValues)
                    filter.set(cornerEndValues[i], filter.getRect(cornerProperties[i], filter.duration - 1))
            }
        }
        filter.blockSignals = false
        setControls()
        if (filter.isNew) {
            filter.set(cornerProperties[0], filter.getRect(cornerProperties[0]))
        }
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
        stretchxSlider.value = (1.0 - filter.getDouble(stretchxProperty, position)) * stretchxSlider.maximumValue
        stretchySlider.value = (1.0 - filter.getDouble(stretchyProperty, position)) * stretchySlider.maximumValue
        interpolatorCombo.currentIndex = Math.round(filter.getDouble(interpolatorProperty) * 6)
        alphaoperationCombo.currentIndex = filter.get(transparentProperty) === '1'?
                    Math.round(filter.getDouble(alphaOpProperty) * 4) + 1 : 0
        featheralphaSlider.value = filter.getDouble(featherProperty, position) * featheralphaSlider.maximumValue

        for (var i in corners)
            corners[i] = filter.getRect(cornerProperties[i], position)
        setSliderValue(corner1xSlider, corners[0].x)
        setSliderValue(corner1ySlider, corners[0].y)
        setSliderValue(corner2xSlider, corners[1].x)
        setSliderValue(corner2ySlider, corners[1].y)
        setSliderValue(corner3xSlider, corners[2].x)
        setSliderValue(corner3ySlider, corners[2].y)
        setSliderValue(corner4xSlider, corners[3].x)
        setSliderValue(corner4ySlider, corners[3].y)

        corner1KeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(corner1xProperty) > 0
        stretchxKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(stretchxProperty) > 0
        stretchyKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(stretchyProperty) > 0
        featheralphaKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(featherProperty) > 0

        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        corner1xSlider.enabled = corner1ySlider.enabled = corner2xSlider.enabled = corner2ySlider.enabled = corner3xSlider.enabled = corner3ySlider.enabled = corner4xSlider.enabled = corner4ySlider.enabled = stretchxSlider.enabled = stretchySlider.enabled = featheralphaSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        setControls()
        updateFilter(corner1xProperty, sliderValue(corner1xSlider), corner1KeyframesButton, null)
        updateFilter(corner1yProperty, sliderValue(corner1ySlider), corner1KeyframesButton, null)
        updateFilter(corner2xProperty, sliderValue(corner2xSlider), corner1KeyframesButton, null)
        updateFilter(corner2yProperty, sliderValue(corner2ySlider), corner1KeyframesButton, null)
        updateFilter(corner3xProperty, sliderValue(corner3xSlider), corner1KeyframesButton, null)
        updateFilter(corner3yProperty, sliderValue(corner3ySlider), corner1KeyframesButton, null)
        updateFilter(corner4xProperty, sliderValue(corner4xSlider), corner1KeyframesButton, null)
        updateFilter(corner4yProperty, sliderValue(corner4ySlider), corner1KeyframesButton, null)
        updateFilter(stretchxProperty, 1.0 - stretchxSlider.value / stretchxSlider.maximumValue, stretchxKeyframesButton, null)
        updateFilter(stretchyProperty, 1.0 - stretchySlider.value / stretchySlider.maximumValue, stretchyKeyframesButton, null)
        updateFilter(featherProperty, featheralphaSlider.value / featheralphaSlider.maximumValue, featheralphaKeyframesButton, null)
        updateFilterCorners(null)
    }

    function resetFilter() {
        filter.resetProperty(corner1xProperty)
        filter.resetProperty(corner1yProperty)
        filter.resetProperty(corner2xProperty)
        filter.resetProperty(corner2yProperty)
        filter.resetProperty(corner3xProperty)
        filter.resetProperty(corner3yProperty)
        filter.resetProperty(corner4xProperty)
        filter.resetProperty(corner4yProperty)
        for (var i in cornerProperties)
            filter.resetProperty(cornerProperties[i])
    }

    function setFilterCorners(corners, position) {
        for (var i in cornerProperties)
            filter.set(cornerProperties[i], corners[i], 1.0, position)
        filter.set(corner1xProperty, corners[0].x, position)
        filter.set(corner1yProperty, corners[0].y, position)
        filter.set(corner2xProperty, corners[1].x, position)
        filter.set(corner2yProperty, corners[1].y, position)
        filter.set(corner3xProperty, corners[2].x, position)
        filter.set(corner3yProperty, corners[2].y, position)
        filter.set(corner4xProperty, corners[3].x, position)
        filter.set(corner4yProperty, corners[3].y, position)
    }

    function updateFilterCorners(position) {
        if (blockUpdate) return
        if (position !== null) {
            filter.blockSignals = true
            if (position <= 0 && filter.animateIn > 0) {
                for (var i in cornerStartValues)
                    filter.set(cornerStartValues[i], corners[i])
            } else if (position >= filter.duration - 1 && filter.animateOut > 0) {
                for (i in cornerEndValues)
                    filter.set(cornerEndValues[i], corners[i])
            } else {
                for (i in cornerMiddleValues)
                    filter.set(cornerMiddleValues[i], corners[i])
            }
            filter.blockSignals = false
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            resetFilter()
            corner1KeyframesButton.checked = false
            if (filter.animateIn > 0) {
                setFilterCorners([filter.getRect(cornerStartValues[0]),filter.getRect(cornerStartValues[1]),filter.getRect(cornerStartValues[2]),filter.getRect(cornerStartValues[3])], 0)
                setFilterCorners([filter.getRect(cornerMiddleValues[0]),filter.getRect(cornerMiddleValues[1]),filter.getRect(cornerMiddleValues[2]),filter.getRect(cornerMiddleValues[3])], filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                setFilterCorners([filter.getRect(cornerMiddleValues[0]),filter.getRect(cornerMiddleValues[1]),filter.getRect(cornerMiddleValues[2]),filter.getRect(cornerMiddleValues[3])], filter.duration - filter.animateOut)
                setFilterCorners([filter.getRect(cornerEndValues[0]),filter.getRect(cornerEndValues[1]),filter.getRect(cornerEndValues[2]),filter.getRect(cornerEndValues[3])], filter.duration - 1)
            }
        } else if (filter.keyframeCount(corner1xProperty) <= 0) {
            resetFilter()
            setFilterCorners([filter.getRect(cornerMiddleValues[0]),filter.getRect(cornerMiddleValues[1]),filter.getRect(cornerMiddleValues[2]),filter.getRect(cornerMiddleValues[3])], -1)
        } else if (position !== null) {
            setFilterCorners(corners, position)
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 4

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            id: preset
            parameters: [corner1xProperty, corner1yProperty, corner2xProperty, corner2yProperty, corner3xProperty, corner3yProperty, corner4xProperty, corner4yProperty, stretchxProperty, stretchyProperty, interpolatorProperty, transparentProperty, featherProperty, alphaOpProperty, cornerProperties[0], cornerProperties[1], cornerProperties[2], cornerProperties[3]]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeyframes()
            }
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
                corner1KeyframesButton.checked = filter.keyframeCount(corner1xProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
                filter.blockSignals = true
                for (var i in cornerMiddleValues)
                    filter.set(cornerMiddleValues[i], filter.getRect(cornerProperties[i], filter.animateIn + 1))
                if (filter.animateIn > 0) {
                    for (i in cornerStartValues)
                        filter.set(cornerStartValues[i], filter.getRect(cornerProperties[i], 0))
                }
                if (filter.animateOut > 0) {
                    for (i in cornerEndValues)
                        filter.set(cornerEndValues[i], filter.getRect(cornerProperties[i], filter.duration - 1))
                }
                filter.blockSignals = false
            }
        }

        Label {
            text: qsTr('Corner 1 X')
            Layout.alignment: Qt.AlignRight

        }
        Shotcut.SliderSpinner {
            id: corner1xSlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: {
                var newValue = sliderValue(corner1xSlider)
                if (corners[0].x !== newValue) {
                    corners[0].x = newValue
                    updateFilterCorners(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: setSliderValue(corner1xSlider, corner1xDefault)
        }
        ColumnLayout {
            Layout.rowSpan: 8
            height: corner1KeyframesButton.height * Layout.rowSpan
            SystemPalette { id: activePalette }
            Rectangle {
                color: activePalette.text
                width: 1
                height: parent.height / 2
                anchors.horizontalCenter: corner1KeyframesButton.horizontalCenter
            }
            Shotcut.KeyframesButton {
                id: corner1KeyframesButton
                onToggled: {
                    toggleKeyframes(checked, corner1xProperty, corners[0].x)
                    toggleKeyframes(checked, corner1yProperty, corners[0].y)
                    toggleKeyframes(checked, corner2xProperty, corners[1].x)
                    toggleKeyframes(checked, corner2yProperty, corners[1].y)
                    toggleKeyframes(checked, corner3xProperty, corners[2].x)
                    toggleKeyframes(checked, corner3yProperty, corners[2].y)
                    toggleKeyframes(checked, corner4xProperty, corners[3].x)
                    toggleKeyframes(checked, corner4yProperty, corners[3].y)
                    for (var i in corners) {
                        if (checked) {
                            blockUpdate = true
                            if (filter.animateIn > 0 || filter.animateOut > 0) {
                                // Reset all of the simple keyframes.
                                resetSimpleKeyframes()
                                filter.animateIn = 0
                                blockUpdate = false
                                filter.animateOut = 0
                            } else {
                                filter.clearSimpleAnimation(cornerProperties[i])
                                blockUpdate = false
                            }
                            // Set this keyframe value.
                            filter.set(cornerProperties[i], corners[i], 1.0, getPosition())
                        } else {
                            // Remove keyframes and set the parameter.
                            filter.resetProperty(cornerProperties[i])
                            filter.set(cornerProperties[i], corners[i])
                        }
                    }
                    setControls()
                }
            }
            Rectangle {
                color: activePalette.text
                width: 1
                height: parent.height / 2
                anchors.horizontalCenter: corner1KeyframesButton.horizontalCenter
            }
        }

        Label {
            text: qsTr('Y')
            Layout.alignment: Qt.AlignRight

        }
        Shotcut.SliderSpinner {
            id: corner1ySlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: {
                var newValue = sliderValue(corner1ySlider)
                if (corners[0].y !== newValue) {
                    corners[0].y = newValue
                    updateFilterCorners(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: setSliderValue(corner1ySlider, corner1yDefault)
        }

        Label {
            text: qsTr('Corner 2 X')
            Layout.alignment: Qt.AlignRight

        }
        Shotcut.SliderSpinner {
            id: corner2xSlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: {
                var newValue = sliderValue(corner2xSlider)
                if (corners[1].x !== newValue) {
                    corners[1].x = newValue
                    updateFilterCorners(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: setSliderValue(corner2xSlider, corner2xDefault)
        }

        Label {
            text: qsTr('Y')
            Layout.alignment: Qt.AlignRight

        }
        Shotcut.SliderSpinner {
            id: corner2ySlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: {
                var newValue = sliderValue(corner2ySlider)
                if (corners[1].y !== newValue) {
                    corners[1].y = newValue
                    updateFilterCorners(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: setSliderValue(corner2ySlider, corner2yDefault)
        }

        Label {
            text: qsTr('Corner 3 X')
            Layout.alignment: Qt.AlignRight

        }
        Shotcut.SliderSpinner {
            id: corner3xSlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: {
                var newValue = sliderValue(corner3xSlider)
                if (corners[2].x !== newValue) {
                    corners[2].x = newValue
                    updateFilterCorners(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: setSliderValue(corner3xSlider, corner3xDefault)
        }

        Label {
            text: qsTr('Y')
            Layout.alignment: Qt.AlignRight

        }
        Shotcut.SliderSpinner {
            id: corner3ySlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: {
                var newValue = sliderValue(corner3ySlider)
                if (corners[2].y !== newValue) {
                    corners[2].y = newValue
                    updateFilterCorners(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: setSliderValue(corner3ySlider, corner3yDefault)
        }

        Label {
            text: qsTr('Corner 4 X')
            Layout.alignment: Qt.AlignRight

        }
        Shotcut.SliderSpinner {
            id: corner4xSlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: {
                var newValue = sliderValue(corner4xSlider)
                if (corners[3].x !== newValue) {
                    corners[3].x = newValue
                    updateFilterCorners(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: setSliderValue(corner4xSlider, corner4xDefault)
        }

        Label {
            text: qsTr('Y')
            Layout.alignment: Qt.AlignRight

        }
        Shotcut.SliderSpinner {
            id: corner4ySlider
            minimumValue: -100
            maximumValue: 200
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: {
                var newValue = sliderValue(corner4ySlider)
                if (corners[3].y !== newValue) {
                    corners[3].y = newValue
                    updateFilterCorners(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: setSliderValue(corner4ySlider, corner4yDefault)
        }

        Label {
            text: qsTr('Stretch X')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: stretchxSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(stretchxProperty, 1.0 - stretchxSlider.value / stretchxSlider.maximumValue, stretchxKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: stretchxSlider.value = stretchxDefault * stretchxSlider.maximumValue
        }
        Shotcut.KeyframesButton {
            id: stretchxKeyframesButton
            onToggled: {
                toggleKeyframes(checked, stretchxProperty, 1.0 - stretchxSlider.value / stretchxSlider.maximumValue)
                setControls()
            }
        }

        Label {
            text: qsTr('Y')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: stretchySlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(stretchyProperty, 1.0 - stretchySlider.value / stretchySlider.maximumValue, stretchyKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: stretchySlider.value = stretchyDefault * stretchySlider.maximumValue
        }
        Shotcut.KeyframesButton {
            id: stretchyKeyframesButton
            onToggled: {
                toggleKeyframes(checked, stretchyProperty, 1.0 - stretchySlider.value / stretchySlider.maximumValue)
                setControls()
            }
        }

        Label {
            text: qsTr('Interpolator')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: interpolatorCombo
            implicitWidth: 180
            model: [qsTr('Nearest Neighbor'), qsTr('Bilinear'), qsTr('Bicubic Smooth'), qsTr('Bicubic Sharp'), qsTr('Spline 4x4'), qsTr('Spline 6x6'), 'Lanczos']
            onActivated: {
                enabled = false
                filter.set(interpolatorProperty, index / 6)
                enabled = true
            }
        }
        Shotcut.UndoButton {
            onClicked: filter.set(interpolatorProperty, interpolatorDefault)
            Layout.columnSpan: 2
        }

        Label {
            text: qsTr('Alpha Operation')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: alphaoperationCombo
            implicitWidth: 180
            model: [qsTr('Opaque'), qsTr('Overwrite'), qsTr('Maximum'), qsTr('Minimum'), qsTr('Add'), qsTr('Subtract')]
            onActivated: {
                enabled = false
                filter.set(transparentProperty, index > 0)
                filter.set(alphaOpProperty, (index - 1) / 4)
                enabled = true
            }
        }
        Shotcut.UndoButton {
            Layout.columnSpan: 2
            onClicked: {
                alphaoperationCombo.currentIndex = filter.get(transparentProperty) === '1'?
                    Math.round(alphaoperationDefault * 4) + 1 : 0
                filter.set(transparentProperty, alphaoperationCombo.currentIndex > 0)
                filter.set(alphaOpProperty, (alphaoperationCombo.currentIndex - 1) / 4)
            }
        }

        Label {
            text: qsTr('Feathering')
            Layout.alignment: Qt.AlignRight

        }
        Shotcut.SliderSpinner {
            id: featheralphaSlider
            enabled: alphaoperationCombo.currentIndex > 0
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(featherProperty, featheralphaSlider.value / featheralphaSlider.maximumValue, featheralphaKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: featheralphaSlider.value = featheralphaDefault * featheralphaSlider.maximumValue
        }
        Shotcut.KeyframesButton {
            id: featheralphaKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, featherProperty, featheralphaSlider.value / featheralphaSlider.maximumValue)
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        onChanged: setControls()
        onInChanged: updateSimpleKeyframes()
        onOutChanged: updateSimpleKeyframes()
        onAnimateInChanged: updateSimpleKeyframes()
        onAnimateOutChanged: updateSimpleKeyframes()
        onPropertyChanged: setControls()
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
