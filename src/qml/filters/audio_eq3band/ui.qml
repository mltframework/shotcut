/*
 * Copyright (c) 2022-2025 Meltytech, LLC
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

    // The three band EQ is implemented by using a 3 band parametric
    // EQ with shelves. Many of the parametric EQ parameters are fixed
    // and the user only has control over three gain parameters for
    // Low, Mid and High tone adjustment
    property bool blockControls: false

    function setControls() {
        if (blockControls)
            return;
        var position = getPosition();
        blockUpdate = true;
        lowSlider.value = filter.getDouble('0', position);
        midSlider.value = filter.getDouble('6', position);
        highSlider.value = filter.getDouble('12', position);
        keyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('0') > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        lowSlider.enabled = midSlider.enabled = highSlider.enabled = enabled;
    }

    function updateSimpleKeyframes(position) {
        if (blockUpdate)
            return;
        setControls();
        updateFilter('0', lowSlider.value, keyframesButton, position);
        updateFilter('6', midSlider.value, keyframesButton, position);
        updateFilter('12', highSlider.value, keyframesButton, position);
    }

    keyframableParameters: ['0', '6', '12']
    startValues: [0, 0, 0]
    middleValues: [0, 0, 0]
    endValues: [0, 0, 0]
    width: 200
    height: 130
    Component.onCompleted: {
        if (filter.isNew) {
            // Low Shelf - adjusts Low tone
            filter.set('0', 0);
            // gain      - adjustable
            filter.set('1', 200);
            // frequency - fixed
            filter.set('2', 0.5);
            // slope     - fixed
            // Band 1 - not used
            filter.set('3', 0);
            // gain      - fixed
            filter.set('4', 1000);
            // frequency - fixed
            filter.set('5', 1);
            // Q         - fixed
            // Band 2 - adjusts Mid tone
            filter.set('6', 0);
            // gain      - adjustable
            filter.set('7', 1000);
            // frequency - fixed
            filter.set('8', 1);
            // Q         - fixed
            // Band 3 - not used
            filter.set('9', 0);
            // gain      - fixed
            filter.set('10', 1000);
            // frequency - fixed
            filter.set('11', 1);
            // Q         - fixed
            // High Shelf - adjusts High tone
            filter.set('12', 0);
            // gain      - adjustable
            filter.set('13', 5000);
            // frequency - fixed
            filter.set('14', 0.5);
            // slope     - fixed
            filter.set('.dummy', '');
            filter.savePreset(preset.parameters);
        }
        setControls();
        channelMask.setChannelsControls();
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

            parameters: keyframableParameters.concat('.dummy', channelMask.channelMaskProperty)
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
                channelMask.setChannelsControls();
            }
        }

        Label {
            text: qsTr('Low')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: lowSlider

            minimumValue: -20
            maximumValue: 20
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: {
                blockControls = true;
                updateSimpleKeyframes(getPosition());
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: lowSlider.value = 0
        }

        ColumnLayout {
            Layout.rowSpan: 3
            height: (keyframesButton.height + 5) * Layout.rowSpan

            SystemPalette {
                id: activePalette
            }

            Rectangle {
                color: activePalette.text
                width: 1
                height: keyframesButton.height
                Layout.alignment: Qt.AlignHCenter
            }

            Shotcut.KeyframesButton {
                id: keyframesButton

                onToggled: {
                    toggleKeyframes(checked, '0', lowSlider.value);
                    toggleKeyframes(checked, '6', midSlider.value);
                    toggleKeyframes(checked, '12', highSlider.value);
                    setControls();
                }
            }

            Rectangle {
                color: activePalette.text
                width: 1
                height: keyframesButton.height
                Layout.alignment: Qt.AlignHCenter
            }
        }

        Label {
            text: qsTr('Mid')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: midSlider

            minimumValue: -20
            maximumValue: 20
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: {
                blockControls = true;
                updateSimpleKeyframes(getPosition());
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: midSlider.value = 0
        }

        Label {
            text: qsTr('High')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: highSlider

            minimumValue: -20
            maximumValue: 20
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: {
                blockControls = true;
                updateSimpleKeyframes(getPosition());
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: highSlider.value = 0
        }

        Label {
            text: qsTr('Channels')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ChannelMask {
            id: channelMask

            Layout.columnSpan: 2
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
