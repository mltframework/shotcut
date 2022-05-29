/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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
    property bool blockUpdate: true
    property double startValue: 1.0
    property double middleValue: 1.0
    property double endValue: 1.0

    Component.onCompleted: {
        filter.set('start', 1)
        if (filter.isNew) {
            // Set default parameter values
            filter.set('level', 1)
            filter.set('alpha', 1.0)
            filter.set('opacity', filter.getDouble('alpha'))
            filter.savePreset(preset.parameters)
        } else {
            middleValue = filter.getDouble('opacity', filter.animateIn)
            if (filter.animateIn > 0)
                startValue = filter.getDouble('opacity', 0)
            if (filter.animateOut > 0)
                endValue = filter.getDouble('opacity', filter.duration - 1)
        }
        setControls()
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        slider.value = filter.getDouble('opacity', position) * 100.0
        keyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('opacity') > 0
        blockUpdate = false
        slider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
    }

    function updateFilter(position) {
        if (blockUpdate) return
        var value = slider.value / 100.0

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value
            else
                middleValue = value
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('alpha')
            filter.resetProperty('opacity')
            keyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set('alpha', startValue, 0)
                filter.set('alpha', middleValue, filter.animateIn - 1)
                filter.set('opacity', startValue, 0)
                filter.set('opacity', middleValue, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set('alpha', middleValue, filter.duration - filter.animateOut)
                filter.set('alpha', endValue, filter.duration - 1)
                filter.set('opacity', middleValue, filter.duration - filter.animateOut)
                filter.set('opacity', endValue, filter.duration - 1)
            }
        } else if (!keyframesButton.checked) {
            filter.resetProperty('alpha')
            filter.resetProperty('opacity')
            filter.set('alpha', middleValue)
            filter.set('opacity', middleValue)
        } else if (position !== null) {
            filter.set('alpha', value, position)
            filter.set('opacity', value, position)
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
            Layout.columnSpan: parent.columns
            parameters: ['opacity', 'alpha']
            onBeforePresetLoaded: {
                filter.resetProperty(parameters[0])
                filter.resetProperty(parameters[1])
            }
            onPresetSelected: {
                setControls()
                keyframesButton.checked = filter.keyframeCount(parameters[0]) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
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
            id: slider
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider.value = 100
        }
        Shotcut.KeyframesButton {
            id: keyframesButton
            onToggled: {
                var value = slider.value / 100.0
                if (checked) {
                    blockUpdate = true
                    filter.clearSimpleAnimation('alpha')
                    filter.clearSimpleAnimation('opacity')
                    blockUpdate = false
                    filter.set('alpha', value, getPosition())
                    filter.set('opacity', value, getPosition())
                } else {
                    filter.resetProperty('alpha')
                    filter.resetProperty('opacity')
                    filter.set('alpha', value)
                    filter.set('opacity', value)
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        function onChanged() { setControls() }
        function onInChanged() { setControls(); updateFilter(null) }
        function onOutChanged() { setControls(); updateFilter(null) }
        function onAnimateInChanged() { setControls(); updateFilter(null) }
        function onAnimateOutChanged() { setControls(); updateFilter(null) }
        function onPropertyChanged(name) { setControls() }
    }

    Connections {
        target: producer
        function onPositionChanged() {
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                setControls()
            } else {
                blockUpdate = true
                slider.value = filter.getDouble('opacity', getPosition()) * 100.0
                blockUpdate = false
                slider.enabled = true
            }
        }
    }
}
