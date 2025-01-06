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
        hCenterSlider.value = filter.getDouble('hue_center', position);
        hCenterKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('hue_center') > 0;
        hRangeSlider.value = filter.getDouble('hue_range', position);
        hRangeKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('hue_range') > 0;
        blendSlider.value = filter.getDouble('blend', position);
        blendKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('blend') > 0;
        hShiftSlider.value = filter.getDouble('h_shift', position);
        hShiftKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('h_shift') > 0;
        sScaleSlider.value = filter.getDouble('s_scale', position);
        sScaleKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('s_scale') > 0;
        lScaleSlider.value = filter.getDouble('l_scale', position);
        lScaleKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('l_scale') > 0;
        // Set the gradient
        var hueStep = hRangeSlider.value / 5;
        var hue = hCenterSlider.value - hRangeSlider.value / 2;
        if (hue < 0)
            hue = hue + 360;
        stop0.color = Qt.hsla(hue / 360.0, 1.0, 0.5, 1.0);
        hue += hueStep;
        if (hue > 360)
            hue = hue - 360;
        stop1.color = Qt.hsla(hue / 360.0, 1.0, 0.5, 1.0);
        hue += hueStep;
        if (hue > 360)
            hue = hue - 360;
        stop2.color = Qt.hsla(hue / 360.0, 1.0, 0.5, 1.0);
        hue += hueStep;
        if (hue > 360)
            hue = hue - 360;
        stop3.color = Qt.hsla(hue / 360.0, 1.0, 0.5, 1.0);
        hue += hueStep;
        if (hue > 360)
            hue = hue - 360;
        stop4.color = Qt.hsla(hue / 360.0, 1.0, 0.5, 1.0);
        hue += hueStep;
        if (hue > 360)
            hue = hue - 360;
        stop5.color = Qt.hsla(hue / 360.0, 1.0, 0.5, 1.0);
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        blendSlider.enabled = hCenterSlider.enabled = hRangeSlider.enabled = enabled;
        hShiftSlider.enabled = sScaleSlider.enabled = lScaleSlider.value.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        updateFilter('hue_center', hCenterSlider.value, hCenterKeyframesButton, null);
        updateFilter('hue_range', hRangeSlider.value, hRangeKeyframesButton, null);
        updateFilter('blend', blendSlider.value, blendKeyframesButton, null);
        updateFilter('h_shift', hShiftSlider.value, hShiftKeyframesButton, null);
        updateFilter('s_scale', sScaleSlider.value, sScaleKeyframesButton, null);
        updateFilter('l_scale', lScaleSlider.value, lScaleKeyframesButton, null);
    }

    keyframableParameters: preset.parameters
    startValues: [180, 0, 60, 0, 100, 100]
    middleValues: [180, 0, 60, 0, 100, 100]
    endValues: [180, 0, 60, 0, 100, 100]
    width: 350
    height: 300
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('hue_center', 180);
            filter.set('hue_range', 0);
            filter.set('blend', 0);
            filter.set('h_shift', 0);
            filter.set('s_scale', 100);
            filter.set('l_scale', 100);
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

            parameters: ['hue_center', 'hue_range', 'blend', 'h_shift', 's_scale', 'l_scale']
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
            text: qsTr('Hue Center')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr('The center of the color range to be changed.')
            }
        }
        Shotcut.SliderSpinner {
            id: hCenterSlider

            minimumValue: 0
            maximumValue: 360
            suffix: ' deg'
            onValueChanged: updateFilter('hue_center', value, hCenterKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hCenterSlider.value = 180
        }

        Shotcut.KeyframesButton {
            id: hCenterKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'hue_center', hCenterSlider.value);
            }
        }

        Label {
            text: qsTr('Hue Range')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr('The width of the color range to be changed.')
            }
        }
        Shotcut.SliderSpinner {
            id: hRangeSlider

            minimumValue: 0
            maximumValue: 180
            suffix: ' deg'
            onValueChanged: updateFilter('hue_range', value, hRangeKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hRangeSlider.value = 60
        }

        Shotcut.KeyframesButton {
            id: hRangeKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'hue_range', hRangeSlider.value);
            }
        }

        Label {
            id: gradientLabel
            text: ' '
            Layout.alignment: Qt.AlignRight
        }

        Rectangle {
            id: gradientRect

            Layout.columnSpan: parent.columns - 2
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            height: gradientLabel.height
            border.color: "gray"
            border.width: 1
            radius: 4
            gradient: Gradient {
                id: gradientView
                orientation: Gradient.Horizontal
                GradientStop {
                    id: stop0
                    position: 0.0
                }
                GradientStop {
                    id: stop1
                    position: 0.2
                }
                GradientStop {
                    id: stop2
                    position: 0.4
                }
                GradientStop {
                    id: stop3
                    position: 0.6
                }
                GradientStop {
                    id: stop4
                    position: 0.8
                }
                GradientStop {
                    id: stop5
                    position: 1.0
                }
            }
        }

        Shotcut.Button {
            id: pickerButton

            icon.name: 'color-picker'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/color-picker.png'
            implicitWidth: 20
            implicitHeight: 20
            checkable: true

            Shotcut.ColorPickerItem {
                id: pickerItem

                onColorPicked: color => {
                    hCenterSlider.value = color.hslHue * 360.0;
                    pickerButton.checked = false;
                    filter.set('disable', 0);
                }
                onCancelled: {
                    pickerButton.checked = false;
                    filter.set('disable', 0);
                }
            }

            onClicked: {
                filter.set('disable', 1);
                pickerItem.pickColor();
            }

            Shotcut.HoverTip {
                text: '<p>' + qsTr("Pick the center hue from a color on the screen. By pressing the mouse button and then moving your mouse you can select a section of the screen from which to get an average color.") + '</p>'
            }
        }

        Label {
            text: qsTr('Blend')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr('The amount of blending to apply to the edges of the color range.')
            }
        }
        Shotcut.SliderSpinner {
            id: blendSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('blend', value, blendKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: blendSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: blendKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'blend', blendSlider.value);
            }
        }

        Label {
            text: qsTr('Hue Shift')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr('The amount to shift the Hue of the color range.')
            }
        }
        Shotcut.SliderSpinner {
            id: hShiftSlider

            minimumValue: -180
            maximumValue: 180
            suffix: ' deg'
            onValueChanged: updateFilter('h_shift', value, hShiftKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hShiftSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: hShiftKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'h_shift', hShiftSlider.value);
            }
        }

        Label {
            text: qsTr('Saturation Scale')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr('The amount to scale the saturation of the color range.')
            }
        }
        Shotcut.SliderSpinner {
            id: sScaleSlider

            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: updateFilter('s_scale', value, sScaleKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: sScaleSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: sScaleKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 's_scale', sScaleSlider.value);
            }
        }

        Label {
            text: qsTr('Lightness Scale')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr('The amount to scale the lightness of the color range.')
            }
        }
        Shotcut.SliderSpinner {
            id: lScaleSlider

            minimumValue: 0
            maximumValue: 200
            suffix: ' %'
            onValueChanged: updateFilter('l_scale', value, lScaleKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: lScaleSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: lScaleKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'l_scale', lScaleSlider.value);
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
