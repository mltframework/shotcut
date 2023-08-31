/*
 * Copyright (c) 2020-2023 Meltytech, LLC
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

Item {
    property bool blockUpdate: true
    property double startValue: 0.5
    property double middleValue: 0.5
    property double endValue: 0.5

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        slider.value = filter.getDouble('radius', position) * slider.maximumValue;
        colorSwatch.value = filter.getColor('color', position);
        keyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('radius') > 0;
        colorKeyframesButton.checked = filter.keyframeCount('color') > 0;
        blockUpdate = false;
        slider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateSimpleKeyframes(position) {
        if (blockUpdate)
            return;
        var value = slider.value / 100;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value;
            else
                middleValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('radius');
            keyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('radius', startValue, 0);
                filter.set('radius', middleValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('radius', middleValue, filter.duration - filter.animateOut);
                filter.set('radius', endValue, filter.duration - 1);
            }
        } else if (!keyframesButton.checked) {
            filter.resetProperty('radius');
            filter.set('radius', middleValue);
        } else if (position !== null) {
            filter.set('radius', value, position);
        }
    }

    function updateFilter(parameter, value, button, position) {
        if (blockUpdate)
            return;
        if (button.checked && position !== null) {
            filter.set(parameter, value, position);
        } else if (position !== null) {
            filter.set(parameter, value);
        }
    }

    function toggleKeyframes(isEnabled, parameter, value) {
        if (isEnabled) {
            blockUpdate = true;
            filter.clearSimpleAnimation(parameter);
            blockUpdate = false;
            // Set this keyframe value.
            filter.set(parameter, value, getPosition());
        } else {
            // Remove keyframes and set the parameter.
            filter.resetProperty(parameter);
            filter.set(parameter, value);
        }
    }

    width: 400
    height: 100
    Component.onCompleted: {
        filter.set('circle', 1);
        if (filter.isNew) {
            // Set default parameter values
            filter.set('color', '#ff000000');
            filter.set('radius', 0.5);
        } else {
            middleValue = filter.getDouble('radius', filter.animateIn);
            if (filter.animateIn > 0)
                startValue = filter.getDouble('radius', 0);
            if (filter.animateOut > 0)
                endValue = filter.getDouble('radius', filter.duration - 1);
        }
        setControls();
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: slider.value = 50
        }

        Shotcut.KeyframesButton {
            id: keyframesButton

            onToggled: {
                var value = slider.value / 100;
                if (checked) {
                    blockUpdate = true;
                    filter.clearSimpleAnimation('radius');
                    blockUpdate = false;
                    filter.set('radius', value, getPosition());
                } else {
                    filter.resetProperty('radius');
                    filter.set('radius', value);
                }
            }
        }

        Label {
            text: qsTr('Color')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.ColorPicker {
                id: colorSwatch

                property bool isReady: false

                alpha: true
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        updateFilter('color', value, colorKeyframesButton, getPosition());
                        filter.set("disable", 0);
                    }
                }
                onPickStarted: {
                    filter.set('disable', 1);
                }
                onPickCancelled: filter.set('disable', 0)
            }

            Shotcut.Button {
                text: qsTr('Transparent')
                onClicked: colorSwatch.value = Qt.rgba(0, 0, 0, 0)
            }
        }

        Shotcut.UndoButton {
            onClicked: colorSwatch.value = Qt.rgba(0, 0, 0, 1)
        }

        Shotcut.KeyframesButton {
            id: colorKeyframesButton
            onToggled: toggleKeyframes(checked, 'color', colorSwatch.value)
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
            updateSimpleKeyframes(null);
        }

        function onOutChanged() {
            updateSimpleKeyframes(null);
        }

        function onAnimateInChanged() {
            updateSimpleKeyframes(null);
        }

        function onAnimateOutChanged() {
            updateSimpleKeyframes(null);
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
}
