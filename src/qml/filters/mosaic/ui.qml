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
    property string xsize: '0'
    property string ysize: '1'
    property real maxFilterPercent: 50.0
    property real maxUserPercent: 20.0
    property real defaultValue: 2.5 / maxFilterPercent

    keyframableParameters: [xsize, ysize]
    startValues: [0, 0]
    middleValues: [defaultValue, defaultValue]
    endValues: [0, 0]

    width: 200
    height: 50

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(xsize, defaultValue)
            filter.set(ysize, defaultValue)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        xsizeSlider.value = filter.getDouble(xsize, position) * maxFilterPercent
        xsizeKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(xsize) > 0
        ysizeSlider.value = filter.getDouble(ysize, position) * maxFilterPercent
        ysizeKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(ysize) > 0
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        xsizeSlider.enabled = ysizeSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        setControls()
        updateFilter(xsize, xsizeSlider.value / maxFilterPercent, xsizeKeyframesButton, null)
        updateFilter(ysize, ysizeSlider.value / maxFilterPercent, ysizeKeyframesButton, null)
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
            parameters: [xsize, ysize]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeyframes()
            }
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
            }
        }

        Label {
            text: qsTr('Width')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: xsizeSlider
            minimumValue: 0
            maximumValue: 20
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(xsize, xsizeSlider.value / maxFilterPercent, xsizeKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: xsizeSlider.value = defaultValue * maxFilterPercent
        }
        Shotcut.KeyframesButton {
            id: xsizeKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, xsize, xsizeSlider.value / maxFilterPercent)
            }
        }

        Label {
            text: qsTr('Height')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: ysizeSlider
            minimumValue: 0
            maximumValue: 20
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(ysize, ysizeSlider.value / maxFilterPercent, ysizeKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: ysizeSlider.value = defaultValue * maxFilterPercent
        }
        Shotcut.KeyframesButton {
            id: ysizeKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, ysize, ysizeSlider.value / maxFilterPercent)
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
