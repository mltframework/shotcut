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
    property var defaultParameters: ['radius', 'smooth', 'opacity', 'mode']
    property bool blockUpdate: true
    property var startValues: [1, 0, 0]
    property var middleValues: [0.5, 2, 0]
    property var endValues: [1, 0, 0]

    function initSimpleAnimation() {
        middleValues = [filter.getDouble(defaultParameters[0], filter.animateIn), filter.getDouble(defaultParameters[1], filter.animateIn), filter.getDouble(defaultParameters[2], filter.animateIn)];
        if (filter.animateIn > 0)
            startValues = [filter.getDouble(defaultParameters[0], 0), filter.getDouble(defaultParameters[1], 0), filter.getDouble(defaultParameters[2], 0)];
        if (filter.animateOut > 0)
            endValues = [filter.getDouble(defaultParameters[0], filter.duration - 1), filter.getDouble(defaultParameters[1], filter.duration - 1), filter.getDouble(defaultParameters[2], filter.duration - 1)];
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setKeyframedControls() {
        var position = getPosition();
        blockUpdate = true;
        radiusSlider.value = filter.getDouble('radius', position) * 100;
        radiusKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('radius') > 0;
        smoothSlider.value = filter.getDouble('smooth', position) * 100;
        smoothKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('smooth') > 0;
        opacitySlider.value = (1 - filter.getDouble('opacity', position)) * 100;
        opacityKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('opacity') > 0;
        blockUpdate = false;
        radiusSlider.enabled = smoothSlider.enabled = opacitySlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function setControls() {
        modeCheckBox.checked = filter.get('mode') === '1';
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
            radiusSlider.enabled = smoothSlider.enabled = opacitySlider.enabled = true;
            blockUpdate = true;
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                for (var i = 0; i < 3; i++)
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
        setKeyframedControls();
        updateFilter('radius', radiusSlider.value / 100, null, radiusKeyframesButton);
        updateFilter('smooth', smoothSlider.value / 100, null, smoothKeyframesButton);
        updateFilter('opacity', 1 - opacitySlider.value / 100, null, opacityKeyframesButton);
    }

    width: 350
    height: 150
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            for (var i = 0; i < 3; i++)
                filter.set(defaultParameters[i], middleValues[i]);
            filter.set('mode', 1);
            filter.savePreset(defaultParameters);
        } else {
            initSimpleAnimation();
        }
        setControls();
        setKeyframedControls();
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
            id: preset

            Layout.columnSpan: 3
            parameters: defaultParameters
            onBeforePresetLoaded: {
                for (var i = 0; i < 3; i++)
                    filter.resetProperty(defaultParameters[i]);
            }
            onPresetSelected: {
                setControls();
                setKeyframedControls();
                initSimpleAnimation();
            }
        }

        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: radiusSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('radius', value / 100, getPosition(), radiusKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: radiusSlider.value = 50
        }

        Shotcut.KeyframesButton {
            id: radiusKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, 'radius', radiusSlider.value / 100)
        }

        Label {
            text: qsTr('Feathering')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: smoothSlider

            minimumValue: 0
            maximumValue: 500
            suffix: ' %'
            onValueChanged: updateFilter('smooth', value / 100, getPosition(), smoothKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: smoothSlider.value = 200
        }

        Shotcut.KeyframesButton {
            id: smoothKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, 'smooth', smoothSlider.value / 100)
        }

        Label {
        }

        CheckBox {
            id: modeCheckBox

            property bool isReady: false

            text: qsTr('Non-linear feathering')
            Layout.columnSpan: 3
            Component.onCompleted: isReady = true
            onClicked: {
                if (isReady)
                    filter.set('mode', checked);
            }
        }

        Label {
            text: qsTr('Opacity')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: opacitySlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter('opacity', 1 - value / 100, getPosition(), opacityKeyframesButton)
        }

        Shotcut.UndoButton {
            onClicked: opacitySlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: opacityKeyframesButton

            onToggled: onKeyframesButtonClicked(checked, 'opacity', 1 - opacitySlider.value / 100)
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        function onChanged() {
            setKeyframedControls();
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
            setKeyframedControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setKeyframedControls();
        }

        target: producer
    }
}
