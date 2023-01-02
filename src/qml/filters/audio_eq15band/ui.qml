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
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Shotcut.KeyframableFilter {
    property bool blockControls: false

    function setControls() {
        if (blockControls)
            return;
        var position = getPosition();
        blockUpdate = true;
        slider0.value = filter.getDouble('0', position);
        slider1.value = filter.getDouble('1', position);
        slider2.value = filter.getDouble('2', position);
        slider3.value = filter.getDouble('3', position);
        slider4.value = filter.getDouble('4', position);
        slider5.value = filter.getDouble('5', position);
        slider6.value = filter.getDouble('6', position);
        slider7.value = filter.getDouble('7', position);
        slider8.value = filter.getDouble('8', position);
        slider9.value = filter.getDouble('9', position);
        slider10.value = filter.getDouble('10', position);
        slider11.value = filter.getDouble('11', position);
        slider12.value = filter.getDouble('12', position);
        slider13.value = filter.getDouble('13', position);
        slider14.value = filter.getDouble('14', position);
        keyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('0') > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        slider0.enabled = slider1.enabled = slider2.enabled = slider3.enabled = slider4.enabled = slider5.enabled = slider6.enabled = slider7.enabled = slider8.enabled = slider9.enabled = slider10.enabled = slider11.enabled = slider12.enabled = slider13.enabled = slider14.enabled = enabled;
    }

    function updateSimpleKeyframes(position) {
        if (blockUpdate)
            return;
        setControls();
        updateFilter('0', slider0.value, keyframesButton, position);
        updateFilter('1', slider1.value, keyframesButton, position);
        updateFilter('2', slider2.value, keyframesButton, position);
        updateFilter('3', slider3.value, keyframesButton, position);
        updateFilter('4', slider4.value, keyframesButton, position);
        updateFilter('5', slider5.value, keyframesButton, position);
        updateFilter('6', slider6.value, keyframesButton, position);
        updateFilter('7', slider7.value, keyframesButton, position);
        updateFilter('8', slider8.value, keyframesButton, position);
        updateFilter('9', slider9.value, keyframesButton, position);
        updateFilter('10', slider10.value, keyframesButton, position);
        updateFilter('11', slider11.value, keyframesButton, position);
        updateFilter('12', slider12.value, keyframesButton, position);
        updateFilter('13', slider13.value, keyframesButton, position);
        updateFilter('14', slider14.value, keyframesButton, position);
    }

    keyframableParameters: ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14']
    startValues: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    middleValues: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    endValues: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    width: 200
    height: 430
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('0', 0);
            filter.set('1', 0);
            filter.set('2', 0);
            filter.set('3', 0);
            filter.set('4', 0);
            filter.set('5', 0);
            filter.set('6', 0);
            filter.set('7', 0);
            filter.set('8', 0);
            filter.set('9', 0);
            filter.set('10', 0);
            filter.set('11', 0);
            filter.set('12', 0);
            filter.set('13', 0);
            filter.set('14', 0);
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

            parameters: keyframableParameters
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
            text: qsTr('50 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider0

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
            onClicked: slider0.value = 0
        }

        ColumnLayout {
            Layout.rowSpan: 15
            height: (keyframesButton.height + 5) * Layout.rowSpan

            SystemPalette {
                id: activePalette
            }

            Rectangle {
                color: activePalette.text
                width: 1
                height: parent.height / 2
                Layout.alignment: Qt.AlignHCenter
            }

            Shotcut.KeyframesButton {
                id: keyframesButton

                onToggled: {
                    toggleKeyframes(checked, '0', slider0.value);
                    toggleKeyframes(checked, '1', slider1.value);
                    toggleKeyframes(checked, '2', slider2.value);
                    toggleKeyframes(checked, '3', slider3.value);
                    toggleKeyframes(checked, '4', slider4.value);
                    toggleKeyframes(checked, '5', slider5.value);
                    toggleKeyframes(checked, '6', slider6.value);
                    toggleKeyframes(checked, '7', slider7.value);
                    toggleKeyframes(checked, '8', slider8.value);
                    toggleKeyframes(checked, '9', slider9.value);
                    toggleKeyframes(checked, '10', slider10.value);
                    toggleKeyframes(checked, '11', slider11.value);
                    toggleKeyframes(checked, '12', slider12.value);
                    toggleKeyframes(checked, '13', slider13.value);
                    toggleKeyframes(checked, '14', slider14.value);
                    setControls();
                }
            }

            Rectangle {
                color: activePalette.text
                width: 1
                height: parent.height / 2
                Layout.alignment: Qt.AlignHCenter
            }
        }

        Label {
            text: qsTr('100 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider1

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
            onClicked: slider1.value = 0
        }

        Label {
            text: qsTr('156 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider2

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
            onClicked: slider2.value = 0
        }

        Label {
            text: qsTr('220 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider3

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
            onClicked: slider3.value = 0
        }

        Label {
            text: qsTr('311 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider4

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
            onClicked: slider4.value = 0
        }

        Label {
            text: qsTr('440 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider5

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
            onClicked: slider5.value = 0
        }

        Label {
            text: qsTr('622 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider6

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
            onClicked: slider6.value = 0
        }

        Label {
            text: qsTr('880 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider7

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
            onClicked: slider7.value = 0
        }

        Label {
            text: qsTr('1250 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider8

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
            onClicked: slider8.value = 0
        }

        Label {
            text: qsTr('1750 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider9

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
            onClicked: slider9.value = 0
        }

        Label {
            text: qsTr('2500 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider10

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
            onClicked: slider10.value = 0
        }

        Label {
            text: qsTr('3500 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider11

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
            onClicked: slider11.value = 0
        }

        Label {
            text: qsTr('5000 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider12

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
            onClicked: slider12.value = 0
        }

        Label {
            text: qsTr('10000 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider13

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
            onClicked: slider13.value = 0
        }

        Label {
            text: qsTr('20000 Hz')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider14

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
            onClicked: slider14.value = 0
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
