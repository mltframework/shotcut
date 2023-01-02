/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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
    property var defaultParameters: ['circle_radius', 'gaussian_radius', 'correlation', 'noise']
    property bool blockUpdate: true
    property var startValues: [0, 0, 0, 0]
    property var middleValues: [2, 0, 0.95, 0.01]
    property var endValues: [0, 0, 0, 0]

    function initSimpleAnimation() {
        middleValues = [filter.getDouble(defaultParameters[0], filter.animateIn), filter.getDouble(defaultParameters[1], filter.animateIn), filter.getDouble(defaultParameters[2], filter.animateIn), filter.getDouble(defaultParameters[3], filter.animateIn)];
        if (filter.animateIn > 0)
            startValues = [filter.getDouble(defaultParameters[0], 0), filter.getDouble(defaultParameters[1], 0), filter.getDouble(defaultParameters[2], 0), filter.getDouble(defaultParameters[3], 0)];
        if (filter.animateOut > 0)
            endValues = [filter.getDouble(defaultParameters[0], filter.duration - 1), filter.getDouble(defaultParameters[1], filter.duration - 1), filter.getDouble(defaultParameters[2], filter.duration - 1), filter.getDouble(defaultParameters[3], filter.duration - 1)];
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        circleSlider.value = filter.getDouble("circle_radius", position);
        circleKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('circle_radius') > 0;
        gaussianSlider.value = filter.getDouble("gaussian_radius", position);
        gaussianKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('gaussian_radius') > 0;
        correlationSlider.value = filter.getDouble("correlation", position);
        correlationKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('correlation') > 0;
        noiseSlider.value = filter.getDouble("noise", position);
        noiseKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('noise') > 0;
        blockUpdate = false;
        circleSlider.enabled = gaussianSlider.enabled = correlationSlider.enabled = noiseSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilter(parameter, value, position, button) {
        if (blockUpdate)
            return;
        var index = defaultParameters.indexOf(parameter);
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValues[index] = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValues[index] = value;
            else
                middleValues[index] = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(parameter);
            button.checked = false;
            if (filter.animateIn > 0) {
                filter.set(parameter, startValues[index], 0);
                filter.set(parameter, middleValues[index], filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set(parameter, middleValues[index], filter.duration - filter.animateOut);
                filter.set(parameter, endValues[index], filter.duration - 1);
            }
        } else if (!button.checked) {
            filter.resetProperty(parameter);
            filter.set(parameter, middleValues[index]);
        } else if (position !== null) {
            filter.set(parameter, value, position);
        }
    }

    function onKeyframesButtonClicked(checked, parameter, value) {
        if (checked) {
            blockUpdate = true;
            circleSlider.enabled = gaussianSlider.enabled = correlationSlider.enabled = noiseSlider.enabled = true;
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                for (var i = 0; i < defaultParameters.length; i++)
                    filter.resetProperty(defaultParameters[i]);
                filter.animateIn = filter.animateOut = 0;
            } else {
                filter.clearSimpleAnimation(parameter);
            }
            blockUpdate = false;
            filter.set(parameter, value, getPosition());
        } else {
            filter.resetProperty(parameter);
            filter.set(parameter, value);
        }
    }

    function updateSimpleAnimation() {
        setControls();
        updateFilter('circle_radius', circleSlider.value, null, circleKeyframesButton);
        updateFilter('gaussian_radius', gaussianSlider.value, null, gaussianKeyframesButton);
        updateFilter('correlation', correlationSlider.value, null, correlationKeyframesButton);
        updateFilter('noise', noiseSlider.value, null, noiseKeyframesButton);
    }

    width: 350
    height: 150
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('circle_radius', 2);
            filter.set('gaussian_radius', 0);
            filter.set('correlation', 0.95);
            filter.set('noise', 0.01);
            filter.savePreset(defaultParameters);
        } else {
            initSimpleAnimation();
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
                for (var i = 0; i < defaultParameters.length; i++)
                    filter.resetProperty(defaultParameters[i]);
            }
            onPresetSelected: {
                setControls();
                initSimpleAnimation();
            }
        }

        // Row 2
        Label {
            text: qsTr('Circle radius')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: circleSlider

            minimumValue: 0
            maximumValue: 99.99
            decimals: 2
            stepSize: 0.1
            onValueChanged: updateFilter('circle_radius', value, getPosition(), circleKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: circleSlider.value = 2
        }

        Shotcut.KeyframesButton {
            id: circleKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, 'circle_radius', circleSlider.value)
        }

        // Row 3
        Label {
            text: qsTr('Gaussian radius')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: gaussianSlider

            minimumValue: 0
            maximumValue: 99.99
            decimals: 2
            stepSize: 0.1
            onValueChanged: updateFilter('gaussian_radius', value, getPosition(), gaussianKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: gaussianSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: gaussianKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, 'gaussian_radius', gaussianSlider.value)
        }

        // Row 4
        Label {
            text: qsTr('Correlation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: correlationSlider

            minimumValue: 0
            maximumValue: 1
            decimals: 2
            onValueChanged: updateFilter('correlation', value, getPosition(), correlationKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: correlationSlider.value = 0.95
        }

        Shotcut.KeyframesButton {
            id: correlationKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, 'correlation', correlationSlider.value)
        }

        // Row 5
        Label {
            text: qsTr('Noise')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: noiseSlider

            minimumValue: 0
            maximumValue: 1
            decimals: 2
            onValueChanged: updateFilter('noise', value, getPosition(), noiseKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: noiseSlider.value = 0.01
        }

        Shotcut.KeyframesButton {
            id: noiseKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, 'noise', noiseSlider.value)
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
            updateSimpleAnimation();
        }

        function onOutChanged() {
            updateSimpleAnimation();
        }

        function onAnimateInChanged() {
            updateSimpleAnimation();
        }

        function onAnimateOutChanged() {
            updateSimpleAnimation();
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
