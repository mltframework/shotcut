/*
 * Copyright (c) 2016-2021 Meltytech, LLC
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

Item {
    property var defaultParameters: ['gamma_r', 'gamma_g', 'gamma_b', 'gain_r', 'gain_g', 'gain_b']
    property double gammaFactor: 2
    property double gainFactor: 2
    property bool blockUpdate: true
    property double startValue: 0.5
    property double middleValue: 0.5
    property double endValue: 0.5

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        contrastSlider.value = filter.getDouble("gain_r", position) / gainFactor * 100;
        keyframesButton.checked = filter.keyframeCount('gain_r') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
        contrastSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilter(position) {
        if (blockUpdate)
            return;
        var value = contrastSlider.value / 100;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value;
            else
                middleValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('gamma_r');
            filter.resetProperty('gamma_g');
            filter.resetProperty('gamma_b');
            filter.resetProperty('gain_r');
            filter.resetProperty('gain_g');
            filter.resetProperty('gain_b');
            keyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("gamma_r", (1 - startValue) * gammaFactor, 0);
                filter.set("gamma_g", (1 - startValue) * gammaFactor, 0);
                filter.set("gamma_b", (1 - startValue) * gammaFactor, 0);
                filter.set("gain_r", startValue * gainFactor, 0);
                filter.set("gain_g", startValue * gainFactor, 0);
                filter.set("gain_b", startValue * gainFactor, 0);
                filter.set("gamma_r", (1 - middleValue) * gammaFactor, filter.animateIn - 1);
                filter.set("gamma_g", (1 - middleValue) * gammaFactor, filter.animateIn - 1);
                filter.set("gamma_b", (1 - middleValue) * gammaFactor, filter.animateIn - 1);
                filter.set("gain_r", middleValue * gainFactor, filter.animateIn - 1);
                filter.set("gain_g", middleValue * gainFactor, filter.animateIn - 1);
                filter.set("gain_b", middleValue * gainFactor, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("gamma_r", (1 - middleValue) * gammaFactor, filter.duration - filter.animateOut);
                filter.set("gamma_g", (1 - middleValue) * gammaFactor, filter.duration - filter.animateOut);
                filter.set("gamma_b", (1 - middleValue) * gammaFactor, filter.duration - filter.animateOut);
                filter.set("gain_r", middleValue * gainFactor, filter.duration - filter.animateOut);
                filter.set("gain_g", middleValue * gainFactor, filter.duration - filter.animateOut);
                filter.set("gain_b", middleValue * gainFactor, filter.duration - filter.animateOut);
                filter.set("gamma_r", (1 - endValue) * gammaFactor, filter.duration - 1);
                filter.set("gamma_g", (1 - endValue) * gammaFactor, filter.duration - 1);
                filter.set("gamma_b", (1 - endValue) * gammaFactor, filter.duration - 1);
                filter.set("gain_r", endValue * gainFactor, filter.duration - 1);
                filter.set("gain_g", endValue * gainFactor, filter.duration - 1);
                filter.set("gain_b", endValue * gainFactor, filter.duration - 1);
            }
        } else if (!keyframesButton.checked) {
            filter.resetProperty('gamma_r');
            filter.resetProperty('gamma_g');
            filter.resetProperty('gamma_b');
            filter.resetProperty('gain_r');
            filter.resetProperty('gain_g');
            filter.resetProperty('gain_b');
            filter.set("gamma_r", (1 - middleValue) * gammaFactor);
            filter.set("gamma_g", (1 - middleValue) * gammaFactor);
            filter.set("gamma_b", (1 - middleValue) * gammaFactor);
            filter.set("gain_r", middleValue * gainFactor);
            filter.set("gain_g", middleValue * gainFactor);
            filter.set("gain_b", middleValue * gainFactor);
        } else if (position !== null) {
            filter.set("gamma_r", (1 - value) * gammaFactor, position);
            filter.set("gamma_g", (1 - value) * gammaFactor, position);
            filter.set("gamma_b", (1 - value) * gammaFactor, position);
            filter.set("gain_r", value * gainFactor, position);
            filter.set("gain_g", value * gainFactor, position);
            filter.set("gain_b", value * gainFactor, position);
        }
    }

    width: 200
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set("gamma_r", 1);
            filter.set("gamma_g", 1);
            filter.set("gamma_b", 1);
            filter.set("gain_r", 1);
            filter.set("gain_g", 1);
            filter.set("gain_b", 1);
            filter.savePreset(defaultParameters);
        } else {
            middleValue = filter.getDouble('gain_r', filter.animateIn) / gainFactor;
            if (filter.animateIn > 0)
                startValue = filter.getDouble('gain_r', 0) / gainFactor;
            if (filter.animateOut > 0)
                endValue = filter.getDouble('gain_r', filter.duration - 1) / gainFactor;
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
            Layout.columnSpan: 3
            parameters: defaultParameters
            onBeforePresetLoaded: {
                filter.resetProperty('gamma_r');
                filter.resetProperty('gamma_g');
                filter.resetProperty('gamma_b');
                filter.resetProperty('gain_r');
                filter.resetProperty('gain_g');
                filter.resetProperty('gain_b');
            }
            onPresetSelected: {
                setControls();
                updateFilter(getPosition());
            }
        }

        Label {
            text: qsTr('Level')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: contrastSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: contrastSlider.value = 50
        }

        Shotcut.KeyframesButton {
            id: keyframesButton

            onToggled: {
                var value = contrastSlider.value / 100;
                blockUpdate = true;
                filter.resetProperty('gamma_r');
                filter.resetProperty('gamma_g');
                filter.resetProperty('gamma_b');
                filter.resetProperty('gain_r');
                filter.resetProperty('gain_g');
                filter.resetProperty('gain_b');
                if (checked) {
                    filter.animateIn = filter.animateOut = 0;
                    blockUpdate = false;
                    var position = getPosition();
                    filter.set("gamma_r", (1 - value) * gammaFactor, position);
                    filter.set("gamma_g", (1 - value) * gammaFactor, position);
                    filter.set("gamma_b", (1 - value) * gammaFactor, position);
                    filter.set("gain_r", value * gainFactor, position);
                    filter.set("gain_g", value * gainFactor, position);
                    filter.set("gain_b", value * gainFactor, position);
                } else {
                    blockUpdate = false;
                    filter.set("gamma_r", (1 - value) * gammaFactor);
                    filter.set("gamma_g", (1 - value) * gammaFactor);
                    filter.set("gamma_b", (1 - value) * gammaFactor);
                    filter.set("gain_r", value * gainFactor);
                    filter.set("gain_g", value * gainFactor);
                    filter.set("gain_b", value * gainFactor);
                }
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
            updateFilter(null);
        }

        function onOutChanged() {
            updateFilter(null);
        }

        function onAnimateInChanged() {
            updateFilter(null);
        }

        function onAnimateOutChanged() {
            updateFilter(null);
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

    Connections {
        function onKeyframeAdded(parameter, position) {
            middleValue = filter.getDouble(parameter, position) / gainFactor;
            filter.set("gain_r", middleValue * gainFactor, position);
            filter.set("gain_g", middleValue * gainFactor, position);
            filter.set("gain_b", middleValue * gainFactor, position);
            var gamma = (1 - middleValue) * gammaFactor;
            filter.set("gamma_r", gamma, position);
            filter.set("gamma_g", gamma, position);
            filter.set("gamma_b", gamma, position);
        }

        target: parameters
    }
}
