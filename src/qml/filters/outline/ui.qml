/*
 * Copyright (c) 2025 Meltytech, LLC
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
    property string colorParam: 'color'
    property string colorDefault: Qt.rgba(0, 0, 0, 1)
    property string thicknessParam: 'thickness'
    property double thicknessDefault: 4
    property var defaultParameters: [colorParam, thicknessParam]

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        colorPicker.value = filter.getColor(colorParam);
        colorKeyframesButton.checked = filter.keyframeCount(colorParam) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        thicknessSlider.value = filter.getDouble(thicknessParam, position);
        thicknessKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(thicknessParam) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        colorPicker.enabled = enabled;
        thicknessSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setControls();
        updateFilter(thicknessParam, thicknessSlider.value, thicknessKeyframesButton, null);
    }

    keyframableParameters: [colorParam, thicknessParam]
    startValues: [colorDefault, 0]
    middleValues: [colorDefault, thicknessDefault]
    endValues: [colorDefault, 0]
    width: 350
    height: 100
    Component.onCompleted: {
        presetItem.parameters = defaultParameters;
        if (filter.isNew) {
            // Set default parameter values
            filter.set(colorParam, colorDefault);
            filter.set(thicknessParam, thicknessDefault);
            filter.savePreset(defaultParameters);
        }
        setControls();
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: presetItem

            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        // Row 1
        Label {
            text: qsTr('Color')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ColorPicker {
            id: colorPicker

            property bool isReady: false

            alpha: true
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    updateFilter('color', Qt.color(value), colorKeyframesButton, getPosition());
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: colorPicker.value = colorDefault
        }

        Shotcut.KeyframesButton {
            id: colorKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, colorParam, Qt.color(colorPicker.value));
            }
        }

        // Row 2
        Label {
            text: qsTr('Thickness')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: thicknessSlider

            minimumValue: 0
            maximumValue: 20
            decimals: 1
            suffix: ' px'
            value: filter.getDouble(thicknessParam)
            onValueChanged: updateFilter(thicknessParam, value, thicknessKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: thicknessSlider.value = thicknessDefault
        }

        Shotcut.KeyframesButton {
            id: thicknessKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, thicknessParam, thicknessSlider.value);
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
