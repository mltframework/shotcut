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
    property var defaultParameters: ['radius', 'inner_radius']
    property bool blockUpdate: true
    property var startValues:  [0.0, 0.0]
    property var middleValues: [0.5, 0.5]
    property var endValues:    [0.0, 0.0]

    width: 350
    height: 100

    Component.onCompleted: {
        if (filter.isNew) {
            for (var i = 0; i < defaultParameters.length; i++)
                filter.set(defaultParameters[i], middleValues[i])
            filter.savePreset(defaultParameters)
        } else {
            initSimpleAnimation()
        }
        setControls()
    }

    function initSimpleAnimation() {
        middleValues = [filter.getDouble(defaultParameters[0], filter.animateIn),
                        filter.getDouble(defaultParameters[1], filter.animateIn)]
        if (filter.animateIn > 0) {
            startValues = [filter.getDouble(defaultParameters[0], 0),
                           filter.getDouble(defaultParameters[1], 0)]
        }
        if (filter.animateOut > 0) {
            endValues = [filter.getDouble(defaultParameters[0], filter.duration - 1),
                         filter.getDouble(defaultParameters[1], filter.duration - 1)]
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        radiusSlider.value = filter.getDouble('radius', position) * 100.0
        radiusKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('radius') > 0
        innerSlider.value = filter.getDouble('inner_radius', position) * 100.0
        innerKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('inner_radius') > 0
        blockUpdate = false
        radiusSlider.enabled = innerSlider.enabled
            = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
    }

    function updateFilter(parameter, value, position, button) {
        if (blockUpdate) return
        var index = defaultParameters.indexOf(parameter)

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValues[index] = value
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValues[index] = value
            else
                middleValues[index] = value
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(parameter)
            button.checked = false
            if (filter.animateIn > 0) {
                filter.set(parameter, startValues[index], 0)
                filter.set(parameter, middleValues[index], filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set(parameter, middleValues[index], filter.duration - filter.animateOut)
                filter.set(parameter, endValues[index], filter.duration - 1)
            }
        } else if (!button.checked) {
            filter.resetProperty(parameter)
            filter.set(parameter, middleValues[index])
        } else if (position !== null) {
            filter.set(parameter, value, position)
        }
    }

    function onKeyframesButtonClicked(checked, parameter, value) {
        if (checked) {
            blockUpdate = true
            radiusSlider.enabled = innerSlider.enabled = true
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                for (var i = 0; i < defaultParameters.length; i++)
                    filter.resetProperty(defaultParameters[i])
                filter.animateIn = filter.animateOut = 0
            } else {
                filter.clearSimpleAnimation(parameter)
            }
            blockUpdate = false
            filter.set(parameter, value, getPosition())
        } else {
            filter.resetProperty(parameter)
            filter.set(parameter, value)
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
            parameters: defaultParameters
            onBeforePresetLoaded: {
                for (var i = 0; i < defaultParameters.length; i++)
                    filter.resetProperty(defaultParameters[i])
            }
            onPresetSelected: {
                setControls()
                initSimpleAnimation()
            }
        }

        Label {
            text: qsTr('Outer radius')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: radiusSlider
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('radius', value / 100.0, getPosition(), radiusKeyframesButton)
        }
        Shotcut.UndoButton {
            onClicked: radiusSlider.value = 50
        }
        Shotcut.KeyframesButton {
            id: radiusKeyframesButton
            onToggled: onKeyframesButtonClicked(checked, 'radius', radiusSlider.value / 100.0)
        }

        Label {
            text: qsTr('Inner radius')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: innerSlider
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('inner_radius', value / 100.0, getPosition(), innerKeyframesButton)
        }
        Shotcut.UndoButton {
            onClicked: innerSlider.value = 50
        }
        Shotcut.KeyframesButton {
            id: innerKeyframesButton
            onToggled: onKeyframesButtonClicked(checked, 'inner_radius', innerSlider.value / 100.0)
        }

        Item {
            Layout.fillHeight: true
        }
    }

    function updateSimpleAnimation() {
        updateFilter('radius', radiusSlider.value / 100.0, null, radiusKeyframesButton)
        updateFilter('inner_radius', innerSlider.value / 100.0, null, innerKeyframesButton)
    }

    Connections {
        target: filter
        onInChanged: updateSimpleAnimation()
        onOutChanged: updateSimpleAnimation()
        onAnimateInChanged: updateSimpleAnimation()
        onAnimateOutChanged: updateSimpleAnimation()
        onPropertyChanged: setControls()
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
