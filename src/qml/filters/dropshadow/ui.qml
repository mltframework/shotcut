/*
 * Copyright (c) 2024 Meltytech, LLC
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

Shotcut.KeyframableFilter {
    property color colorDefault: Qt.rgba(0, 0, 0, 180 / 255)
    property double radiusDefault: 8.0
    property double xOffsetDefault: 8.0
    property double yOffsetDefault: 8.0

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        colorPicker.value = filter.getColor('color', position);
        colorKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('color') > 0;
        radiusSlider.value = filter.getDouble('radius', position);
        radiusKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('radius') > 0;
        xOffsetSlider.value = filter.getDouble('x', position);
        xOffsetKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('x') > 0;
        yOffsetSlider.value = filter.getDouble('y', position);
        yOffsetKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('y') > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        colorPicker.enabled = enabled;
        radiusSlider.enabled = enabled;
        xOffsetSlider.enabled = enabled;
        yOffsetSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setControls();
        updateFilter('color', colorPicker.value, colorKeyframesButton, null);
        updateFilter('radius', radiusSlider.value, radiusKeyframesButton, null);
        updateFilter('x', xOffsetSlider.value, xOffsetKeyframesButton, null);
        updateFilter('y', yOffsetSlider.value, yOffsetKeyframesButton, null);
    }

    keyframableParameters: ['color', 'radius', 'x', 'y']
    startValues: [colorDefault, radiusDefault, xOffsetDefault, yOffsetDefault]
    middleValues: [colorDefault, radiusDefault, xOffsetDefault, yOffsetDefault]
    endValues: [colorDefault, radiusDefault, xOffsetDefault, yOffsetDefault]
    width: 200
    height: 150
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('color', colorDefault);
            filter.set('radius', radiusDefault);
            filter.set('x', xOffsetDefault);
            filter.set('y', yOffsetDefault);
            filter.savePreset(keyframableParameters);
        }
        setControls();
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
            id: presetItem

            Layout.columnSpan: 3
            parameters: keyframableParameters
            onBeforePresetLoaded: resetSimpleKeyframes()
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        Label {
            text: qsTr('Color')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ColorPicker {
            id: colorPicker

            property bool isReady: false

            alpha: true
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    updateFilter('color', Qt.color(value), colorKeyframesButton, getPosition());
                    filter.set('disable', 0);
                }
            }
            onPickStarted: filter.set('disable', 1)
            onPickCancelled: filter.set('disable', 0)
        }

        Shotcut.UndoButton {
            onClicked: colorPicker.value = colorDefault
        }

        Shotcut.KeyframesButton {
            id: colorKeyframesButton

            onToggled: toggleKeyframes(checked, 'color', Qt.color(colorPicker.value))
        }

        Label {
            text: qsTr('Blur')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: radiusSlider

            minimumValue: -100
            maximumValue: 100
            decimals: 1
            suffix: 'px'
            onValueChanged: updateFilter('radius', value, radiusKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: radiusSlider.value = radiusDefault
        }

        Shotcut.KeyframesButton {
            id: radiusKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'radius', radiusSlider.value);
            }
        }

        Label {
            text: qsTr('X')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: xOffsetSlider

            minimumValue: -100
            maximumValue: 100
            decimals: 1
            suffix: ' px'
            onValueChanged: updateFilter('x', value, xOffsetKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: xOffsetSlider.value = xOffsetDefault
        }

        Shotcut.KeyframesButton {
            id: xOffsetKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'x', xOffsetSlider.value);
            }
        }

        Label {
            text: qsTr('Y')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: yOffsetSlider

            minimumValue: -100
            maximumValue: 100
            decimals: 1
            suffix: ' px'
            onValueChanged: updateFilter('y', value, yOffsetKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: yOffsetSlider.value = yOffsetDefault
        }

        Shotcut.KeyframesButton {
            id: yOffsetKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'y', yOffsetSlider.value);
            }
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
}
