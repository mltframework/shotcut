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
    property string glitchFreq: '0'
    property string blockH: '1'
    property string shiftInt: '2'
    property string colorInt: '3'
    property double glitchFreqDefault: 0.5
    property double blockHDefault: 0.5
    property double shiftIntDefault: 0.5
    property double colorIntDefault: 0.5

    keyframableParameters: [glitchFreq, blockH, shiftInt, colorInt]
    startValues: [0, 0.5, 0.5, 0.5]
    middleValues: [glitchFreqDefault, blockHDefault, shiftIntDefault, colorIntDefault]
    endValues: [0, 0.5, 0.5, 0.5]

    width: 350
    height: 150

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(glitchFreq, glitchFreqDefault)
            filter.set(blockH, blockHDefault)
            filter.set(shiftInt, shiftIntDefault)
            filter.set(colorInt, colorIntDefault)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        glitchFreqSlider.value = filter.getDouble(glitchFreq, position) * glitchFreqSlider.maximumValue
        glitchKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(glitchFreq) > 0
        blockHSlider.value = filter.getDouble(blockH, position) * blockHSlider.maximumValue
        blockKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(blockH) > 0
        shiftIntSlider.value = filter.getDouble(shiftInt, position) * shiftIntSlider.maximumValue
        shiftKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(shiftInt) > 0
        colorIntSlider.value = filter.getDouble(colorInt, position) * colorIntSlider.maximumValue
        colorKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(colorInt) > 0
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        glitchFreqSlider.enabled = blockHSlider.enabled = shiftIntSlider.enabled = colorIntSlider.enabled = enabled
        
    }

    function updateSimpleKeyframes() {
        setControls()
        updateFilter(glitchFreq, glitchFreqSlider.value / glitchFreqSlider.maximumValue, glitchKeyframesButton, null)
        updateFilter(blockH, blockHSlider.value / blockHSlider.maximumValue, blockKeyframesButton, null)
        updateFilter(shiftInt, shiftIntSlider.value / shiftIntSlider.maximumValue, shiftKeyframesButton, null)
        updateFilter(colorInt, colorIntSlider.value / colorIntSlider.maximumValue, colorKeyframesButton, null)
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
            parameters: [glitchFreq, blockH, shiftInt, colorInt]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeyframes
            }
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
            }
        }

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: glitchFreqSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(glitchFreq, glitchFreqSlider.value / glitchFreqSlider.maximumValue, glitchKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: glitchFreqSlider.value = glitchFreqDefault * glitchFreqSlider.maximumValue
        }
        Shotcut.KeyframesButton {
            id: glitchKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, glitchFreq, glitchFreqSlider.value / glitchFreqSlider.maximumValue)
            }
        }

        Label {
            text: qsTr('Block height')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: blockHSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(blockH, blockHSlider.value / blockHSlider.maximumValue, blockKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: blockHSlider.value = blockHDefault * blockHSlider.maximumValue
        }
        Shotcut.KeyframesButton {
            id: blockKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, blockH, blockHSlider.value / blockHSlider.maximumValue)
            }
        }

        Label {
            text: qsTr('Shift intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: shiftIntSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(shiftInt, shiftIntSlider.value / shiftIntSlider.maximumValue, shiftKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: shiftIntSlider.value = shiftIntDefault * shiftIntSlider.maximumValue
        }
        Shotcut.KeyframesButton {
            id: shiftKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, shiftInt, shiftIntSlider.value / shiftIntSlider.maximumValue)
            }
        }

        Label {
            text: qsTr('Color intensity')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: colorIntSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(colorInt, colorIntSlider.value / colorIntSlider.maximumValue, colorKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: colorIntSlider.value = colorIntDefault * colorIntSlider.maximumValue
        }
        Shotcut.KeyframesButton {
            id: colorKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, colorInt, colorIntSlider.value / colorIntSlider.maximumValue)
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        function onChanged() { setControls() }
        function onInChanged() { updateSimpleKeyframes() }
        function onOutChanged() { updateSimpleKeyframes() }
        function onAnimateInChanged() { updateSimpleKeyframes() }
        function onAnimateOutChanged() { updateSimpleKeyframes() }
        function onPropertyChanged(name) { setControls() }
    }

    Connections {
        target: producer
        function onPositionChanged() { setControls() }
    }
}
