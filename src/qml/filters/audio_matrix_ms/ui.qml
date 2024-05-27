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
    property string widthProperty: '0'
    property string wetnessProperty: 'wetness'

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        widthSlider.value = filter.getDouble(widthProperty, position) * 50;
        widthKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(widthProperty) > 0;
        sliderWetness.value = filter.getDouble(wetnessProperty, position) * sliderWetness.maximumValue;
        wetnessKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(wetnessProperty) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        widthSlider.enabled = sliderWetness.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        updateFilter(widthProperty, widthSlider.value, widthKeyframesButton, null);
        updateFilter(wetnessProperty, sliderWetness.value / sliderWetness.maximumValue, wetnessKeyframesButton, null);
    }

    width: 350
    height: 100
    keyframableParameters: preset.parameters
    startValues: [1, 0]
    middleValues: [1, 1]
    endValues: [1, 0]
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set(widthProperty, 1);
            filter.set(wetnessProperty, 1);
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

            parameters: [widthProperty, wetnessProperty]
            Layout.columnSpan: parent.columns - 1
            onBeforePresetLoaded: {
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        Label {
            text: qsTr('Width')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: widthSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter(widthProperty, value / 50, widthKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: widthSlider.value = 50
        }

        Shotcut.KeyframesButton {
            id: widthKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, widthProperty, widthSlider.value / 50);
            }
        }

        Label {
            text: qsTr('Dry')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderWetness

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            label: qsTr('Wet')
            suffix: ' %'
            onValueChanged: updateFilter(wetnessProperty, value / maximumValue, wetnessKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sliderWetness.value = sliderWetness.maximumValue
        }

        Shotcut.KeyframesButton {
            id: wetnessKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, wetnessProperty, sliderWetness.value / sliderWetness.maximumValue);
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
