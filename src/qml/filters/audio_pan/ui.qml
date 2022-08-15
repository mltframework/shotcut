/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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
    property double startValue: 0
    property double middleValue: 0
    property double endValue: 0

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        slider.value = filter.getDouble('split', position) * slider.maximumValue;
        keyframesButton.checked = filter.keyframeCount('split') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
        slider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilter(position) {
        if (blockUpdate)
            return ;

        var value = slider.value / slider.maximumValue;
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
    height: 80
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            combo.currentIndex = 0;
            filter.set('channel', 0);
            filter.set('start', 0);
            filter.set('split', 0);
            filter.savePreset(preset.parameters);
        } else {
            // Convert old version of filter.
            if (filter.getDouble('start') !== 0)
                filter.set('split', filter.getDouble('start'));

            middleValue = filter.getDouble('split', filter.animateIn);
            if (filter.animateIn > 0)
                startValue = filter.getDouble('split', 0);

            if (filter.animateOut > 0)
                endValue = filter.getDouble('split', filter.duration - 1);

        }
        setControls();
        combo.currentIndex = filter.get("channel");
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
            parameters: ['split', 'channel']
            onBeforePresetLoaded: {
                filter.resetProperty(parameters[0]);
            }
            onPresetSelected: {
                setControls();
                combo.currentIndex = filter.get("channel");
                middleValue = filter.getDouble(parameters[0], filter.animateIn);
                if (filter.animateIn > 0)
                    startValue = filter.getDouble(parameters[0], 0);

                if (filter.animateOut > 0)
                    endValue = filter.getDouble(parameters[0], filter.duration - 1);

            }
        }

        Label {
            text: qsTr('Channel')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: combo

            Layout.columnSpan: 3
            model: [qsTr('Left'), qsTr('Right')]
            onActivated: filter.set('channel', currentIndex)
        }

        Label {
            text: qsTr('Left')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider

            minimumValue: 0
            maximumValue: 1000
            label: qsTr('Right')
            ratio: maximumValue
            decimals: 2
            onValueChanged: updateFilter(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: slider.value = 0
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
