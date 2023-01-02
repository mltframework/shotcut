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
    property bool blockUpdate: true
    property double startValue: 1
    property double middleValue: 1
    property double endValue: 1

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        brightnessSlider.value = filter.getDouble('opacity', position) * 100;
        brightnessKeyframesButton.checked = filter.keyframeCount('opacity') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
        brightnessSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilter(position) {
        if (blockUpdate)
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
            filter.resetProperty('opacity');
            brightnessKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('opacity', startValue, 0);
                filter.set('opacity', middleValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('opacity', middleValue, filter.duration - filter.animateOut);
                filter.set('opacity', endValue, filter.duration - 1);
            }
        } else if (!brightnessKeyframesButton.checked) {
            filter.resetProperty('opacity');
            filter.set('opacity', middleValue);
        } else if (position !== null) {
            filter.set('opacity', value, position);
        }
    }

    width: 200
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('alpha', 1);
            filter.set('opacity', 1);
            filter.savePreset(preset.parameters);
        } else {
            middleValue = filter.getDouble('opacity', filter.animateIn);
            if (filter.animateIn > 0)
                startValue = filter.getDouble('opacity', 0);
            if (filter.animateOut > 0)
                endValue = filter.getDouble('opacity', filter.duration - 1);
        }
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
                blockUpdate = true;
                brightnessSlider.value = filter.getDouble('opacity', getPosition()) * 100;
                blockUpdate = false;
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
            parameters: ['opacity']
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
            text: qsTr('Level')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: brightnessSlider

            minimumValue: 0
            maximumValue: 200
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: brightnessSlider.value = 100
        }

        Shotcut.KeyframesButton {
            id: brightnessKeyframesButton

            onToggled: {
                var value = brightnessSlider.value / 100;
                if (checked) {
                    blockUpdate = true;
                    filter.clearSimpleAnimation('opacity');
                    blockUpdate = false;
                    filter.set('opacity', value, getPosition());
                } else {
                    filter.resetProperty('opacity');
                    filter.set('opacity', value);
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
