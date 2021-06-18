/*
 * Copyright (c) 2016-2021 Meltytech, LLC
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
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    width: 200
    height: 50
    property bool blockUpdate: true
    property double startValue: 1.0
    property double middleValue: 1.0
    property double endValue: 1.0

    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('level', 1.0)
            filter.savePreset(preset.parameters)
        } else {
            middleValue = filter.getDouble('level', filter.animateIn)
            if (filter.animateIn > 0)
                startValue = filter.getDouble('level', 0)
            if (filter.animateOut > 0)
                endValue = filter.getDouble('level', filter.duration - 1)
        }
        setControls()
    }

    Connections {
        target: filter
        onInChanged: updateFilter(null)
        onOutChanged: updateFilter(null)
        onAnimateInChanged: updateFilter(null)
        onAnimateOutChanged: updateFilter(null)
        onPropertyChanged: setControls()
    }

    Connections {
        target: producer
        onPositionChanged: {
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                setControls()
            } else {
                blockUpdate = true
                brightnessSlider.value = filter.getDouble('level', getPosition()) * 100.0
                blockUpdate = false
                brightnessSlider.enabled = true
            }
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        brightnessSlider.value = filter.getDouble('level', position) * 100.0
        brightnessKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('level') > 0
        blockUpdate = false
        brightnessSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
    }

    function updateFilter(position) {
        if (blockUpdate) return
        var value = brightnessSlider.value / 100.0

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value
            else
                middleValue = value
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('level')
            brightnessKeyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set('level', startValue, 0)
                filter.set('level', middleValue, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set('level', middleValue, filter.duration - filter.animateOut)
                filter.set('level', endValue, filter.duration - 1)
            }
        } else if (!brightnessKeyframesButton.checked) {
            filter.resetProperty('level')
            filter.set('level', middleValue)
        } else if (position !== null) {
            filter.set('level', value, position)
        }
//        console.log('level: ' + filter.get('level'))
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
            parameters: ['level']
            onBeforePresetLoaded: {
                filter.resetProperty(parameters[0])
            }
            onPresetSelected: {
                setControls()
                middleValue = filter.getDouble(parameters[0], filter.animateIn)
                if (filter.animateIn > 0)
                    startValue = filter.getDouble(parameters[0], 0)
                if (filter.animateOut > 0)
                    endValue = filter.getDouble(parameters[0], filter.duration - 1)
            }
        }

        Label {
            text: qsTr('Level')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: brightnessSlider
            minimumValue: 0.0
            maximumValue: 200.0
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: brightnessSlider.value = 100
        }
        Shotcut.KeyframesButton {
            id: brightnessKeyframesButton
            onToggled: {
                var value = brightnessSlider.value / 100.0
                if (checked) {
                    blockUpdate = true
                    filter.clearSimpleAnimation('level')
                    blockUpdate = false
                    filter.set('level', value, getPosition())
                } else {
                    filter.resetProperty('level')
                    filter.set('level', value)
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
