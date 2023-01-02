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
    property string paramBlur: '0'
    property var defaultParameters: [paramBlur]
    property bool blockUpdate: true
    property double startValue: 0
    property double middleValue: 0.5
    property double endValue: 0

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        bslider.value = filter.getDouble(paramBlur, position) * 100;
        blurKeyframesButton.checked = filter.keyframeCount(paramBlur) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
        bslider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateFilter(position) {
        if (blockUpdate)
            return;
        var value = bslider.value / 100;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value;
            else
                middleValue = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(paramBlur);
            blurKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set(paramBlur, startValue, 0);
                filter.set(paramBlur, middleValue, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set(paramBlur, middleValue, filter.duration - filter.animateOut);
                filter.set(paramBlur, endValue, filter.duration - 1);
            }
        } else if (!blurKeyframesButton.checked) {
            filter.resetProperty(paramBlur);
            filter.set(paramBlur, middleValue);
        } else if (position !== null) {
            filter.set(paramBlur, value, position);
        }
    }

    width: 350
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set(paramBlur, 50 / 100);
            filter.savePreset(defaultParameters);
        } else {
            middleValue = filter.getDouble(paramBlur, filter.animateIn);
            if (filter.animateIn > 0)
                startValue = filter.getDouble(paramBlur, 0);
            if (filter.animateOut > 0)
                endValue = filter.getDouble(paramBlur, filter.duration - 1);
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
            onBeforePresetLoaded: filter.resetProperty(paramBlur)
            onPresetSelected: {
                setControls();
                middleValue = filter.getDouble(paramBlur, filter.animateIn);
                if (filter.animateIn > 0)
                    startValue = filter.getDouble(paramBlur, 0);
                if (filter.animateOut > 0)
                    endValue = filter.getDouble(paramBlur, filter.duration - 1);
            }
        }

        Label {
            text: qsTr('Blur')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: bslider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: updateFilter(getPosition())
        }

        Shotcut.UndoButton {
            onClicked: bslider.value = 50
        }

        Shotcut.KeyframesButton {
            id: blurKeyframesButton

            onToggled: {
                var value = bslider.value / 100;
                if (checked) {
                    blockUpdate = true;
                    filter.clearSimpleAnimation(paramBlur);
                    blockUpdate = false;
                    filter.set(paramBlur, value, getPosition());
                } else {
                    filter.resetProperty(paramBlur);
                    filter.set(paramBlur, value);
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
