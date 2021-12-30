/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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

Item {
    width: 350
    height: 50
    property string saturationParameter: '0'
    property real frei0rMaximum: 800
    property bool blockUpdate: true
    property double startValue: 0.125
    property double middleValue: 0.125
    property double endValue: 0.125

    Component.onCompleted: {
        filter.set('threads', 0)
        if (filter.isNew) {
            // Set default parameter values
            filter.set(saturationParameter, 0)
            filter.savePreset(preset.parameters, qsTr('Grayscale'))
            filter.set(saturationParameter, 100 / frei0rMaximum)
            filter.savePreset(preset.parameters)
        } else {
            middleValue = filter.getDouble(saturationParameter, filter.animateIn)
            if (filter.animateIn > 0)
                startValue = filter.getDouble(saturationParameter, 0)
            if (filter.animateOut > 0)
                endValue = filter.getDouble(saturationParameter, filter.duration - 1)
        }
        setControls()
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        slider.value = filter.getDouble(saturationParameter, position) * frei0rMaximum
        keyframesButton.checked = filter.keyframeCount(saturationParameter) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
        blockUpdate = false
        slider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
    }

    function updateFilter(position) {
        if (blockUpdate) return
        var value = slider.value / frei0rMaximum

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value
            else
                middleValue = value
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(saturationParameter)
            keyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set(saturationParameter, startValue, 0)
                filter.set(saturationParameter, middleValue, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set(saturationParameter, middleValue, filter.duration - filter.animateOut)
                filter.set(saturationParameter, endValue, filter.duration - 1)
            }
        } else if (!keyframesButton.checked) {
            filter.resetProperty(saturationParameter)
            filter.set(saturationParameter, middleValue)
        } else if (position !== null) {
            filter.set(saturationParameter, value, position)
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
        Shotcut.Preset {
            id: preset
            Layout.columnSpan: 3
            parameters: [saturationParameter]
            onBeforePresetLoaded: {
                filter.resetProperty(saturationParameter)
            }
            onPresetSelected: {
                setControls()
                middleValue = filter.getDouble(saturationParameter, filter.animateIn)
                if (filter.animateIn > 0)
                    startValue = filter.getDouble(saturationParameter, 0)
                if (filter.animateOut > 0)
                    endValue = filter.getDouble(saturationParameter, filter.duration - 1)
            }
        }

        Label {
            text: qsTr('Level')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider
            minimumValue: 0
            maximumValue: 300
            suffix: ' %'
            onValueChanged: updateFilter(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider.value = 100
        }
        Shotcut.KeyframesButton {
            id: keyframesButton
            onToggled: {
                var value = slider.value / frei0rMaximum
                if (checked) {
                    blockUpdate = true
                    filter.clearSimpleAnimation(saturationParameter)
                    blockUpdate = false
                    filter.set(saturationParameter, value, getPosition())
                } else {
                    filter.resetProperty(saturationParameter)
                    filter.set(saturationParameter, value)
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        onChanged: setControls()
        onInChanged: { setControls(); updateFilter(null) }
        onOutChanged: { setControls(); updateFilter(null) }
        onAnimateInChanged:  { setControls(); updateFilter(null) }
        onAnimateOutChanged:  { setControls(); updateFilter(null) }
        onPropertyChanged: setControls()
    }

    Connections {
        target: producer
        onPositionChanged: {
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                setControls()
            } else {
                blockUpdate = true
                slider.value = filter.getDouble(saturationParameter, getPosition()) * frei0rMaximum
                blockUpdate = false
                slider.enabled = true
            }
        }
    }
}
