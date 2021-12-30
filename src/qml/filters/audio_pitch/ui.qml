/*
 * Copyright (c) 2020-2021 Meltytech, LLC
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
    width: 200
    height: 50
    property bool blockUpdate: true
    property double startValue: 0.0
    property double middleValue: 0.0
    property double endValue: 0.0

    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('octaveshift', 0.0)
            filter.savePreset(preset.parameters)
        } else {
            middleValue = filter.getDouble('octaveshift', filter.animateIn)
            if (filter.animateIn > 0)
                startValue = filter.getDouble('octaveshift', 0)
            if (filter.animateOut > 0)
                endValue = filter.getDouble('octaveshift', filter.duration - 1)
        }
        setControls()
    }

    Connections {
        target: filter
        onChanged: setControls()
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
                octaveSlider.value = filter.getDouble('octaveshift', getPosition())
                blockUpdate = false
                octaveSlider.enabled = true
            }
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        octaveSlider.value = filter.getDouble('octaveshift', position)
        frequencySlider.value = 1.0 / Math.pow(2, filter.getDouble('octaveshift', position))
        octaveKeyframesButton.checked = filter.keyframeCount('octaveshift') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
        blockUpdate = false
        octaveSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
    }

    function updateFilter(position) {
        if (blockUpdate) return

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = octaveSlider.value
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = octaveSlider.value
            else
                middleValue = octaveSlider.value
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('octaveshift')
            octaveKeyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set('octaveshift', startValue, 0)
                filter.set('octaveshift', middleValue, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set('octaveshift', middleValue, filter.duration - filter.animateOut)
                filter.set('octaveshift', endValue, filter.duration - 1)
            }
        } else if (!octaveKeyframesButton.checked) {
            filter.resetProperty('octaveshift')
            filter.set('octaveshift', middleValue)
        } else if (position !== null) {
            filter.set('octaveshift', octaveSlider.value, position)
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
            Layout.columnSpan: parent.columns - 1
            parameters: ['octaveshift']
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
            text: qsTr('Octave Shift')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Specify the pitch shift in octaves.\n-1 shifts down an octave.\n+1 shifts up an octave.\n0 is unchanged.') }
        }
        Shotcut.SliderSpinner {
            id: octaveSlider
            minimumValue: -2.0
            maximumValue: 2.0
            decimals: 6
            stepSize: 1.0/12.0 // 12 half steps in an octave
            spinnerWidth: 100
            onValueChanged: {
                updateFilter(getPosition())
                if (frequencySlider.noUpdate == false) {
                    frequencySlider.value = 1.0 / Math.pow(2, value)
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: octaveSlider.value = 0.0
        }
        Shotcut.KeyframesButton {
            id: octaveKeyframesButton
            onToggled: {
                if (checked) {
                    blockUpdate = true
                    filter.clearSimpleAnimation('octaveshift')
                    blockUpdate = false
                    filter.set('octaveshift', octaveSlider.value, getPosition())
                } else {
                    filter.resetProperty('octaveshift')
                    filter.set('octaveshift', octaveSlider.value)
                }
            }
        }

        Label {
            text: qsTr('Speed Compensation')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Specify the speed change that should be compensated for.\n2x will halve the pitch to compensate for the speed being doubled.') }
        }
        Shotcut.SliderSpinner {
            property bool noUpdate: false
            id: frequencySlider
            minimumValue: 0.25
            maximumValue: 4.0
            decimals: 6
            spinnerWidth: 100
            suffix: ' x'
            enabled: octaveSlider.enabled
            onValueChanged: {
                noUpdate = true
                octaveSlider.value = Math.log(1.0 / value) / Math.log(2)
                noUpdate = false
            }
        }
        Item {
            Layout.columnSpan: 2
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
