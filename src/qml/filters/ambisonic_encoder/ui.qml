/*
 * Copyright (c) 2024 Meltytech, LLC
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
    property double azimuthStart: 0
    property double azimuthMiddle: 0
    property double azimuthEnd: 0
    property double elevationStart: 0
    property double elevationMiddle: 0
    property double elevationEnd: 0

    function updateSimpleKeyframes() {
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            // When enabling simple keyframes, initialize the keyframes with the current value
            if (filter.keyframeCount("azimuth") <= 0)
                azimuthStart = azimuthMiddle = azimuthEnd = filter.getDouble("azimuth");
            if (filter.keyframeCount("elevation") <= 0)
                elevationStart = elevationMiddle = elevationEnd = filter.getDouble("elevation");
        }
        setControls();
        updateProperty_azimuth(null);
        updateProperty_elevation(null);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        azimuthSlider.value = filter.getDouble("azimuth", position);
        azimuthKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("azimuth") > 0;
        elevationSlider.value = filter.getDouble("elevation", position);
        elevationKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("elevation") > 0;
        blockUpdate = false;
    }

    function updateProperty_azimuth(position) {
        if (blockUpdate)
            return;
        var value = azimuthSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                azimuthStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                azimuthEnd = value;
            else
                azimuthMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("azimuth");
            azimuthKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("azimuth", azimuthStart, 0);
                filter.set("azimuth", azimuthMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("azimuth", azimuthMiddle, filter.duration - filter.animateOut);
                filter.set("azimuth", azimuthEnd, filter.duration - 1);
            }
        } else if (!azimuthKeyframesButton.checked) {
            filter.resetProperty("azimuth");
            filter.set("azimuth", azimuthMiddle);
        } else if (position !== null) {
            filter.set("azimuth", value, position);
        }
    }

    function updateProperty_elevation(position) {
        if (blockUpdate)
            return;
        var value = elevationSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                elevationStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                elevationEnd = value;
            else
                elevationMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("elevation");
            elevationKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("elevation", elevationStart, 0);
                filter.set("elevation", elevationMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("elevation", elevationMiddle, filter.duration - filter.animateOut);
                filter.set("elevation", elevationEnd, filter.duration - 1);
            }
        } else if (!elevationKeyframesButton.checked) {
            filter.resetProperty("elevation");
            filter.set("elevation", elevationMiddle);
        } else if (position !== null) {
            filter.set("elevation", value, position);
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    width: 350
    height: 200
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set("azimuth", 0);
        } else {
            azimuthMiddle = filter.getDouble("azimuth", filter.animateIn);
            if (filter.animateIn > 0)
                azimuthStart = filter.getDouble("azimuth", 0);
            if (filter.animateOut > 0)
                azimuthEnd = filter.getDouble("azimuth", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("elevation", 0);
        } else {
            elevationMiddle = filter.getDouble("elevation", filter.animateIn);
            if (filter.animateIn > 0)
                elevationStart = filter.getDouble("elevation", 0);
            if (filter.animateOut > 0)
                elevationEnd = filter.getDouble("elevation", filter.duration - 1);
        }
        if (filter.isNew)
            filter.savePreset(preset.parameters);
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

            parameters: ["azimuth", "elevation"]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty('azimuth');
                filter.resetProperty('elevation');
            }
            onPresetSelected: {
                azimuthMiddle = filter.getDouble("azimuth", filter.animateIn);
                if (filter.animateIn > 0)
                    azimuthStart = filter.getDouble("azimuth", 0);
                if (filter.animateOut > 0)
                    azimuthEnd = filter.getDouble("azimuth", filter.duration - 1);
                elevationMiddle = filter.getDouble("elevation", filter.animateIn);
                if (filter.animateIn > 0)
                    elevationStart = filter.getDouble("elevation", 0);
                if (filter.animateOut > 0)
                    elevationEnd = filter.getDouble("elevation", filter.duration - 1);
                setControls(null);
            }
        }

        Label {
            text: qsTr('Azimuth')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: azimuthSlider

            minimumValue: -360
            maximumValue: 360
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 1
            stepSize: 1
            onValueChanged: updateProperty_azimuth(getPosition())
        }

        Shotcut.UndoButton {
            id: azimuthUndo

            onClicked: azimuthSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: azimuthKeyframesButton

            onToggled: {
                var value = azimuthSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("azimuth");
                        azimuthSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("azimuth");
                    blockUpdate = false;
                    filter.set("azimuth", value, getPosition());
                } else {
                    filter.resetProperty("azimuth");
                    filter.set("azimuth", value);
                }
            }
        }

        Label {
            text: qsTr('Elevation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: elevationSlider

            minimumValue: -360
            maximumValue: 360
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 1
            stepSize: 1
            onValueChanged: updateProperty_elevation(getPosition())
        }

        Shotcut.UndoButton {
            id: elevationUndo

            onClicked: elevationSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: elevationKeyframesButton

            onToggled: {
                var value = elevationSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("elevation");
                        elevationSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("elevation");
                    blockUpdate = false;
                    filter.set("elevation", value, getPosition());
                } else {
                    filter.resetProperty("elevation");
                    filter.set("elevation", value);
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
            updateSimpleKeyframes();
        }

        function onOutChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateInChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateOutChanged() {
            updateSimpleKeyframes();
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
