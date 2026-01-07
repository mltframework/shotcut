/*
 * Copyright (c) 2016-2024 Meltytech, LLC
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
    property bool _blockUpdate: true
    property double startValue: 1
    property double middleValue: 1
    property double endValue: 1

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        _blockUpdate = true;
        brightnessSlider.value = filter.getDouble('level', position) * 100;
        brightnessKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('level') > 0;
        _blockUpdate = false;
        brightnessSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilter(position) {
        if (_blockUpdate)
            return;
        var value = brightnessSlider.value / 100;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value;
            else
                middleValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('level');
            brightnessKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('level', startValue, 0);
                filter.set('level', middleValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('level', middleValue, filter.duration - filter.animateOut);
                filter.set('level', endValue, filter.duration - 1);
            }
        } else if (!brightnessKeyframesButton.checked) {
            filter.resetProperty('level');
            filter.set('level', middleValue);
        } else if (position !== null) {
            filter.set('level', value, position);
        }
    }

    width: 200
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('level', 1);
            filter.savePreset(preset.parameters);
        } else {
            middleValue = filter.getDouble('level', filter.animateIn);
            if (filter.animateIn > 0)
                startValue = filter.getDouble('level', 0);
            if (filter.animateOut > 0)
                endValue = filter.getDouble('level', filter.duration - 1);
        }
        filter.set('rgb_only', 1);
        setControls();
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
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                setControls();
            } else {
                _blockUpdate = true;
                brightnessSlider.value = filter.getDouble('level', getPosition()) * 100;
                _blockUpdate = false;
                brightnessSlider.enabled = true;
            }
        }

        target: producer
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
            parameters: ['level']
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
            id: levelLabel
            text: qsTr('Level')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: brightnessSlider

            minimumValue: 0
            maximumValue: 200
            decimals: 1
            suffix: ' %'
            onValueChanged: {
                if (_blockUpdate)
                    return;
                filter.startUndoParameterCommand(levelLabel.text);
                updateFilter(getPosition());
                filter.endUndoCommand();
            }
        }

        Shotcut.UndoButton {
            onClicked: brightnessSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: brightnessKeyframesButton

            onToggled: {
                if (_blockUpdate)
                    return;
                filter.startUndoParameterCommand(levelLabel.text);
                var value = brightnessSlider.value / 100;
                if (checked) {
                    _blockUpdate = true;
                    filter.clearSimpleAnimation('level');
                    _blockUpdate = false;
                    filter.set('level', value, getPosition());
                } else {
                    filter.resetProperty('level');
                    filter.set('level', value);
                }
                filter.endUndoCommand();
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
