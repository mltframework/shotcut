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
    property double fovStart: 0
    property double fovMiddle: 0
    property double fovEnd: 0
    property double fisheyeStart: 0
    property double fisheyeMiddle: 0
    property double fisheyeEnd: 0
    property int interpolationValue: 0

    function updateSimpleKeyframes() {
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            // When enabling simple keyframes, initialize the keyframes with the current value
            if (filter.keyframeCount("yaw") <= 0)
                yawStart = yawMiddle = yawEnd = filter.getDouble("yaw");
            if (filter.keyframeCount("pitch") <= 0)
                pitchStart = pitchMiddle = pitchEnd = filter.getDouble("pitch");
            if (filter.keyframeCount("roll") <= 0)
                rollStart = rollMiddle = rollEnd = filter.getDouble("roll");
            if (filter.keyframeCount("fov") <= 0)
                fovStart = fovMiddle = fovEnd = filter.getDouble("fov");
            if (filter.keyframeCount("fisheye") <= 0)
                fisheyeStart = fisheyeMiddle = fisheyeEnd = filter.getDouble("fisheye");
        }
        setControls();
        updateProperty_yaw(null);
        updateProperty_pitch(null);
        updateProperty_roll(null);
        updateProperty_fov(null);
        updateProperty_fisheye(null);
        updateProperty_interpolation();
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
        fovSlider.value = filter.getDouble("fov", position);
        fovKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("fov") > 0;
        fisheyeSlider.value = filter.getDouble("fisheye", position);
        fisheyeKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("fisheye") > 0;
        interpolationComboBox.currentIndex = filter.get("interpolation");
        blockUpdate = false;
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

    function updateProperty_fov(position) {
        if (blockUpdate)
            return;
        var value = fovSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                fovStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                fovEnd = value;
            else
                fovMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("fov");
            fovKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("fov", fovStart, 0);
                filter.set("fov", fovMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("fov", fovMiddle, filter.duration - filter.animateOut);
                filter.set("fov", fovEnd, filter.duration - 1);
            }
        } else if (!fovKeyframesButton.checked) {
            filter.resetProperty("fov");
            filter.set("fov", fovMiddle);
        } else if (position !== null) {
            filter.set("fov", value, position);
        }
    }

    function updateProperty_fisheye(position) {
        if (blockUpdate)
            return;
        var value = fisheyeSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                fisheyeStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                fisheyeEnd = value;
            else
                fisheyeMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("fisheye");
            fisheyeKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("fisheye", fisheyeStart, 0);
                filter.set("fisheye", fisheyeMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("fisheye", fisheyeMiddle, filter.duration - filter.animateOut);
                filter.set("fisheye", fisheyeEnd, filter.duration - 1);
            }
        } else if (!fisheyeKeyframesButton.checked) {
            filter.resetProperty("fisheye");
            filter.set("fisheye", fisheyeMiddle);
        } else if (position !== null) {
            filter.set("fisheye", value, position);
        }
    }

    function updateProperty_interpolation() {
        if (blockUpdate)
            return;
        var value = interpolationComboBox.currentIndex;
        filter.set("interpolation", value);
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
            filter.set("fov", 90);
        } else {
            fovMiddle = filter.getDouble("fov", filter.animateIn);
            if (filter.animateIn > 0)
                fovStart = filter.getDouble("fov", 0);
            if (filter.animateOut > 0)
                fovEnd = filter.getDouble("fov", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("fisheye", 0);
        } else {
            fisheyeMiddle = filter.getDouble("fisheye", filter.animateIn);
            if (filter.animateIn > 0)
                fisheyeStart = filter.getDouble("fisheye", 0);
            if (filter.animateOut > 0)
                fisheyeEnd = filter.getDouble("fisheye", filter.duration - 1);
        }
        if (filter.isNew)
            filter.set("interpolation", 1);
        else
            interpolationValue = filter.get("interpolation");
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

            parameters: ["yaw", "pitch", "roll", "fov", "interpolation", "fisheye"]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty('yaw');
                filter.resetProperty('pitch');
                filter.resetProperty('roll');
                filter.resetProperty('fov');
                filter.resetProperty('fisheye');
                filter.resetProperty('interpolation');
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
                fovMiddle = filter.getDouble("fov", filter.animateIn);
                if (filter.animateIn > 0)
                    fovStart = filter.getDouble("fov", 0);
                if (filter.animateOut > 0)
                    fovEnd = filter.getDouble("fov", filter.duration - 1);
                fisheyeMiddle = filter.getDouble("fisheye", filter.animateIn);
                if (filter.animateIn > 0)
                    fisheyeStart = filter.getDouble("fisheye", 0);
                if (filter.animateOut > 0)
                    fisheyeEnd = filter.getDouble("fisheye", filter.duration - 1);
                interpolationValue = filter.get("interpolation");
                setControls(null);
            }
        }

        Label {
            text: qsTr('Interpolation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: interpolationComboBox

            currentIndex: 0
            model: ["Nearest-neighbor", "Bilinear"]
            onCurrentIndexChanged: updateProperty_interpolation()
        }

        Shotcut.UndoButton {
            id: interpolationUndo

            onClicked: interpolationComboBox.currentIndex = 0
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr('Yaw')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: yawSlider

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
            text: qsTr('FOV', 'field of view')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr('Field of view')
            }
        }

        Shotcut.SliderSpinner {
            id: fovSlider

            minimumValue: 0
            maximumValue: 720
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_fov(getPosition())
        }

        Shotcut.UndoButton {
            id: fovUndo

            onClicked: fovSlider.value = 90
        }

        Shotcut.KeyframesButton {
            id: fovKeyframesButton

            onToggled: {
                var value = fovSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("fov");
                        fovSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("fov");
                    blockUpdate = false;
                    filter.set("fov", value, getPosition());
                } else {
                    filter.resetProperty("fov");
                    filter.set("fov", value);
                }
            }
        }

        Label {
            text: qsTr('Fisheye')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: fisheyeSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            decimals: 0
            stepSize: 1
            onValueChanged: updateProperty_fisheye(getPosition())
        }

        Shotcut.UndoButton {
            id: fisheyeUndo

            onClicked: fisheyeSlider.value = 0
        }

        Shotcut.KeyframesButton {
            id: fisheyeKeyframesButton

            onToggled: {
                var value = fisheyeSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("fisheye");
                        fisheyeSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("fisheye");
                    blockUpdate = false;
                    filter.set("fisheye", value, getPosition());
                } else {
                    filter.resetProperty("fisheye");
                    filter.set("fisheye", value);
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
