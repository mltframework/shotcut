/*
 * Copyright (c) 2019-2022 Meltytech, LLC
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
    property string noise: '0'
    property double noiseDefault: 0.2

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        noiseSlider.value = filter.getDouble(noise, position) * noiseSlider.maximumValue;
        noiseKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(noise) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        noiseSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setControls();
        updateFilter(noise, noiseSlider.value / noiseSlider.maximumValue, noiseKeyframesButton, null);
    }

    keyframableParameters: [noise]
    startValues: [0]
    middleValues: [noiseDefault]
    endValues: [0]
    width: 350
    height: 100
    Component.onCompleted: {
        filter.set('threads', 0);
        if (filter.isNew) {
            filter.set(noise, noiseDefault);
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

            parameters: [noise]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty(noise);
            }
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        Label {
            text: qsTr('Amount')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: noiseSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(noise, noiseSlider.value / noiseSlider.maximumValue, noiseKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: noiseSlider.value = noiseDefault * noiseSlider.maximumValue
        }

        Shotcut.KeyframesButton {
            id: noiseKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, noise, noiseSlider.value / noiseSlider.maximumValue);
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
