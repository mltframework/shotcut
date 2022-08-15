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
    property string paramAmount: '0'
    property string paramSize: '1'
    property var defaultParameters: [paramAmount, paramSize]
    property bool blockUpdate: true
    property var startValues: [0.3, 0.5]
    property var middleValues: [0.5, 0.5]
    property var endValues: [0.3, 0.5]

    function initSimpleAnimation() {
        middleValues = [filter.getDouble(defaultParameters[0], filter.animateIn), filter.getDouble(defaultParameters[1], filter.animateIn)];
        if (filter.animateIn > 0)
            startValues = [filter.getDouble(defaultParameters[0], 0), filter.getDouble(defaultParameters[1], 0)];

        if (filter.animateOut > 0)
            endValues = [filter.getDouble(defaultParameters[0], filter.duration - 1), filter.getDouble(defaultParameters[1], filter.duration - 1)];

    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        amountSlider.value = filter.getDouble(paramAmount, position) * 100;
        amountKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(paramAmount) > 0;
        sizeSlider.value = filter.getDouble(paramSize, position) * 100;
        sizeKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(paramSize) > 0;
        blockUpdate = false;
        amountSlider.enabled = sizeSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilter(parameter, value, position, button) {
        if (blockUpdate)
            return ;

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
            amountSlider.enabled = sizeSlider.enabled = true;
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                filter.resetProperty(defaultParameters[0]);
                filter.resetProperty(defaultParameters[1]);
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
        updateFilter(paramAmount, amountSlider.value / 100, null, amountKeyframesButton);
        updateFilter(paramSize, sizeSlider.value / 100, null, sizeKeyframesButton);
    }

    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set(paramAmount, 0.5);
            filter.set(paramSize, 0.5);
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
                filter.resetProperty(paramAmount);
                filter.resetProperty(paramSize);
            }
            onPresetSelected: {
                setControls();
                initSimpleAnimation();
            }
        }

        Label {
            text: qsTr('Amount')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: amountSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            decimals: 1
            onValueChanged: updateFilter(paramAmount, value / 100, getPosition(), amountKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: amountSlider.value = 50
        }

        Shotcut.KeyframesButton {
            id: amountKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, paramAmount, amountSlider.value / 100)
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sizeSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            decimals: 1
            onValueChanged: updateFilter(paramSize, value / 100, getPosition(), sizeKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: sizeSlider.value = 50
        }

        Shotcut.KeyframesButton {
            id: sizeKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, paramSize, sizeSlider.value / 100)
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
