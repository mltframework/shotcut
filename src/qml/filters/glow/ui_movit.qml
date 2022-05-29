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
    property var defaultParameters: ['radius','blur_mix','highlight_cutoff']
    property bool blockUpdate: true
    property var startValues:  [0.0,  0.0, 0.1]
    property var middleValues: [20.0, 1.0, 0.2]
    property var endValues:    [0.0,  0.0, 0.1]
    width: 350
    height: 125
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('radius', 20.0)
            filter.set('blur_mix', 1.0)
            filter.set('highlight_cutoff', 0.2)
            filter.savePreset(defaultParameters)
        } else {
            initSimpleAnimation()
        }
        setControls()
    }

    function initSimpleAnimation() {
        middleValues = [filter.getDouble(defaultParameters[0], filter.animateIn),
                        filter.getDouble(defaultParameters[1], filter.animateIn),
                        filter.getDouble(defaultParameters[2], filter.animateIn)]
        if (filter.animateIn > 0) {
            startValues = [filter.getDouble(defaultParameters[0], 0),
                           filter.getDouble(defaultParameters[1], 0),
                           filter.getDouble(defaultParameters[2], 0)]
        }
        if (filter.animateOut > 0) {
            endValues = [filter.getDouble(defaultParameters[0], filter.duration - 1),
                         filter.getDouble(defaultParameters[1], filter.duration - 1),
                         filter.getDouble(defaultParameters[2], filter.duration - 1)]
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        radiusslider.value = filter.getDouble('radius', position)
        blurslider.value = filter.getDouble('blur_mix', position)
        cutoffslider.value = filter.getDouble('highlight_cutoff', position)
        blockUpdate = false
        radiusslider.enabled = blurslider.enabled = cutoffslider.enabled
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
            radiusslider.enabled = blurslider.enabled = cutoffslider.enabled = true
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                filter.resetProperty('radius')
                filter.resetProperty('blur_mix')
                filter.resetProperty('highlight_cutoff')
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
            Layout.columnSpan: 3
            parameters: defaultParameters
            onBeforePresetLoaded: {
                filter.resetProperty('radius')
                filter.resetProperty('blur_mix')
                filter.resetProperty('highlight_cutoff')
            }
            onPresetSelected: {
                setControls()
                initSimpleAnimation()
            }
        }

        // Row 1
        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: radiusslider
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            onValueChanged: updateFilter('radius', value, getPosition(), radiusKeyframesButton)
        }
        Shotcut.UndoButton {
            onClicked: radiusslider.value = 20
        }
        Shotcut.KeyframesButton {
            id: radiusKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('radius') > 0
            onToggled: onKeyframesButtonClicked(checked, 'radius', radiusslider.value)
        }

        // Row 2
        Label { 
            text: qsTr('Highlight blurriness')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: blurslider
            minimumValue: 0.0
            maximumValue: 1.0
            decimals: 2
            onValueChanged: updateFilter('blur_mix', value, getPosition(), blurKeyframesButton)
        }
        Shotcut.UndoButton {
            onClicked: blurslider.value = 1.0
        }
        Shotcut.KeyframesButton {
            id: blurKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('blur_mix') > 0
            onToggled: onKeyframesButtonClicked(checked, 'blur_mix', blurslider.value)
        }

        // Row 3
        Label {
            text: qsTr('Highlight cutoff')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: cutoffslider
            minimumValue: 0.1
            maximumValue: 1.0
            decimals: 2
            onValueChanged: updateFilter('highlight_cutoff', value, getPosition(), cutoffKeyframesButton)
        }
        Shotcut.UndoButton {
            onClicked: cutoffslider.value = 0.2
        }
        Shotcut.KeyframesButton {
            id: cutoffKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('highlight_cutoff') > 0
            onToggled: onKeyframesButtonClicked(checked, 'highlight_cutoff', cutoffslider.value)
        }

        Item {
            Layout.fillHeight: true
        }
    }

    function updateSimpleAnimation() {
        updateFilter('radius', radiusslider.value, getPosition(), radiusKeyframesButton)
        updateFilter('blur_mix', blurslider.value, getPosition(), blurKeyframesButton)
        updateFilter('highlight_cutoff', cutoffslider.value, getPosition(), cutoffKeyframesButton)
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
