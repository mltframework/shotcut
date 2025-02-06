/*
 * Copyright (c) 2019-2024 Meltytech, LLC
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
    property string vertical: '0'
    property double verticalDefault: 0.2
    property string horizontal: '1'
    property double horizontalDefault: 0.0

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        verticalSlider.value = filter.getDouble(vertical, position) * verticalSlider.maximumValue;
        verticalKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(vertical) > 0;
        horizontalSlider.value = filter.getDouble(horizontal, position) * horizontalSlider.maximumValue;
        horizontalKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(horizontal) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        verticalSlider.enabled = enabled;
        horizontalSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setControls();
        updateFilter(vertical, verticalSlider.value / verticalSlider.maximumValue, verticalKeyframesButton, null);
        updateFilter(horizontal, horizontalSlider.value / horizontalSlider.maximumValue, horizontalKeyframesButton, null);
    }

    keyframableParameters: [vertical, horizontal]
    startValues: [0, 0]
    middleValues: [verticalDefault, horizontalDefault]
    endValues: [0, 0]
    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(vertical, verticalDefault);
            filter.set(horizontal, horizontalDefault);
            filter.savePreset(preset.parameters);
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

            parameters: [vertical, horizontal]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty(vertical);
                filter.resetProperty(horizontal);
            }
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        Label {
            text: qsTr('Vertical')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: verticalSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 0.01
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(vertical, verticalSlider.value / verticalSlider.maximumValue, verticalKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: verticalSlider.value = verticalDefault * verticalSlider.maximumValue
        }

        Shotcut.KeyframesButton {
            id: verticalKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, vertical, verticalSlider.value / verticalSlider.maximumValue);
            }
        }

        Label {
            text: qsTr('Horizontal')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: horizontalSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 0.01
            decimals: 2
            suffix: ' %'
            onValueChanged: updateFilter(horizontal, horizontalSlider.value / horizontalSlider.maximumValue, horizontalKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: horizontalSlider.value = horizontalDefault * horizontalSlider.maximumValue
        }

        Shotcut.KeyframesButton {
            id: horizontalKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, horizontal, horizontalSlider.value / horizontalSlider.maximumValue);
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
