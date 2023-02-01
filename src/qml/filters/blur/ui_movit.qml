/*
 * Copyright (c) 2013-2023 Meltytech, LLC
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
    property double startValue: 0
    property double middleValue: 3
    property double endValue: 0

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        slider.value = filter.getDouble('radius', position);
        keyframesButton.checked = filter.keyframeCount('radius') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
        slider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilter(position) {
        if (blockUpdate)
            return;
        var value = slider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value;
            else
                middleValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('radius');
            keyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('radius', startValue, 0);
                filter.set('radius', middleValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('radius', middleValue, filter.duration - filter.animateOut);
                filter.set('radius', endValue, filter.duration - 1);
            }
        } else if (!keyframesButton.checked) {
            filter.resetProperty('radius');
            filter.set('radius', middleValue);
        } else if (position !== null) {
            filter.set('radius', value, position);
        }
    }

    width: 200
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('radius', 3);
            filter.savePreset(preset.parameters);
        } else {
            middleValue = filter.getDouble('radius', filter.animateIn);
            if (filter.animateIn > 0)
                startValue = filter.getDouble('radius', 0);
            if (filter.animateOut > 0)
                endValue = filter.getDouble('radius', filter.duration - 1);
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
            parameters: ['radius']
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
            text: qsTr('Radius')
        }

        Shotcut.SliderSpinner {
            id: slider

            minimumValue: 0
            maximumValue: 99.99
            decimals: 2
            onValueChanged: updateFilter(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: slider.value = 3
        }

        Shotcut.KeyframesButton {
            id: keyframesButton

            onToggled: {
                if (checked) {
                    blockUpdate = true;
                    filter.clearSimpleAnimation('radius');
                    blockUpdate = false;
                    filter.set('radius', slider.value, getPosition());
                } else {
                    filter.resetProperty('radius');
                    filter.set('radius', slider.value);
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
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                setControls();
            } else {
                blockUpdate = true;
                slider.value = filter.getDouble('radius', getPosition());
                blockUpdate = false;
                slider.enabled = true;
            }
        }

        target: producer
    }
}
