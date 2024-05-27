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
    property double intensityDefault: 0
    property double redBalanceDefault: 1
    property double greenBalanceDefault: 1
    property double blueBalanceDefault: 1

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        intensitySlider.value = filter.getDouble('av.intensity', position) * 50;
        intensityKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('av.intensity') > 0;
        redBalanceSlider.value = filter.getDouble('av.rbal', position) * 10;
        redBalanceKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('av.rbal') > 0;
        greenBalanceSlider.value = filter.getDouble('av.gbal', position) * 10;
        greenBalanceKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('av.gbal') > 0;
        blueBalanceSlider.value = filter.getDouble('av.bbal', position) * 10;
        blueBalanceKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('av.bbal') > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        intensitySlider.enabled = enabled;
        redBalanceSlider.enabled = enabled;
        greenBalanceSlider.enabled = enabled;
        blueBalanceSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setControls();
        updateFilter('av.intensity', intensitySlider.value / 50, intensityKeyframesButton, null);
        updateFilter('av.rbal', redBalanceSlider.value / 10, redBalanceKeyframesButton, null);
        updateFilter('av.gbal', greenBalanceSlider.value / 10, greenBalanceKeyframesButton, null);
        updateFilter('av.bbal', blueBalanceSlider.value / 10, blueBalanceKeyframesButton, null);
    }

    keyframableParameters: ['av.intensity', 'av.rbal', 'av.gbal', 'av.bbal']
    startValues: [intensityDefault, redBalanceDefault, greenBalanceDefault, blueBalanceDefault]
    middleValues: [intensityDefault, redBalanceDefault, greenBalanceDefault, blueBalanceDefault]
    endValues: [intensityDefault, redBalanceDefault, greenBalanceDefault, blueBalanceDefault]
    width: 200
    height: 150
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('av.intensity', intensityDefault);
            filter.set('av.rbal', redBalanceDefault);
            filter.set('av.gbal', greenBalanceDefault);
            filter.set('av.bbal', blueBalanceDefault);
            filter.savePreset(keyframableParameters);
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
            parameters: keyframableParameters
            onBeforePresetLoaded: resetSimpleKeyframes()
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        Label {
            text: qsTr('Intensity')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: intensitySlider

            minimumValue: -100
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('av.intensity', value / 50, intensityKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: intensitySlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: intensityKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'av.intensity', intensitySlider.value / 50);
            }
        }

        Label {
            text: qsTr('Red')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: redBalanceSlider

            minimumValue: -100
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('av.rbal', value / 10, redBalanceKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: redBalanceSlider.value = 10
        }

        Shotcut.KeyframesButton {
            id: redBalanceKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'av.rbal', redBalanceSlider.value / 10);
            }
        }

        Label {
            text: qsTr('Green')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: greenBalanceSlider

            minimumValue: -100
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('av.gbal', value / 10, greenBalanceKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: greenBalanceSlider.value = 10
        }

        Shotcut.KeyframesButton {
            id: greenBalanceKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'av.gbal', greenBalanceSlider.value / 10);
            }
        }

        Label {
            text: qsTr('Blue')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: blueBalanceSlider

            minimumValue: -100
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('av.bbal', value / 10, blueBalanceKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: blueBalanceSlider.value = 10
        }

        Shotcut.KeyframesButton {
            id: blueBalanceKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'av.bbal', blueBalanceSlider.value / 10);
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
