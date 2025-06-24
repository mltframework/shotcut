/*
 * Copyright (c) 2013-2025 Meltytech, LLC
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

Item {
    property bool blockUpdate: true
    property double startValue: 0.5
    property double middleValue: 0.5
    property double endValue: 0.5

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setLabels() {
        switch (operationCombo.currentIndex) {
        case 0:
            label.text = qsTr('Left');
            slider.label = qsTr('Right');
            break;
        case 1:
            label.text = qsTr('Left');
            slider.label = qsTr('Right');
            break;
        case 2:
            label.text = qsTr('Left');
            slider.label = qsTr('Right');
            break;
        case 3:
            label.text = qsTr('Front');
            slider.label = qsTr('Surround');
            break;
        case 4:
            label.text = qsTr('Front');
            slider.label = qsTr('Surround');
            break;
        case 5:
            label.text = qsTr('Front');
            slider.label = qsTr('Surround');
            break;
        }
    }

    function setControls() {
        let gang = filter.get('gang') === '1';
        switch (parseInt(filter.get('channel'))) {
        case -1:
            operationCombo.currentIndex = gang ? 2 : 0;
            break;
        case -2:
            operationCombo.currentIndex = 1;
            break;
        case -3:
            operationCombo.currentIndex = gang ? 5 : 3;
            break;
        case -4:
            operationCombo.currentIndex = 4;
            break;
        default:
            break;
        }
        setLabels();
        let position = getPosition();
        blockUpdate = true;
        slider.value = filter.getDouble('split', position) * slider.maximumValue;
        blockUpdate = false;
        slider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
        keyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('split') > 0;
    }

    function updateFilter(position) {
        if (blockUpdate)
            return;
        let value = slider.value / slider.maximumValue;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value;
            else
                middleValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('split');
            keyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('split', startValue, 0);
                filter.set('split', middleValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('split', middleValue, filter.duration - filter.animateOut);
                filter.set('split', endValue, filter.duration - 1);
            }
        } else if (!keyframesButton.checked) {
            filter.resetProperty('split');
            filter.set('split', middleValue);
        } else if (position !== null) {
            filter.set('split', value, position);
        }
    }

    width: 350
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('start', 0.5);
            filter.set('split', 0.5);
            filter.savePreset(preset.parameters);
        } else {
            // Convert old version of filter.
            if (filter.getDouble('start') !== 0.5)
                filter.set('split', filter.getDouble('start'));
            middleValue = filter.getDouble('split', filter.animateIn);
            if (filter.animateIn > 0)
                startValue = filter.getDouble('split', 0);
            if (filter.animateOut > 0)
                endValue = filter.getDouble('split', filter.duration - 1);
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
            id: preset

            Layout.columnSpan: parent.columns - 1
            parameters: ['split', 'channel', 'gang']
            onBeforePresetLoaded: {
                filter.resetProperty(parameters[0]);
            }
            onPresetSelected: {
                setControls();
                middleValue = filter.getDouble(parameters[0], filter.animateIn);
                if (filter.animateIn > 0)
                    startValue = filter.getDouble(parameters[0], 0);
                if (filter.animateOut > 0)
                    endValue = filter.getDouble(parameters[0], filter.duration - 1);
            }
        }

        Label {
            text: qsTr('Operation')
            Layout.alignment: Qt.AlignRight
            visible: application.audioChannels() > 3
        }

        Shotcut.ComboBox {
            id: operationCombo

            visible: application.audioChannels() > 3
            Layout.columnSpan: 2
            model: [qsTr('Front Balance'), qsTr('Surround Balance'), qsTr('Front + Surround Balance'), qsTr('Left Fade'), qsTr('Right Fade'), qsTr('Left + Right Fade')]
            onActivated: {
                switch (currentIndex) {
                case 0:
                    filter.set('channel', -1);
                    filter.set('gang', 0);
                    break;
                case 1:
                    filter.set('channel', -2);
                    filter.set('gang', 0);
                    break;
                case 2:
                    filter.set('channel', -1);
                    filter.set('gang', 1);
                    break;
                case 3:
                    filter.set('channel', -3);
                    filter.set('gang', 0);
                    break;
                case 4:
                    filter.set('channel', -4);
                    filter.set('gang', 0);
                    break;
                case 5:
                    filter.set('channel', -3);
                    filter.set('gang', 1);
                    break;
                }
                setLabels();
            }
        }

        Shotcut.UndoButton {
            visible: application.audioChannels() > 3
            onClicked: operationCombo.currentIndex = 0
        }

        Label {
            id: label

            text: qsTr('Left')
        }

        Shotcut.SliderSpinner {
            id: slider

            minimumValue: 0
            maximumValue: 1000
            ratio: 1000
            decimals: 2
            label: qsTr('Right')
            onValueChanged: updateFilter(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: slider.value = 500
        }

        Shotcut.KeyframesButton {
            id: keyframesButton

            onToggled: {
                var value = slider.value / slider.maximumValue;
                if (checked) {
                    blockUpdate = true;
                    filter.clearSimpleAnimation('split');
                    blockUpdate = false;
                    filter.set('split', value, getPosition());
                } else {
                    filter.resetProperty('split');
                    filter.set('split', value);
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
}
