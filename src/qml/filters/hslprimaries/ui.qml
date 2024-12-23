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

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        overlapSlider.value = filter.getDouble('overlap', position);
        overlapKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('overlap') > 0;
        hRedSlider.value = filter.getDouble('h_shift_red', position);
        hRedKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('h_shift_red') > 0;
        sRedSlider.value = filter.getDouble('s_scale_red', position);
        sRedKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('s_scale_red') > 0;
        lRedSlider.value = filter.getDouble('l_scale_red', position);
        lRedKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('l_scale_red') > 0;
        hYellowSlider.value = filter.getDouble('h_shift_yellow', position);
        hYellowKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('h_shift_yellow') > 0;
        sYellowSlider.value = filter.getDouble('s_scale_yellow', position);
        sYellowKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('s_scale_yellow') > 0;
        lYellowSlider.value = filter.getDouble('l_scale_yellow', position);
        lYellowKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('l_scale_yellow') > 0;
        hGreenSlider.value = filter.getDouble('h_shift_green', position);
        hGreenKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('h_shift_green') > 0;
        sGreenSlider.value = filter.getDouble('s_scale_green', position);
        sGreenKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('s_scale_green') > 0;
        lGreenSlider.value = filter.getDouble('l_scale_green', position);
        lGreenKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('l_scale_green') > 0;
        hCyanSlider.value = filter.getDouble('h_shift_cyan', position);
        hCyanKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('h_shift_cyan') > 0;
        sCyanSlider.value = filter.getDouble('s_scale_cyan', position);
        sCyanKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('s_scale_cyan') > 0;
        lCyanSlider.value = filter.getDouble('l_scale_cyan', position);
        lCyanKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('l_scale_cyan') > 0;
        hBlueSlider.value = filter.getDouble('h_shift_blue', position);
        hBlueKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('h_shift_blue') > 0;
        sBlueSlider.value = filter.getDouble('s_scale_blue', position);
        sBlueKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('s_scale_blue') > 0;
        lBlueSlider.value = filter.getDouble('l_scale_blue', position);
        lBlueKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('l_scale_blue') > 0;
        hMagentaSlider.value = filter.getDouble('h_shift_magenta', position);
        hMagentaKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('h_shift_magenta') > 0;
        sMagentaSlider.value = filter.getDouble('s_scale_magenta', position);
        sMagentaKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('s_scale_magenta') > 0;
        lMagentaSlider.value = filter.getDouble('l_scale_magenta', position);
        lMagentaKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('l_scale_magenta') > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        overlapSlider.enabled = enabled;
        hRedSlider.enabled = sRedSlider.enabled = lRedSlider.value.enabled = enabled;
        hYellowSlider.enabled = sYellowSlider.enabled = lYellowSlider.enabled = enabled;
        hGreenSlider.enabled = sGreenSlider.enabled = lGreenSlider.enabled = enabled;
        hCyanSlider.enabled = sCyanSlider.enabled = lCyanSlider.enabled = enabled;
        hBlueSlider.enabled = sBlueSlider.enabled = lBlueSlider.enabled = enabled;
        hMagentaSlider.enabled = sMagentaSlider.enabled = lMagentaSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        updateFilter('overlap', overlapSlider.value, overlapKeyframesButton, null);
        updateFilter('h_shift_red', hRedSlider.value, hRedKeyframesButton, null);
        updateFilter('s_scale_red', sRedSlider.value, sRedKeyframesButton, null);
        updateFilter('l_scale_red', lRedSlider.value, lRedKeyframesButton, null);
        updateFilter('h_shift_yellow', hYellowSlider.value, hYellowKeyframesButton, null);
        updateFilter('s_scale_yellow', sYellowSlider.value, sYellowKeyframesButton, null);
        updateFilter('l_scale_yellow', lYellowSlider.value, lYellowKeyframesButton, null);
        updateFilter('h_shift_green', hGreenSlider.value, hGreenKeyframesButton, null);
        updateFilter('s_scale_green', sGreenSlider.value, sGreenKeyframesButton, null);
        updateFilter('l_scale_green', lGreenSlider.value, lGreenKeyframesButton, null);
        updateFilter('h_shift_cyan', hCyanSlider.value, hCyanKeyframesButton, null);
        updateFilter('s_scale_cyan', sCyanSlider.value, sCyanKeyframesButton, null);
        updateFilter('l_scale_cyan', lCyanSlider.value, lCyanKeyframesButton, null);
        updateFilter('h_shift_blue', hBlueSlider.value, hBlueKeyframesButton, null);
        updateFilter('s_scale_blue', sBlueSlider.value, sBlueKeyframesButton, null);
        updateFilter('l_scale_blue', lBlueSlider.value, lBlueKeyframesButton, null);
        updateFilter('h_shift_magenta', hMagentaSlider.value, hMagentaKeyframesButton, null);
        updateFilter('s_scale_magenta', sMagentaSlider.value, sMagentaKeyframesButton, null);
        updateFilter('l_scale_magenta', lMagentaSlider.value, lMagentaKeyframesButton, null);
    }

    keyframableParameters: preset.parameters
    startValues: [0, 0, 100, 100, 0, 100, 100, 0, 100, 100, 0, 100, 100, 0, 100, 100, 0, 100, 100]
    middleValues: [0, 0, 100, 100, 0, 100, 100, 0, 100, 100, 0, 100, 100, 0, 100, 100, 0, 100, 100]
    endValues: [0, 0, 100, 100, 0, 100, 100, 0, 100, 100, 0, 100, 100, 0, 100, 100, 0, 100, 100]
    width: 350
    height: 580
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('overlap', 0);
            filter.set('h_shift_red', 0);
            filter.set('s_scale_red', 100);
            filter.set('l_scale_red', 100);
            filter.set('h_shift_yellow', 0);
            filter.set('s_scale_yellow', 100);
            filter.set('l_scale_yellow', 100);
            filter.set('h_shift_green', 0);
            filter.set('s_scale_green', 100);
            filter.set('l_scale_green', 100);
            filter.set('h_shift_cyan', 0);
            filter.set('s_scale_cyan', 100);
            filter.set('l_scale_cyan', 100);
            filter.set('h_shift_blue', 0);
            filter.set('s_scale_blue', 100);
            filter.set('l_scale_blue', 100);
            filter.set('h_shift_magenta', 0);
            filter.set('s_scale_magenta', 100);
            filter.set('l_scale_magenta', 100);
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

            parameters: ['overlap', 'h_shift_red', 's_scale_red', 'l_scale_red', 'h_shift_yellow', 's_scale_yellow', 'l_scale_yellow', 'h_shift_green', 's_scale_green', 'l_scale_green', 'h_shift_cyan', 's_scale_cyan', 'l_scale_cyan', 'h_shift_blue', 's_scale_blue', 'l_scale_blue', 'h_shift_magenta', 's_scale_magenta', 'l_scale_magenta']
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
            text: qsTr('Overlap')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: overlapSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('overlap', value, overlapKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: overlapSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: overlapKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'overlap', overlapSlider.value);
            }
        }

        // HUE
        Label {
            text: qsTr('Hue Shift')
            Layout.columnSpan: parent.columns
            Layout.alignment: Qt.AlignCenter
        }

        Label {
            text: qsTr('Reds')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hRedSlider

            minimumValue: -180
            maximumValue: 180
            suffix: ' deg'
            onValueChanged: updateFilter('h_shift_red', value, hRedKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hRedSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: hRedKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'h_shift_red', hRedSlider.value);
            }
        }

        Label {
            text: qsTr('Yellows')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hYellowSlider

            minimumValue: -180
            maximumValue: 180
            suffix: ' deg'
            onValueChanged: updateFilter('h_shift_yellow', value, hYellowKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hYellowSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: hYellowKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'h_shift_yellow', hYellowSlider.value);
            }
        }

        Label {
            text: qsTr('Greens')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hGreenSlider

            minimumValue: -180
            maximumValue: 180
            suffix: ' deg'
            onValueChanged: updateFilter('h_shift_green', value, hGreenKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hGreenSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: hGreenKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'h_shift_green', hGreenSlider.value);
            }
        }

        Label {
            text: qsTr('Cyans')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hCyanSlider

            minimumValue: -180
            maximumValue: 180
            suffix: ' deg'
            onValueChanged: updateFilter('h_shift_cyan', value, hCyanKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hCyanSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: hCyanKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'h_shift_cyan', hCyanSlider.value);
            }
        }

        Label {
            text: qsTr('Blues')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hBlueSlider

            minimumValue: -180
            maximumValue: 180
            suffix: ' deg'
            onValueChanged: updateFilter('h_shift_blue', value, hBlueKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hBlueSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: hBlueKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'h_shift_blue', hBlueSlider.value);
            }
        }

        Label {
            text: qsTr('Magentas')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hMagentaSlider

            minimumValue: -180
            maximumValue: 180
            suffix: ' deg'
            onValueChanged: updateFilter('h_shift_magenta', value, hMagentaKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hMagentaSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: hMagentaKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'h_shift_magenta', hMagentaSlider.value);
            }
        }

        // Saturation
        Label {
            text: qsTr('Saturation Scale')
            Layout.columnSpan: parent.columns
            Layout.alignment: Qt.AlignCenter
        }

        Label {
            text: qsTr('Reds')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sRedSlider

            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: updateFilter('s_scale_red', value, sRedKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sRedSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: sRedKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 's_scale_red', sRedSlider.value);
            }
        }

        Label {
            text: qsTr('Yellows')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sYellowSlider

            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: updateFilter('s_scale_yellow', value, sYellowKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sYellowSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: sYellowKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 's_scale_yellow', sYellowSlider.value);
            }
        }

        Label {
            text: qsTr('Greens')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sGreenSlider

            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: updateFilter('s_scale_green', value, sGreenKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sGreenSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: sGreenKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 's_scale_green', sGreenSlider.value);
            }
        }

        Label {
            text: qsTr('Cyans')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sCyanSlider

            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: updateFilter('s_scale_cyan', value, sCyanKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sCyanSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: sCyanKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 's_scale_cyan', sCyanSlider.value);
            }
        }

        Label {
            text: qsTr('Blues')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sBlueSlider

            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: updateFilter('s_scale_blue', value, sBlueKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sBlueSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: sBlueKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 's_scale_blue', sBlueSlider.value);
            }
        }

        Label {
            text: qsTr('Magentas')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: sMagentaSlider

            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: updateFilter('s_scale_magenta', value, sMagentaKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sMagentaSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: sMagentaKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 's_scale_magenta', sMagentaSlider.value);
            }
        }

        // Lightness
        Label {
            text: qsTr('Lightness Scale')
            Layout.columnSpan: parent.columns
            Layout.alignment: Qt.AlignCenter
        }

        Label {
            text: qsTr('Reds')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lRedSlider

            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: updateFilter('l_scale_red', value, lRedKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: lRedSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: lRedKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'l_scale_red', lRedSlider.value);
            }
        }

        Label {
            text: qsTr('Yellows')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lYellowSlider

            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: updateFilter('l_scale_yellow', value, lYellowKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: lYellowSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: lYellowKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'l_scale_yellow', lYellowSlider.value);
            }
        }

        Label {
            text: qsTr('Greens')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lGreenSlider

            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: updateFilter('l_scale_green', value, lGreenKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: lGreenSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: lGreenKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'l_scale_green', lGreenSlider.value);
            }
        }

        Label {
            text: qsTr('Cyans')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lCyanSlider

            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: updateFilter('l_scale_cyan', value, lCyanKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: lCyanSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: lCyanKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'l_scale_cyan', lCyanSlider.value);
            }
        }

        Label {
            text: qsTr('Blues')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lBlueSlider

            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: updateFilter('l_scale_blue', value, lBlueKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: lBlueSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: lBlueKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'l_scale_blue', lBlueSlider.value);
            }
        }

        Label {
            text: qsTr('Magentas')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: lMagentaSlider

            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: updateFilter('l_scale_magenta', value, lMagentaKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: lMagentaSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: lMagentaKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'l_scale_magenta', lMagentaSlider.value);
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
