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
    property string lfkey: '0'
    property string hfkey: '1'
    property string threshold: '2'
    property string attack: '3'
    property string hold: '4'
    property string decay: '5'
    property string range: '6'
    property string output: '7'
    property double lfkeyDefault: 33.6
    property double hfkeyDefault: 23520
    property double thresholdDefault: -70
    property double attackDefault: 250.008
    property double holdDefault: 1500.5
    property double decayDefault: 2001
    property double rangeDefault: -90

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        lfkeySlider.value = filter.getDouble(lfkey, position);
        hfkeySlider.value = filter.getDouble(hfkey, position);
        thresholdSlider.value = filter.getDouble(threshold, position);
        attackSlider.value = filter.getDouble(attack, position);
        holdSlider.value = filter.getDouble(hold, position);
        decaySlider.value = filter.getDouble(decay, position);
        rangeSlider.value = filter.getDouble(range, position);
        lfkeyKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(lfkey) > 0;
        hfkeyKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(hfkey) > 0;
        thresholdKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(threshold) > 0;
        attackKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(attack) > 0;
        holdKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(hold) > 0;
        decayKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(decay) > 0;
        rangeKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(range) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        lfkeySlider.enabled = hfkeySlider.enabled = thresholdSlider.enabled = thresholdSlider.enabled = attackSlider.enabled = holdSlider.enabled = decaySlider.enabled = rangeSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setControls();
        updateFilter(lfkey, lfkeySlider.value, lfkeyKeyframesButton, null);
        updateFilter(hfkey, hfkeySlider.value, hfkeyKeyframesButton, null);
        updateFilter(threshold, thresholdSlider.value, thresholdKeyframesButton, null);
        updateFilter(attack, attackSlider.value, attackKeyframesButton, null);
        updateFilter(hold, holdSlider.value, holdKeyframesButton, null);
        updateFilter(decay, decaySlider.value, decayKeyframesButton, null);
        updateFilter(range, rangeSlider.value, rangeKeyframesButton, null);
    }

    keyframableParameters: [lfkey, hfkey, threshold, attack, hold, decay, range]
    startValues: [lfkeyDefault, hfkeyDefault, thresholdDefault, attackDefault, holdDefault, decayDefault, rangeDefault]
    middleValues: [lfkeyDefault, hfkeyDefault, thresholdDefault, attackDefault, holdDefault, decayDefault, rangeDefault]
    endValues: [lfkeyDefault, hfkeyDefault, thresholdDefault, attackDefault, holdDefault, decayDefault, rangeDefault]
    width: 200
    height: 250
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(lfkey, lfkeyDefault);
            filter.set(hfkey, hfkeyDefault);
            filter.set(threshold, thresholdDefault);
            filter.set(attack, attackDefault);
            filter.set(hold, holdDefault);
            filter.set(decay, decayDefault);
            filter.set(range, rangeDefault);
            filter.savePreset(preset.parameters);
        }
        setControls();
        outputCheckbox.checked = filter.get(output) === '-1';
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

            parameters: [lfkey, hfkey, threshold, attack, hold, decay, range]
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
            text: qsTr('Key Filter: Low Frequency')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: lfkeySlider

            minimumValue: 33.6
            maximumValue: 4800
            stepSize: 0.1
            decimals: 1
            suffix: ' Hz'
            onValueChanged: updateFilter(lfkey, value, lfkeyKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: lfkeySlider.value = lfkeyDefault
        }

        Shotcut.KeyframesButton {
            id: lfkeyKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, lfkey, lfkeySlider.value);
            }
        }

        Label {
            text: qsTr('Key Filter: High Frequency')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: hfkeySlider

            minimumValue: 240
            maximumValue: 23520
            stepSize: 0.1
            decimals: 1
            suffix: ' Hz'
            onValueChanged: updateFilter(hfkey, value, hfkeyKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hfkeySlider.value = hfkeyDefault
        }

        Shotcut.KeyframesButton {
            id: hfkeyKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, hfkey, hfkeySlider.value);
            }
        }

        Label {
        }

        CheckBox {
            id: outputCheckbox

            Layout.columnSpan: 3
            text: qsTr('Output key only')
            onClicked: filter.set(output, checked ? -1 : 0)
        }

        Label {
            text: qsTr('Threshold')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: thresholdSlider

            minimumValue: -70
            maximumValue: 20
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: updateFilter(threshold, value, thresholdKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: thresholdSlider.value = thresholdDefault
        }

        Shotcut.KeyframesButton {
            id: thresholdKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, threshold, thresholdSlider.value);
            }
        }

        Label {
            text: qsTr('Attack')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: attackSlider

            minimumValue: 0.01
            maximumValue: 1000
            stepSize: 1
            suffix: ' ms'
            onValueChanged: updateFilter(attack, value, attackKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: attackSlider.value = attackDefault
        }

        Shotcut.KeyframesButton {
            id: attackKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, attack, attackSlider.value);
            }
        }

        Label {
            text: qsTr('Hold')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: holdSlider

            minimumValue: 2
            maximumValue: 2000
            stepSize: 1
            suffix: ' ms'
            onValueChanged: updateFilter(hold, value, holdKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: holdSlider.value = holdDefault
        }

        Shotcut.KeyframesButton {
            id: holdKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, hold, holdSlider.value);
            }
        }

        Label {
            text: qsTr('Decay')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: decaySlider

            minimumValue: 2
            maximumValue: 4000
            stepSize: 1
            suffix: ' ms'
            onValueChanged: updateFilter(decay, value, decayKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: decaySlider.value = decayDefault
        }

        Shotcut.KeyframesButton {
            id: decayKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, decay, decaySlider.value);
            }
        }

        Label {
            text: qsTr('Range')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: rangeSlider

            minimumValue: -90
            maximumValue: 0
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: updateFilter(range, value, rangeKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: rangeSlider.value = rangeDefault
        }

        Shotcut.KeyframesButton {
            id: rangeKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, range, rangeSlider.value);
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
