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

import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0

KeyframableFilter {
    property string levels: '0'
    property string matrixid: '1'
    property double levelsDefault: 0.1
    property double matrixidDefault: 0.01

    keyframableParameters: [levels, matrixid]
    startValues: [0.5, 0.5]
    middleValues: [levelsDefault, matrixidDefault]
    endValues: [0.5, 0.5]

    width: 350
    height: 100

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(levels, levelsDefault)
            filter.set(matrixid, matrixidDefault)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        levelsSlider.value = filter.getDouble(levels, position) * levelsSlider.maximumValue
        matrixidSlider.value = filter.getDouble(matrixid, position) * matrixidSlider.maximumValue
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        levelsSlider.enabled = matrixidSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        updateFilter(levels, levelsSlider.value / levelsSlider.maximumValue, levelKeyframesButton)
        updateFilter(matrixid, matrixidSlider.value / matrixidSlider.maximumValue, matrixKeyframesButton)
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
            parameters: [levels, matrixid]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty(levels)
                filter.resetProperty(matrixid)
            }
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
            }
        }

        Label {
            text: qsTr('Levels')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: levelsSlider
            minimumValue: 0.0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(levels, levelsSlider.value / levelsSlider.maximumValue, levelKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: levelsSlider.value = levelsDefault * levelsSlider.maximumValue
        }
        KeyframesButton {
            id: levelKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(levels) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, levels, levelsSlider.value / levelsSlider.maximumValue)
            }
        }

        Label {
            text: qsTr('Matrix')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: matrixidSlider
            minimumValue: 0.00
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(matrixid, matrixidSlider.value / matrixidSlider.maximumValue, matrixKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: matrixidSlider.value = matrixidDefault * matrixidSlider.maximumValue
        }
        KeyframesButton {
            id: matrixKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(matrixid) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, matrixid, matrixidSlider.value / matrixidSlider.maximumValue)
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
