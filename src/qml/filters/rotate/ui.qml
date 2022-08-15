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

Shotcut.KeyframableFilter {
    property bool isAtLeastVersion4: filter.isAtLeastVersion('4')

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        rotationSlider.value = filter.getDouble('transition.fix_rotate_x', position);
        rotationSlider.enabled = scaleSlider.enabled = isSimpleKeyframesActive();
        rotationKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('transition.fix_rotate_x') > 0;
        var scale = filter.getDouble('transition.scale_x', position);
        scaleSlider.value = isAtLeastVersion4 ? scale * 100 : 100 / scale;
        scaleKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('transition.scale_x') > 0;
        xOffsetSlider.value = filter.getDouble('transition.ox', position) * -1;
        xOffsetKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('transition.ox') > 0;
        yOffsetSlider.value = filter.getDouble('transition.oy', position) * -1;
        yOffsetKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('transition.oy') > 0;
        blockUpdate = false;
        var s = filter.get('background');
        if (s.substring(0, 6) === 'color:')
            bgColor.value = s.substring(6);
        else if (s.substring(0, 7) === 'colour:')
            bgColor.value = s.substring(7);
    }

    function getScaleValue() {
        if (isAtLeastVersion4)
            return scaleSlider.value / 100;
        else
            return 100 / scaleSlider.value;
    }

    function updateFilterScale(position) {
        if (blockUpdate)
            return ;

        var value = getScaleValue();
        var index = 1;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValues[index] = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValues[index] = value;
            else
                middleValues[index] = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('transition.scale_x');
            filter.resetProperty('transition.scale_y');
            scaleKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('transition.scale_x', startValues[index], 0);
                filter.set('transition.scale_x', middleValues[index], filter.animateIn - 1);
                filter.set('transition.scale_y', startValues[index], 0);
                filter.set('transition.scale_y', middleValues[index], filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('transition.scale_x', middleValues[index], filter.duration - filter.animateOut);
                filter.set('transition.scale_x', endValues[index], filter.duration - 1);
                filter.set('transition.scale_y', middleValues[index], filter.duration - filter.animateOut);
                filter.set('transition.scale_y', endValues[index], filter.duration - 1);
            }
        } else if (!scaleKeyframesButton.checked) {
            filter.resetProperty('transition.scale_x');
            filter.set('transition.scale_x', middleValues[index]);
            filter.resetProperty('transition.scale_y');
            filter.set('transition.scale_y', middleValues[index]);
        } else if (position !== null) {
            filter.set('transition.scale_x', value, position);
            filter.set('transition.scale_y', value, position);
        }
    }

    function updateSimpleKeyframes() {
        updateFilter('transition.fix_rotate_x', rotationSlider.value, rotationKeyframesButton, null);
        updateFilterScale(null);
    }

    width: 350
    height: 180
    keyframableParameters: ['transition.fix_rotate_x', 'transition.scale_x', 'transition.scale_y']
    startValues: [0, 1, 1]
    middleValues: [0, 1, 1]
    endValues: [0, 1, 1]
    Component.onCompleted: {
        if (isAtLeastVersion4 && filter.get('transition.invert_scale') != 1) {
            var scale = filter.getDouble('transition.scale_x');
            if (scale !== 0)
                filter.set('transition.scale_x', 1 / scale);

            scale = filter.getDouble('transition.scale_y');
            if (scale !== 0)
                filter.set('transition.scale_y', 1 / scale);

            filter.set('transition.invert_scale', 1);
        }
        if (filter.isNew) {
            // Set default parameter values
            filter.set('transition.fix_rotate_x', 0);
            filter.set('transition.scale_x', 1);
            filter.set('transition.scale_y', 1);
            filter.set('transition.ox', 0);
            filter.set('transition.oy', 0);
            filter.set('transition.threads', 0);
            filter.set('background', 'color:#00000000');
            filter.savePreset(preset.parameters);
        } else {
            initializeSimpleKeyframes();
        }
        setControls();
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

            parameters: ['transition.fix_rotate_x', 'transition.scale_x', 'transition.ox', 'transition.oy', 'background']
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeyframes();
                filter.resetProperty('transition.ox');
                filter.resetProperty('transition.oy');
            }
            onPresetSelected: {
                filter.set('transition.scale_y', filter.get('transition.scale_x'));
                setControls();
                initializeSimpleKeyframes();
            }
        }

        Label {
            text: qsTr('Rotation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: rotationSlider

            minimumValue: -360
            maximumValue: 360
            decimals: 1
            spinnerWidth: 110
            suffix: qsTr(' deg', 'degrees')
            onValueChanged: updateFilter('transition.fix_rotate_x', value, rotationKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: rotationSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: rotationKeyframesButton

            onToggled: {
                toggleKeyframes(checked, 'transition.fix_rotate_x', rotationSlider.value);
                setControls();
            }
        }

        Label {
            text: qsTr('Scale')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: scaleSlider

            minimumValue: 0.1
            maximumValue: 1000
            decimals: 1
            spinnerWidth: 110
            suffix: ' %'
            onValueChanged: updateFilterScale(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: scaleSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: scaleKeyframesButton

            onToggled: {
                var value = getScaleValue();
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        resetSimpleKeyframes();
                        filter.animateIn = filter.animateOut = 0;
                    } else {
                        filter.clearSimpleAnimation('transition.scale_x');
                        filter.clearSimpleAnimation('transition.scale_y');
                    }
                    blockUpdate = false;
                    filter.set('transition.scale_x', value, getPosition());
                    filter.set('transition.scale_y', value, getPosition());
                } else {
                    filter.resetProperty('transition.scale_x');
                    filter.resetProperty('transition.scale_y');
                    filter.set('transition.scale_x', value);
                    filter.set('transition.scale_y', value);
                }
                setControls();
            }
        }

        Label {
            text: qsTr('X offset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: xOffsetSlider

            minimumValue: -5000
            maximumValue: 5000
            spinnerWidth: 110
            onValueChanged: {
                if (!blockUpdate) {
                    if (xOffsetKeyframesButton.checked)
                        filter.set('transition.ox', -value, getPosition());
                    else
                        filter.set('transition.ox', -value);
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: xOffsetSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: xOffsetKeyframesButton

            onToggled: {
                if (checked) {
                    filter.set('transition.ox', -xOffsetSlider.value, getPosition());
                } else {
                    filter.resetProperty('transition.ox');
                    filter.set('transition.ox', -xOffsetSlider.value);
                }
            }
        }

        Label {
            text: qsTr('Y offset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: yOffsetSlider

            minimumValue: -5000
            maximumValue: 5000
            spinnerWidth: 110
            onValueChanged: {
                if (!blockUpdate) {
                    if (yOffsetKeyframesButton.checked)
                        filter.set('transition.oy', -value, getPosition());
                    else
                        filter.set('transition.oy', -value);
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: yOffsetSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: yOffsetKeyframesButton

            onToggled: {
                if (checked) {
                    filter.set('transition.oy', -yOffsetSlider.value, getPosition());
                } else {
                    filter.resetProperty('transition.oy');
                    filter.set('transition.oy', -yOffsetSlider.value);
                }
            }
        }

        Label {
            text: qsTr('Background color')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ColorPicker {
            id: bgColor

            eyedropper: true
            alpha: true
            onValueChanged: filter.set('background', 'color:' + value)
        }

        Shotcut.UndoButton {
            visible: bgColor.visible
            onClicked: bgColor.value = '#00000000'
        }

        Item {
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true
        }

    }

    Connections {
        function onChanged() {
            setControls();
        }

        function onInChanged() {
            updateSimpleKeyframes();
        }

        function onOutChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateInChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateOutChanged() {
            updateSimpleKeyframes();
        }

        function onPropertyChanged(name) {
            setControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setControls();
        }

        target: producer
    }

    Connections {
        function onKeyframeAdded() {
            var n = filter.getDouble(parameter, position);
            filter.set(parameter, n, position);
        }

        target: parameters
    }

}
