/*
 * Copyright (c) 2019-2021 Meltytech, LLC
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
    property string threshold: 'midpoint'
    property double thresholdDefault: 128

    function setControls() {
        setKeyframableControls();
        invertCheckbox.checked = filter.get('invert') === '1';
        useAlphaCheckbox.checked = filter.get('use_alpha') === '1';
    }

    function setKeyframableControls() {
        var position = getPosition();
        blockUpdate = true;
        thresholdSlider.value = filter.getDouble(threshold, position) / 255 * thresholdSlider.maximumValue;
        thresholdKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(threshold) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        thresholdSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setKeyframableControls();
        updateFilter(threshold, thresholdSlider.value / thresholdSlider.maximumValue * 255, thresholdKeyframesButton, null);
    }

    keyframableParameters: [threshold]
    startValues: [thresholdDefault]
    middleValues: [thresholdDefault]
    endValues: [thresholdDefault]
    width: 350
    height: 150
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(threshold, thresholdDefault);
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

            parameters: [threshold, 'invert', 'use_alpha']
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        Label {
            text: qsTr('Level')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: thresholdSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(threshold, thresholdSlider.value / thresholdSlider.maximumValue * 255, thresholdKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: thresholdSlider.value = thresholdDefault / 255 * thresholdSlider.maximumValue
        }

        Shotcut.KeyframesButton {
            id: thresholdKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, threshold, thresholdSlider.value / thresholdSlider.maximumValue * 255);
            }
        }

        Item {
            width: 1
        }

        CheckBox {
            id: invertCheckbox

            text: qsTr('Invert')
            onCheckedChanged: filter.set('invert', checked)
        }

        Shotcut.UndoButton {
            onClicked: invertCheckbox.checked = false
        }

        Item {
            width: 1
        }

        Label {
        }

        CheckBox {
            id: useAlphaCheckbox

            text: qsTr('Compare with alpha channel')
            onCheckedChanged: filter.set('use_alpha', checked)
        }

        Shotcut.UndoButton {
            onClicked: useAlphaCheckbox.checked = false
        }

        Item {
            width: 1
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        function onChanged() {
            setKeyframableControls();
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

        function onPropertyChanged() {
            setKeyframableControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setKeyframableControls();
        }

        target: producer
    }
}
