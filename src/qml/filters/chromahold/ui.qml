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
    property string colorParam: 'av.color'
    property string colorDefault: '0x000000'
    property string distanceParam: 'av.similarity'
    property double distanceDefault: 10
    property var defaultParameters: [colorParam, distanceParam]

    function setControls() {
        colorPicker.value = filter.get(colorParam);
        blockUpdate = true;
        distanceSlider.value = filter.getDouble(distanceParam, getPosition()) * 100;
        distanceKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(distanceParam) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        distanceSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        updateFilter(distanceParam, distanceSlider.value / 100, distanceKeyframesButton, null);
    }

    keyframableParameters: [distanceParam]
    startValues: [1]
    middleValues: [distanceDefault / 100]
    endValues: [1]
    width: 350
    height: 50
    Component.onCompleted: {
        presetItem.parameters = defaultParameters;
        if (filter.isNew) {
            // Set default parameter values
            filter.set(colorParam, colorDefault);
            filter.set(distanceParam, distanceDefault / 100);
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
            onBeforePresetLoaded: resetSimpleKeyframes()
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

            onValueChanged: {
                filter.set(colorParam, value);
                filter.set('disable', 0);
            }
            onPickStarted: filter.set('disable', 1)
            onPickCancelled: filter.set('disable', 0)
        }

        Shotcut.UndoButton {
            onClicked: colorPicker.value = colorDefault
        }

        Item {
            Layout.fillWidth: true
        }

        // Row 2
        Label {
            text: qsTr('Distance')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: distanceSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(distanceParam, value / 100, distanceKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: distanceSlider.value = distanceDefault
        }

        Shotcut.KeyframesButton {
            id: distanceKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, distanceParam, distanceSlider.value / 100);
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
