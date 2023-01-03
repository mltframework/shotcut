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
    property string verSplit: '0'
    property string horSplit: '1'
    property double verSplitDefault: 0.4
    property double horSplitDefault: 0.4

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        verSplitSlider.value = filter.getDouble(verSplit, position) * verSplitSlider.maximumValue;
        verKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(verSplit) > 0;
        horSplitSlider.value = filter.getDouble(horSplit, position) * horSplitSlider.maximumValue;
        horKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(horSplit) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        verSplitSlider.enabled = horSplitSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setControls();
        updateFilter(verSplit, verSplitSlider.value / verSplitSlider.maximumValue, verKeyframesButton, null);
        updateFilter(horSplit, horSplitSlider.value / horSplitSlider.maximumValue, horKeyframesButton, null);
    }

    keyframableParameters: [verSplit, horSplit]
    startValues: [0.5, 0.5]
    middleValues: [verSplitDefault, horSplitDefault]
    endValues: [0.5, 0.5]
    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(verSplit, verSplitDefault);
            filter.set(horSplit, horSplitDefault);
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

            parameters: [verSplit, horSplit]
            Layout.columnSpan: 3
            onBeforePresetLoaded: resetSimpleKeyframes()
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
            id: verSplitSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(verSplit, value / maximumValue, verKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: verSplitSlider.value = verSplitDefault * verSplitSlider.maximumValue
        }

        Shotcut.KeyframesButton {
            id: verKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, verSplit, verSplitSlider.value / verSplitSlider.maximumValue);
            }
        }

        Label {
            text: qsTr('Horizontal')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: horSplitSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(horSplit, value / maximumValue, horKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: horSplitSlider.value = horSplitDefault * horSplitSlider.maximumValue
        }

        Shotcut.KeyframesButton {
            id: horKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, horSplit, horSplitSlider.value / horSplitSlider.maximumValue);
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
