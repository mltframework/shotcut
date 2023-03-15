/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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
    property double middleValue: 0
    property double endValue: 0

    function toDb(value) {
        return 20 * Math.log(value) / Math.LN10;
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        gainSlider.value = filter.getDouble('level', position);
        if (filter.animateIn > 0 || filter.animateOut > 0)
            gainSlider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
        else
            gainSlider.enabled = true;
        gainKeyframesButton.checked = filter.keyframeCount('level') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
    }

    function updateFilter(position) {
        if (blockUpdate)
            return;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = gainSlider.value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = gainSlider.value;
            else
                middleValue = gainSlider.value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('level');
            gainKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('level', startValue, 0);
                filter.set('level', middleValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('level', middleValue, filter.duration - filter.animateOut);
                filter.set('level', endValue, filter.duration - 1);
            }
        } else if (!gainKeyframesButton.checked) {
            filter.resetProperty('level');
            filter.set('level', middleValue);
        } else if (position !== null) {
            filter.set('level', gainSlider.value, position);
        }
    }

    width: 200
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('level', 0);
            filter.savePreset(preset.parameters);
        } else {
            // Convert old version of filter.
            if (filter.getDouble('gain') !== 0) {
                filter.set('level', toDb(filter.getDouble('gain')));
                filter.resetProperty('gain');
            }
            middleValue = filter.getDouble('level', filter.animateIn);
            if (filter.animateIn > 0)
                startValue = filter.getDouble('level', 0);
            if (filter.animateOut > 0)
                endValue = filter.getDouble('level', filter.duration - 1);
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
            setControls();
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
            text: qsTr('Level')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: gainSlider

            minimumValue: -70
            maximumValue: 24
            suffix: ' dB'
            decimals: 1
            onValueChanged: updateFilter(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: gainSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: gainKeyframesButton

            onToggled: {
                if (checked) {
                    blockUpdate = true;
                    filter.clearSimpleAnimation('level');
                    blockUpdate = false;
                    filter.set('level', gainSlider.value, getPosition());
                } else {
                    filter.resetProperty('level');
                    filter.set('level', gainSlider.value);
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
