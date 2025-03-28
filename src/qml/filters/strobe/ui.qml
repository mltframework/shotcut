/*
 * Copyright (c) 2025 Meltytech, LLC
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
    property var defaultParameters: ['strobe_invert', 'interval']
    property bool blockUpdate: true
    property double startValue: 0
    property double middleValue: 1
    property double endValue: 0

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        islider.value = filter.getDouble('interval', position);
        iKeyframesButton.checked = filter.keyframeCount('interval') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
        islider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilter(position) {
        if (blockUpdate)
            return;
        var value = islider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value;
            else
                middleValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('interval');
            iKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set('interval', startValue, 0);
                filter.set('interval', middleValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set('interval', middleValue, filter.duration - filter.animateOut);
                filter.set('interval', endValue, filter.duration - 1);
            }
        } else if (!iKeyframesButton.checked) {
            filter.resetProperty('interval');
            filter.set('interval', middleValue);
        } else if (position !== null) {
            filter.set('interval', value, position);
        }
    }

    width: 350
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('interval', 1);
            filter.set('strobe_invert', 0);
            filter.savePreset(defaultParameters);
        } else {
            middleValue = filter.getDouble('interval', filter.animateIn);
            if (filter.animateIn > 0)
                startValue = filter.getDouble('interval', 0);
            if (filter.animateOut > 0)
                endValue = filter.getDouble('interval', filter.duration - 1);
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
            onBeforePresetLoaded: filter.resetProperty('interval')
            onPresetSelected: {
                setControls();
                middleValue = filter.getDouble('interval', filter.animateIn);
                if (filter.animateIn > 0)
                    startValue = filter.getDouble('interval', 0);
                if (filter.animateOut > 0)
                    endValue = filter.getDouble('interval', filter.duration - 1);
            }
        }

        Label {
            text: qsTr('Interval')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: islider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: islider.value = 1
        }

        Shotcut.KeyframesButton {
            id: iKeyframesButton

            onToggled: {
                if (checked) {
                    blockUpdate = true;
                    filter.clearSimpleAnimation('interval');
                    blockUpdate = false;
                    filter.set('interval', islider.value, getPosition());
                } else {
                    filter.resetProperty('interval');
                    filter.set('interval', islider.value);
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
