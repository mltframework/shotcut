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
    property string levels: '0'
    property double levelsDefault: 0.10
   
    keyframableParameters: [levels]
    startValues: [0.5]
    middleValues: [levelsDefault]
    endValues: [0.5]

    width: 350
    height: 100

    Component.onCompleted: {
        filter.set('threads', 0)
        if (filter.isNew) {
            filter.set(levels, levelsDefault)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        levelsSlider.value = filter.getDouble(levels, position) * levelsSlider.maximumValue
        levelKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(levels) > 0
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        levelsSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        updateFilter(levels, levelsSlider.value / levelsSlider.maximumValue, levelKeyframesButton, null)
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
            parameters: [levels]
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
            text: qsTr('Levels', 'Posterize filter')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: levelsSlider
            minimumValue: 0
            maximumValue: 100.0
            stepSize: 1
            decimals: 0
            suffix: ' %'
            onValueChanged: updateFilter(levels, levelsSlider.value / levelsSlider.maximumValue, levelKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: levelsSlider.value = levelsDefault * levelsSlider.maximumValue
        }
        Shotcut.KeyframesButton {
            id: levelKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, levels, levelsSlider.value / levelsSlider.maximumValue)
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
        onPropertyChanged: setControls()
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
