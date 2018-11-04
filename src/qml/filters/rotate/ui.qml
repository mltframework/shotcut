/*
 * Copyright (c) 2013-2018 Meltytech, LLC
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
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    width: 350
    height: 150
    property bool blockUpdate: true
    property double startRotateValue: 0.0
    property double middleRotateValue: 0.0
    property double endRotateValue: 0.0
    property double startScaleValue: 1.0
    property double middleScaleValue: 1.0
    property double endScaleValue: 1.0

    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('transition.fix_rotate_x', 0)
            filter.set('transition.scale_x', 1)
            filter.set('transition.ox', 0)
            filter.set('transition.oy', 0)
            filter.set('transition.threads', 0)
            filter.savePreset(preset.parameters)
        } else {
            middleRotateValue = filter.getDouble('transition.fix_rotate_x', filter.animateIn)
            middleScaleValue = filter.getDouble('transition.scale_x', filter.animateIn)
            if (filter.animateIn > 0) {
                startRotateValue = filter.getDouble('transition.fix_rotate_x', 0)
                startScaleValue = filter.getDouble('transition.scale_x', 0)
            }
            if (filter.animateOut > 0) {
                endRotateValue = filter.getDouble('transition.fix_rotate_x', filter.duration - 1)
                endScaleValue = filter.getDouble('transition.scale_x', filter.duration - 1)
            }
        }
        setControls()
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        rotationSlider.value = filter.getDouble('transition.fix_rotate_x', position)
        scaleSlider.enabled = rotationSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
        scaleSlider.value = 100 / filter.getDouble('transition.scale_x', position)
        xOffsetSlider.value = filter.getDouble('transition.ox', position) * -1
        yOffsetSlider.value = filter.getDouble('transition.oy', position) * -1
        blockUpdate = false
    }

    function updateFilterRotation(position) {
        if (blockUpdate) return
        var value = rotationSlider.value

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startRotateValue = value
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endRotateValue = value
            else
                middleRotateValue = value
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('transition.fix_rotate_x')
            rotationKeyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set('transition.fix_rotate_x', startRotateValue, 0)
                filter.set('transition.fix_rotate_x', middleRotateValue, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set('transition.fix_rotate_x', middleRotateValue, filter.duration - filter.animateOut)
                filter.set('transition.fix_rotate_x', endRotateValue, filter.duration - 1)
            }
        } else if (!rotationKeyframesButton.checked) {
            filter.resetProperty('transition.fix_rotate_x')
            filter.set('transition.fix_rotate_x', middleRotateValue)
        } else if (position !== null) {
            filter.set('transition.fix_rotate_x', value, position)
        }
    }

    function updateFilterScale(position) {
        if (blockUpdate) return
        var value = 100 / scaleSlider.value

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startScaleValue = value
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endScaleValue = value
            else
                middleScaleValue = value
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('transition.scale_x')
            filter.resetProperty('transition.scale_y')
            scaleKeyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set('transition.scale_x', startScaleValue, 0)
                filter.set('transition.scale_x', middleScaleValue, filter.animateIn - 1)
                filter.set('transition.scale_y', startScaleValue, 0)
                filter.set('transition.scale_y', middleScaleValue, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set('transition.scale_x', middleScaleValue, filter.duration - filter.animateOut)
                filter.set('transition.scale_x', endScaleValue, filter.duration - 1)
                filter.set('transition.scale_y', middleScaleValue, filter.duration - filter.animateOut)
                filter.set('transition.scale_y', endScaleValue, filter.duration - 1)
            }
        } else if (!scaleKeyframesButton.checked) {
            filter.resetProperty('transition.scale_x')
            filter.set('transition.scale_x', middleScaleValue)
            filter.resetProperty('transition.scale_y')
            filter.set('transition.scale_y', middleScaleValue)
        } else if (position !== null) {
            filter.set('transition.scale_x', value, position)
            filter.set('transition.scale_y', value, position)
        }
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
            parameters: ['transition.fix_rotate_x', 'transition.scale_x', 'transition.ox', 'transition.oy']
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty('transition.fix_rotate_x')
                filter.resetProperty('transition.scale_x')
                filter.resetProperty('transition.scale_y')
                filter.resetProperty('transition.ox')
                filter.resetProperty('transition.oy')
            }
            onPresetSelected: {
                filter.set('transition.scale_y', filter.get('transition.scale_x'))
                setControls()
                middleRotateValue = filter.getDouble('transition.fix_rotate_x', filter.animateIn)
                middleScaleValue = filter.getDouble('transition.scale_x', filter.animateIn)
                if (filter.animateIn > 0) {
                    startRotateValue = filter.getDouble('transition.fix_rotate_x', 0)
                    startScaleValue = filter.getDouble('transition.scale_x', 0)
                }
                if (filter.animateOut > 0) {
                    endRotateValue = filter.getDouble('transition.fix_rotate_x', filter.duration - 1)
                    endScaleValue = filter.getDouble('transition.scale_x', filter.duration - 1)
                }
            }
        }

        Label { text: qsTr('Rotation') }
        SliderSpinner {
            id: rotationSlider
            minimumValue: -360
            maximumValue: 360
            decimals: 1
            spinnerWidth: 110
            suffix: qsTr(' deg', 'degrees')
            onValueChanged: updateFilterRotation(getPosition())
        }
        UndoButton {
            onClicked: rotationSlider.value = 0
        }
        KeyframesButton {
            id: rotationKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('transition.fix_rotate_x') > 0
            onToggled: {
                var value = rotationSlider.value
                if (checked) {
                    blockUpdate = true
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty('transition.scale_x')
                        filter.resetProperty('transition.scale_y')
                        filter.set('transition.scale_x', middleScaleValue)
                        filter.set('transition.scale_y', middleScaleValue)
                        scaleSlider.enabled = true
                    }
                    filter.clearSimpleAnimation('transition.fix_rotate_x')
                    blockUpdate = false
                    filter.set('transition.fix_rotate_x', value, getPosition())
                } else {
                    filter.resetProperty('transition.fix_rotate_x')
                    filter.set('transition.fix_rotate_x', value)
                }
                checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('transition.fix_rotate_x') > 0
            }
        }

        Label {
            text: qsTr('Scale')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: scaleSlider
            minimumValue: 0.1
            maximumValue: 500
            decimals: 1
            spinnerWidth: 110
            suffix: ' %'
            onValueChanged: updateFilterScale(getPosition())
        }
        UndoButton {
            onClicked: scaleSlider.value = 100
        }
        KeyframesButton {
            id: scaleKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('transition.scale_x') > 0
            onToggled: {
                var value = 100 / scaleSlider.value
                if (checked) {
                    blockUpdate = true
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty('transition.fix_rotate_x')
                        filter.set('transition.fix_rotate_x', middleRotateValue)
                        rotationSlider.enabled = true
                    }
                    filter.clearSimpleAnimation('transition.scale_x')
                    filter.clearSimpleAnimation('transition.scale_y')
                    blockUpdate = false
                    filter.set('transition.scale_x', value, getPosition())
                    filter.set('transition.scale_y', value, getPosition())
                } else {
                    filter.resetProperty('transition.scale_x')
                    filter.resetProperty('transition.scale_y')
                    filter.set('transition.scale_x', value)
                    filter.set('transition.scale_y', value)
                }
                checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('transition.scale_x') > 0
            }
        }

        Label { text: qsTr('X offset') }
        SliderSpinner {
            id: xOffsetSlider
            minimumValue: -2000
            maximumValue: 2000
            spinnerWidth: 110
            onValueChanged: if (!blockUpdate) {
                if (xOffsetKeyframesButton.checked)
                    filter.set('transition.ox', -value, getPosition())
                else
                    filter.set('transition.ox', -value)
            }
        }
        UndoButton {
            onClicked: xOffsetSlider.value = 0
        }
        KeyframesButton {
            id: xOffsetKeyframesButton
            checked: filter.keyframeCount('transition.ox') > 0
            onToggled: {
                if (checked) {
                    filter.set('transition.ox', -xOffsetSlider.value, getPosition())
                } else {
                    filter.resetProperty('transition.ox')
                    filter.set('transition.ox', -xOffsetSlider.value)
                }
            }
        }

        Label { text: qsTr('Y offset') }
        SliderSpinner {
            id: yOffsetSlider
            minimumValue: -2000
            maximumValue: 2000
            spinnerWidth: 110
            onValueChanged: if (!blockUpdate) {
                if (yOffsetKeyframesButton.checked)
                    filter.set('transition.oy', -value, getPosition())
                else
                    filter.set('transition.oy', -value)
            }
        }
        UndoButton {
            onClicked: yOffsetSlider.value = 0
        }
        KeyframesButton {
            id: yOffsetKeyframesButton
            checked: filter.keyframeCount('transition.oy') > 0
            onToggled: {
                if (checked) {
                    filter.set('transition.oy', -yOffsetSlider.value, getPosition())
                } else {
                    filter.resetProperty('transition.oy')
                    filter.set('transition.oy', -yOffsetSlider.value)
                }
            }
        }

        Item {
            Layout.fillHeight: true;
        }
    }

    Connections {
        target: filter
        onChanged: setControls()
        onInChanged: { updateFilterRotation(null); updateFilterScale(null) }
        onOutChanged: { updateFilterRotation(null); updateFilterScale(null) }
        onAnimateInChanged: { updateFilterRotation(null); updateFilterScale(null) }
        onAnimateOutChanged: { updateFilterRotation(null); updateFilterScale(null) }
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
