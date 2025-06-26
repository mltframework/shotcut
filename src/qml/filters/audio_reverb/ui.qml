/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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
    property string roomProperty: '0'
    property string timeProperty: '1'
    property string dampProperty: '2'
    property string inputProperty: '3'
    property string dryProperty: '4'
    property string reflectionProperty: '5'
    property string tailProperty: '6'

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        sliderRoom.value = filter.getDouble(roomProperty, position);
        roomKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(roomProperty) > 0;
        sliderTime.value = filter.getDouble(timeProperty, position);
        timeKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(timeProperty) > 0;
        sliderDamp.value = filter.getDouble(dampProperty, position) * sliderDamp.maximumValue;
        dampKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(dampProperty) > 0;
        sliderInput.value = filter.getDouble(inputProperty, position) * sliderInput.maximumValue;
        inputKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(inputProperty) > 0;
        sliderDry.value = filter.getDouble(dryProperty, position);
        dryKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(dryProperty) > 0;
        sliderReflection.value = filter.getDouble(reflectionProperty, position);
        reflectionKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(reflectionProperty) > 0;
        sliderTail.value = filter.getDouble(tailProperty, position);
        tailKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(tailProperty) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        sliderRoom.enabled = sliderTime.enabled = sliderDamp.enabled = sliderInput.enabled = sliderDry.enabled = sliderReflection.enabled = sliderTail.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        updateFilter(roomProperty, sliderRoom.value, roomKeyframesButton, null);
        updateFilter(timeProperty, sliderTime.value, timeKeyframesButton, null);
        updateFilter(dampProperty, sliderDamp.value / sliderDamp.maximumValue, dampKeyframesButton, null);
        updateFilter(inputProperty, sliderInput.value / sliderInput.maximumValue, inputKeyframesButton, null);
        updateFilter(dryProperty, sliderDry.value, dryKeyframesButton, null);
        updateFilter(reflectionProperty, sliderReflection.value, reflectionKeyframesButton, null);
        updateFilter(tailProperty, sliderTail.value, tailKeyframesButton, null);
    }

    width: 350
    height: 250
    keyframableParameters: preset.parameters
    startValues: [1, 0.1, 0, 0, -70, -70, -70]
    middleValues: [30, 7.5, 0.5, 0.75, 0, -10, -17.5]
    endValues: [1, 0.1, 0, 0, -70, -70, -70]
    Component.onCompleted: {
        filter.blockSignals = true;
        if (filter.isNew) {
            // Set preset parameter values
            filter.set('0', 40);
            filter.set('1', 4);
            filter.set('2', 0.9);
            filter.set('3', 0.75);
            filter.set('4', 0);
            filter.set('5', -22);
            filter.set('6', -28);
            filter.savePreset(preset.parameters, qsTr('Quick fix'));
            filter.set('0', 50);
            filter.set('1', 1.5);
            filter.set('2', 0.1);
            filter.set('3', 0.75);
            filter.set('4', -1.5);
            filter.set('5', -10);
            filter.set('6', -20);
            filter.savePreset(preset.parameters, qsTr('Small hall'));
            filter.set('0', 40);
            filter.set('1', 20);
            filter.set('2', 0.5);
            filter.set('3', 0.75);
            filter.set('4', 0);
            filter.set('5', -10);
            filter.set('6', -30);
            filter.savePreset(preset.parameters, qsTr('Large hall'));
            filter.set('0', 6);
            filter.set('1', 15);
            filter.set('2', 0.9);
            filter.set('3', 0.1);
            filter.set('4', -10);
            filter.set('5', -10);
            filter.set('6', -10);
            filter.savePreset(preset.parameters, qsTr('Sewer'));
            filter.set('0', 6);
            filter.set('1', 15);
            filter.set('2', 0.9);
            filter.set('3', 0.1);
            filter.set('4', -10);
            filter.set('5', -10);
            filter.set('6', -10);
            filter.savePreset(preset.parameters, qsTr('Church'));
            // Set default parameter values
            filter.set('0', 30);
            filter.set('1', 7.5);
            filter.set('2', 0.5);
            filter.set('3', 0.75);
            filter.set('4', 0);
            filter.set('5', -10);
            filter.set('6', -17.5);
            filter.savePreset(preset.parameters);
        }
        filter.blockSignals = false;
        filter.changed();
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

            parameters: ['0', '1', '2', '3', '4', '5', '6']
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
            text: qsTr('Room size')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The size of the room, in meters. Excessively large, and excessively small values will make it sound a bit unrealistic. Values of around 30 sound good.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderRoom

            minimumValue: 1
            maximumValue: 300
            suffix: ' m'
            onValueChanged: updateFilter(roomProperty, value, roomKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sliderRoom.value = 30
        }

        Shotcut.KeyframesButton {
            id: roomKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, roomProperty, sliderRoom.value);
            }
        }

        Label {
            text: qsTr('Reverb time')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderTime

            minimumValue: 0.1
            maximumValue: 30
            decimals: 1
            suffix: ' s'
            onValueChanged: updateFilter(timeProperty, value, timeKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sliderTime.value = 7.5
        }

        Shotcut.KeyframesButton {
            id: timeKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, timeProperty, sliderTime.value);
            }
        }

        Label {
            text: qsTr('Damping')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('This controls the high frequency damping (a lowpass filter), values near 1 will make it sound very bright, values near 0 will make it sound very dark.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderDamp

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(dampProperty, value / maximumValue, dampKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sliderDamp.value = 0.5 * sliderDamp.maximumValue
        }

        Shotcut.KeyframesButton {
            id: dampKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, dampProperty, sliderDamp.value / sliderDamp.maximumValue);
            }
        }

        Label {
            text: qsTr('Input bandwidth')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('This is like a damping control for the input, it has a similar effect to the damping control, but is subtly different.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderInput

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(inputProperty, value / maximumValue, inputKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sliderInput.value = 0.75 * sliderInput.maximumValue
        }

        Shotcut.KeyframesButton {
            id: inputKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, inputProperty, sliderInput.value / sliderInput.maximumValue);
            }
        }

        Label {
            text: qsTr('Dry signal level')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount of dry signal to be mixed with the reverberated signal.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderDry

            minimumValue: -70
            maximumValue: 0
            suffix: ' dB'
            decimals: 1
            onValueChanged: updateFilter(dryProperty, value, dryKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sliderDry.value = 0
        }

        Shotcut.KeyframesButton {
            id: dryKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, dryProperty, sliderDry.value);
            }
        }

        Label {
            text: qsTr('Early reflection level')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The distance from the threshold where the knee curve starts.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderReflection

            minimumValue: -70
            maximumValue: 0
            suffix: ' dB'
            onValueChanged: updateFilter(reflectionProperty, value, reflectionKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sliderReflection.value = -10
        }

        Shotcut.KeyframesButton {
            id: reflectionKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, reflectionProperty, sliderReflection.value);
            }
        }

        Label {
            text: qsTr('Tail level')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The quantity of early reflections (scatter reflections directly from the source).')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderTail

            minimumValue: -70
            maximumValue: 0
            decimals: 1
            suffix: ' dB'
            onValueChanged: updateFilter(tailProperty, value, tailKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sliderTail.value = -17.5
        }

        Shotcut.KeyframesButton {
            id: tailKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, tailProperty, sliderTail.value);
            }
        }

        Label {
            Layout.columnSpan: parent.columns
            text: qsTr('About reverb')
            font.underline: true

            MouseArea {
                anchors.fill: parent
                onClicked: Qt.openUrlExternally('https://web.archive.org/web/20221113010916/https://wiki.audacityteam.org/wiki/GVerb')
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
