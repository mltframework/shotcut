/*
 * Copyright (c) 2022 Meltytech, LLC
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
    property string amountH: 'av.sigma'
    property string amountV: 'av.sigmaV'
    property double amountSliderMin: 0
    property double amountSliderMax: 100
    property double amountSliderDefault: 4

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        amountSlider.value = filter.getDouble(amountH, position);
        amountKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(amountH) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        amountSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        updateFilter(amountH, amountSlider.value, amountKeyframesButton, null);
        updateFilter(amountV, amountSlider.value, amountKeyframesButton, null);
    }

    keyframableParameters: [amountH, amountV]
    startValues: [0, 0]
    middleValues: [amountSliderDefault, amountSliderDefault]
    endValues: [0, 0]
    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(amountH, amountSliderDefault);
            filter.set(amountV, amountSliderDefault);
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
            text: qsTr('Amount')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: amountSlider

            minimumValue: amountSliderMin
            maximumValue: amountSliderMax
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: {
                updateFilter(amountH, amountSlider.value, amountKeyframesButton, getPosition());
                updateFilter(amountV, amountSlider.value, amountKeyframesButton, getPosition());
            }
        }

        Shotcut.UndoButton {
            onClicked: amountSlider.value = amountSliderDefault
        }

        Shotcut.KeyframesButton {
            id: amountKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, amountH, amountSlider.value);
                toggleKeyframes(checked, amountV, amountSlider.value);
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
