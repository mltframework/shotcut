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

Shotcut.KeyframableFilter {
    property double hueDegreeDefault: 0
    property double lightnessDefault: 0
    property double saturationDefault: 1


    keyframableParameters: ['av.h', 'av.b', 'av.s']
    startValues: [hueDegreeDefault, lightnessDefault, saturationDefault]
    middleValues: [hueDegreeDefault, lightnessDefault, saturationDefault]
    endValues: [hueDegreeDefault, lightnessDefault, saturationDefault]

    width: 200
    height: 125
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('av.h', hueDegreeDefault)
            filter.set('av.b', lightnessDefault)
            filter.set('av.s', saturationDefault)
            filter.savePreset(keyframableParameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        hueDegreeSlider.value = filter.getDouble('av.h', position)
        hueKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('av.h') > 0
        lightnessSlider.value = filter.getDouble('av.b', position) * 100 / 10 + 100
        lightnessKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('av.b') > 0
        saturationSlider.value = filter.getDouble('av.s', position) * 100
        saturationKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('av.s') > 0
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        hueDegreeSlider.enabled = enabled
        lightnessSlider.enabled = enabled
        saturationSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        setControls()
        updateFilter('av.h', hueDegreeSlider.value, hueKeyframesButton, null)
        updateFilter('av.b', (lightnessSlider.value - 100) * 10 / 100, lightnessKeyframesButton, null)
        updateFilter('av.s', saturationSlider.value / 100, saturationKeyframesButton, null)
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
            id: presetItem
            Layout.columnSpan: 3
            parameters: keyframableParameters
            onBeforePresetLoaded: resetSimpleKeyframes()
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
            }
        }

        Label {
            text: qsTr('Hue')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hueDegreeSlider
            minimumValue: -360
            maximumValue: 360
            suffix: qsTr(' °', 'degrees')
            onValueChanged: updateFilter('av.h', value, hueKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: hueDegreeSlider.value = 0
        }
        Shotcut.KeyframesButton {
            id: hueKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, 'av.h', hueDegreeSlider.value)
            }
        }

        Label {
            text: qsTr('Lightness')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lightnessSlider
            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: updateFilter('av.b', (value - 100) * 10 / 100, lightnessKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: lightnessSlider.value = 100
        }
        Shotcut.KeyframesButton {
            id: lightnessKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, 'av.b', (lightnessSlider.value - 100) * 10 / 100)
            }
        }

        Label {
            text: qsTr('Saturation')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: saturationSlider
            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: updateFilter('av.s', value / 100.0, saturationKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: saturationSlider.value = 100
        }
        Shotcut.KeyframesButton {
            id: saturationKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, 'av.s', saturationSlider.value / 100)
            }
        }

        Item { Layout.fillHeight: true }
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
