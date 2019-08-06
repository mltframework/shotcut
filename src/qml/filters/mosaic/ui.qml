/*
 * Copyright (c) 2019 Meltytech, LLC
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

KeyframableFilter {
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
        ysizeSlider.value = filter.getDouble(ysize, position) * maxFilterPercent
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        xsizeSlider.enabled = ysizeSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        updateFilter(xsize, xsizeSlider.value / maxFilterPercent, xsizeKeyframesButton)
        updateFilter(ysize, ysizeSlider.value / maxFilterPercent, ysizeKeyframesButton)
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
            parameters: [xsize, ysize]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty(xsize)
                filter.resetProperty(ysize)
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
        SliderSpinner {
            id: xsizeSlider
            minimumValue: 0
            maximumValue: 20
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(xsize, xsizeSlider.value / maxFilterPercent, xsizeKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: xsizeSlider.value = defaultValue * maxFilterPercent
        }
        KeyframesButton {
            id: xsizeKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(xsize) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, xsize, xsizeSlider.value / maxFilterPercent)
            }
        }

        Label {
            text: qsTr('Height')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: ysizeSlider
            minimumValue: 0
            maximumValue: 20
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(ysize, ysizeSlider.value / maxFilterPercent, ysizeKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: ysizeSlider.value = defaultValue * maxFilterPercent
        }
        KeyframesButton {
            id: ysizeKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(ysize) > 0
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
