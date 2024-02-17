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
    property double yawStart: 0
    property double yawMiddle: 0
    property double yawEnd: 0
    property double pitchStart: 0
    property double pitchMiddle: 0
    property double pitchEnd: 0
    property double rollStart: 0
    property double rollMiddle: 0
    property double rollEnd: 0
    property double zoomStart: 0
    property double zoomMiddle: 0
    property double zoomEnd: 0

    function updateSimpleKeyframes() {
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            // When enabling simple keyframes, initialize the keyframes with the current value
            if (filter.keyframeCount("yaw") <= 0)
                yawStart = yawMiddle = yawEnd = filter.getDouble("yaw");
            if (filter.keyframeCount("pitch") <= 0)
                pitchStart = pitchMiddle = pitchEnd = filter.getDouble("pitch");
            if (filter.keyframeCount("roll") <= 0)
                rollStart = rollMiddle = rollEnd = filter.getDouble("roll");
            if (filter.keyframeCount("zoom") <= 0)
                zoomStart = zoomMiddle = zoomEnd = filter.getDouble("zoom");
        }
        setControls();
        updateProperty_yaw(null);
        updateProperty_pitch(null);
        updateProperty_roll(null);
        updateProperty_zoom(null);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        yawSlider.value = filter.getDouble("yaw", position);
        yawKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("yaw") > 0;
        pitchSlider.value = filter.getDouble("pitch", position);
        pitchKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("pitch") > 0;
        rollSlider.value = filter.getDouble("roll", position);
        rollKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("roll") > 0;
        zoomSlider.value = filter.getDouble("zoom", position) * 100;
        zoomKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("zoom") > 0;
        blockUpdate = false;
        binauralRadioButton.checked = parseInt(filter.get('binaural'));
        ambisonicRadioButton.checked = parseInt(filter.get('ambisonic'));
        if (application.audioChannels() === 2)
            stereoRadioButton.checked = !binauralRadioButton.checked && !ambisonicRadioButton.checked;
        else if (application.audioChannels() === 4)
            quadRadioButton.checked = !binauralRadioButton.checked && !ambisonicRadioButton.checked;
    }

    function updateProperty_yaw(position) {
        if (blockUpdate)
            return;
        var value = yawSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                yawStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                yawEnd = value;
            else
                yawMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("yaw");
            yawKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("yaw", yawStart, 0);
                filter.set("yaw", yawMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("yaw", yawMiddle, filter.duration - filter.animateOut);
                filter.set("yaw", yawEnd, filter.duration - 1);
            }
        } else if (!yawKeyframesButton.checked) {
            filter.resetProperty("yaw");
            filter.set("yaw", yawMiddle);
        } else if (position !== null) {
            filter.set("yaw", value, position);
        }
    }

    function updateProperty_pitch(position) {
        if (blockUpdate)
            return;
        var value = pitchSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                pitchStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                pitchEnd = value;
            else
                pitchMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("pitch");
            pitchKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("pitch", pitchStart, 0);
                filter.set("pitch", pitchMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("pitch", pitchMiddle, filter.duration - filter.animateOut);
                filter.set("pitch", pitchEnd, filter.duration - 1);
            }
        } else if (!pitchKeyframesButton.checked) {
            filter.resetProperty("pitch");
            filter.set("pitch", pitchMiddle);
        } else if (position !== null) {
            filter.set("pitch", value, position);
        }
    }

    function updateProperty_roll(position) {
        if (blockUpdate)
            return;
        var value = rollSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                rollStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                rollEnd = value;
            else
                rollMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("roll");
            rollKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("roll", rollStart, 0);
                filter.set("roll", rollMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("roll", rollMiddle, filter.duration - filter.animateOut);
                filter.set("roll", rollEnd, filter.duration - 1);
            }
        } else if (!rollKeyframesButton.checked) {
            filter.resetProperty("roll");
            filter.set("roll", rollMiddle);
        } else if (position !== null) {
            filter.set("roll", value, position);
        }
    }

    function updateProperty_zoom(position) {
        if (blockUpdate)
            return;
        var value = zoomSlider.value / 100;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                zoomStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                zoomEnd = value;
            else
                zoomMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("zoom");
            zoomKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("zoom", zoomStart, 0);
                filter.set("zoom", zoomMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("zoom", zoomMiddle, filter.duration - filter.animateOut);
                filter.set("zoom", zoomEnd, filter.duration - 1);
            }
        } else if (!zoomKeyframesButton.checked) {
            filter.resetProperty("zoom");
            filter.set("zoom", zoomMiddle);
        } else if (position !== null) {
            filter.set("zoom", value, position);
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    width: 350
    height: 200
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set("yaw", 0);
        } else {
            yawMiddle = filter.getDouble("yaw", filter.animateIn);
            if (filter.animateIn > 0)
                yawStart = filter.getDouble("yaw", 0);
            if (filter.animateOut > 0)
                yawEnd = filter.getDouble("yaw", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("pitch", 0);
        } else {
            pitchMiddle = filter.getDouble("pitch", filter.animateIn);
            if (filter.animateIn > 0)
                pitchStart = filter.getDouble("pitch", 0);
            if (filter.animateOut > 0)
                pitchEnd = filter.getDouble("pitch", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("roll", 0);
        } else {
            rollMiddle = filter.getDouble("roll", filter.animateIn);
            if (filter.animateIn > 0)
                rollStart = filter.getDouble("roll", 0);
            if (filter.animateOut > 0)
                rollEnd = filter.getDouble("roll", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("zoom", 0);
        } else {
            zoomMiddle = filter.getDouble("zoom", filter.animateIn);
            if (filter.animateIn > 0)
                zoomStart = filter.getDouble("zoom", 0);
            if (filter.animateOut > 0)
                zoomEnd = filter.getDouble("zoom", filter.duration - 1);
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

            parameters: ["yaw", "pitch", "roll", "zoom"]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty('yaw');
                filter.resetProperty('pitch');
                filter.resetProperty('roll');
                filter.resetProperty('zoom');
            }
            onPresetSelected: {
                yawMiddle = filter.getDouble("yaw", filter.animateIn);
                if (filter.animateIn > 0)
                    yawStart = filter.getDouble("yaw", 0);
                if (filter.animateOut > 0)
                    yawEnd = filter.getDouble("yaw", filter.duration - 1);
                pitchMiddle = filter.getDouble("pitch", filter.animateIn);
                if (filter.animateIn > 0)
                    pitchStart = filter.getDouble("pitch", 0);
                if (filter.animateOut > 0)
                    pitchEnd = filter.getDouble("pitch", filter.duration - 1);
                rollMiddle = filter.getDouble("roll", filter.animateIn);
                if (filter.animateIn > 0)
                    rollStart = filter.getDouble("roll", 0);
                if (filter.animateOut > 0)
                    rollEnd = filter.getDouble("roll", filter.duration - 1);
                zoomMiddle = filter.getDouble("zoom", filter.animateIn);
                if (filter.animateIn > 0)
                    zoomStart = filter.getDouble("zoom", 0);
                if (filter.animateOut > 0)
                    zoomEnd = filter.getDouble("zoom", filter.duration - 1);
                setControls(null);
            }
        }

        Label {
            id: modeLabel
            text: qsTr('Mode')
            Layout.alignment: Qt.AlignRight
            visible: application.audioChannels() <= 4
        }
        RowLayout {
            visible: modeLabel.visible

            RadioButton {
                id: stereoRadioButton
                text: qsTr('Stereo')
                visible: application.audioChannels() === 2
                onClicked: {
                    filter.set('ambisonic', 0);
                    filter.set('binaural', 0);
                }
            }
            RadioButton {
                id: binauralRadioButton
                text: qsTr('Binaural')
                visible: application.audioChannels() === 2 || application.audioChannels() === 4
                onClicked: {
                    filter.set('ambisonic', 0);
                    filter.set('binaural', 1);
                }
            }
            RadioButton {
                id: quadRadioButton
                text: qsTr('Quad')
                visible: application.audioChannels() === 4
                onClicked: {
                    filter.set('ambisonic', 0);
                    filter.set('binaural', 0);
                }
            }
            RadioButton {
                id: ambisonicRadioButton
                text: 'Ambisonic'
                visible: application.audioChannels() === 4
                onClicked: {
                    filter.set('ambisonic', 1);
                    filter.set('binaural', 0);
                }
            }
        }

        Shotcut.UndoButton {
            Layout.columnSpan: 2
            visible: modeLabel.visible
            onClicked: {
                stereoRadioButton.checked = true;
                filter.set('binaural', 0);
                quadRadioButton.checked = true;
                // ambisonicRadioButton.checked = false;
                filter.set('ambisonic', 0);
            }
        }

        Label {
            text: qsTr('Yaw')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: yawSlider

            enabled: !binauralRadioButton.checked
            minimumValue: -360
            maximumValue: 360
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_yaw(getPosition())
        }

        Shotcut.UndoButton {
            id: yawUndo

            onClicked: yawSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: yawKeyframesButton

            enabled: !binauralRadioButton.checked
            onToggled: {
                var value = yawSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("yaw");
                        yawSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("yaw");
                    blockUpdate = false;
                    filter.set("yaw", value, getPosition());
                } else {
                    filter.resetProperty("yaw");
                    filter.set("yaw", value);
                }
            }
        }

        Label {
            text: qsTr('Pitch', 'rotation around the side-to-side axis (roll, pitch, yaw)')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: pitchSlider

            enabled: !binauralRadioButton.checked
            minimumValue: -180
            maximumValue: 180
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_pitch(getPosition())
        }

        Shotcut.UndoButton {
            id: pitchUndo

            onClicked: pitchSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: pitchKeyframesButton

            enabled: !binauralRadioButton.checked
            onToggled: {
                var value = pitchSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("pitch");
                        pitchSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("pitch");
                    blockUpdate = false;
                    filter.set("pitch", value, getPosition());
                } else {
                    filter.resetProperty("pitch");
                    filter.set("pitch", value);
                }
            }
        }

        Label {
            text: qsTr('Roll')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: rollSlider

            enabled: !binauralRadioButton.checked
            minimumValue: -180
            maximumValue: 180
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_roll(getPosition())
        }

        Shotcut.UndoButton {
            id: rollUndo

            onClicked: rollSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: rollKeyframesButton

            enabled: !binauralRadioButton.checked
            onToggled: {
                var value = rollSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("roll");
                        rollSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("roll");
                    blockUpdate = false;
                    filter.set("roll", value, getPosition());
                } else {
                    filter.resetProperty("roll");
                    filter.set("roll", value);
                }
            }
        }

        Label {
            text: qsTr('Zoom')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: zoomSlider

            enabled: !binauralRadioButton.checked
            minimumValue: -100
            maximumValue: 100
            spinnerWidth: 120
            suffix: ' %'
            decimals: 1
            stepSize: 1
            onValueChanged: updateProperty_zoom(getPosition())
        }

        Shotcut.UndoButton {
            id: zoomUndo

            onClicked: zoomSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: zoomKeyframesButton

            enabled: !binauralRadioButton.checked
            onToggled: {
                var value = zoomSlider.value / 100;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("zoom");
                        zoomSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("zoom");
                    blockUpdate = false;
                    filter.set("zoom", value, getPosition());
                } else {
                    filter.resetProperty("zoom");
                    filter.set("zoom", value);
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
