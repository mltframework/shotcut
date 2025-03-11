import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut
Item {
    property bool blockUpdate: true
    property double hfov0Start: 0
    property double hfov0Middle: 0
    property double hfov0End: 0
    property double hfov1Start: 0
    property double hfov1Middle: 0
    property double hfov1End: 0
    property double vfov0Start: 0
    property double vfov0Middle: 0
    property double vfov0End: 0
    property double vfov1Start: 0
    property double vfov1Middle: 0
    property double vfov1End: 0
    property double blurStartStart: 0
    property double blurStartMiddle: 0
    property double blurStartEnd: 0
    property double blurEndStart: 0
    property double blurEndMiddle: 0
    property double blurEndEnd: 0
    function updateSimpleKeyframes() {
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            if (filter.keyframeCount("hfov0") <= 0)
            hfov0Start = hfov0Middle = hfov0End = filter.getDouble("hfov0");
            if (filter.keyframeCount("hfov1") <= 0)
            hfov1Start = hfov1Middle = hfov1End = filter.getDouble("hfov1");
            if (filter.keyframeCount("vfov0") <= 0)
            vfov0Start = vfov0Middle = vfov0End = filter.getDouble("vfov0");
            if (filter.keyframeCount("vfov1") <= 0)
            vfov1Start = vfov1Middle = vfov1End = filter.getDouble("vfov1");
            if (filter.keyframeCount("blurStart") <= 0)
            blurStartStart = blurStartMiddle = blurStartEnd = filter.getDouble("blurStart");
            if (filter.keyframeCount("blurEnd") <= 0)
            blurEndStart = blurEndMiddle = blurEndEnd = filter.getDouble("blurEnd");
        }
        setControls();
        updateProperty_hfov0(null);
        updateProperty_hfov1(null);
        updateProperty_vfov0(null);
        updateProperty_vfov1(null);
        updateProperty_blurStart(null);
        updateProperty_blurEnd(null);
    }
    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        hfov0Slider.value = filter.getDouble("hfov0", position);
        hfov0KeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("hfov0") > 0;
        hfov1Slider.value = filter.getDouble("hfov1", position);
        hfov1KeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("hfov1") > 0;
        vfov0Slider.value = filter.getDouble("vfov0", position);
        vfov0KeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("vfov0") > 0;
        vfov1Slider.value = filter.getDouble("vfov1", position);
        vfov1KeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("vfov1") > 0;
        blurStartSlider.value = filter.getDouble("blurStart", position);
        blurStartKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("blurStart") > 0;
        blurEndSlider.value = filter.getDouble("blurEnd", position);
        blurEndKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("blurEnd") > 0;
        blockUpdate = false;
    }
    function updateProperty_hfov0(position) {
        if (blockUpdate)
        return;
        var value = hfov0Slider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
            hfov0Start = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
            hfov0End = value;
            else
            hfov0Middle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("hfov0");
            hfov0KeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("hfov0", hfov0Start, 0);
                filter.set("hfov0", hfov0Middle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("hfov0", hfov0Middle, filter.duration - filter.animateOut);
                filter.set("hfov0", hfov0End, filter.duration - 1);
            }
        } else if (!hfov0KeyframesButton.checked) {
            filter.resetProperty("hfov0");
            filter.set("hfov0", hfov0Middle);
        } else if (position !== null) {
            filter.set("hfov0", value, position);
        }
    }
    function updateProperty_hfov1(position) {
        if (blockUpdate)
        return;
        var value = hfov1Slider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
            hfov1Start = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
            hfov1End = value;
            else
            hfov1Middle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("hfov1");
            hfov1KeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("hfov1", hfov1Start, 0);
                filter.set("hfov1", hfov1Middle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("hfov1", hfov1Middle, filter.duration - filter.animateOut);
                filter.set("hfov1", hfov1End, filter.duration - 1);
            }
        } else if (!hfov1KeyframesButton.checked) {
            filter.resetProperty("hfov1");
            filter.set("hfov1", hfov1Middle);
        } else if (position !== null) {
            filter.set("hfov1", value, position);
        }
    }
    function updateProperty_vfov0(position) {
        if (blockUpdate)
        return;
        var value = vfov0Slider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
            vfov0Start = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
            vfov0End = value;
            else
            vfov0Middle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("vfov0");
            vfov0KeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("vfov0", vfov0Start, 0);
                filter.set("vfov0", vfov0Middle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("vfov0", vfov0Middle, filter.duration - filter.animateOut);
                filter.set("vfov0", vfov0End, filter.duration - 1);
            }
        } else if (!vfov0KeyframesButton.checked) {
            filter.resetProperty("vfov0");
            filter.set("vfov0", vfov0Middle);
        } else if (position !== null) {
            filter.set("vfov0", value, position);
        }
    }
    function updateProperty_vfov1(position) {
        if (blockUpdate)
        return;
        var value = vfov1Slider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
            vfov1Start = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
            vfov1End = value;
            else
            vfov1Middle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("vfov1");
            vfov1KeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("vfov1", vfov1Start, 0);
                filter.set("vfov1", vfov1Middle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("vfov1", vfov1Middle, filter.duration - filter.animateOut);
                filter.set("vfov1", vfov1End, filter.duration - 1);
            }
        } else if (!vfov1KeyframesButton.checked) {
            filter.resetProperty("vfov1");
            filter.set("vfov1", vfov1Middle);
        } else if (position !== null) {
            filter.set("vfov1", value, position);
        }
    }
    function updateProperty_blurStart(position) {
        if (blockUpdate)
        return;
        var value = blurStartSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
            blurStartStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
            blurStartEnd = value;
            else
            blurStartMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("blurStart");
            blurStartKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("blurStart", blurStartStart, 0);
                filter.set("blurStart", blurStartMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("blurStart", blurStartMiddle, filter.duration - filter.animateOut);
                filter.set("blurStart", blurStartEnd, filter.duration - 1);
            }
        } else if (!blurStartKeyframesButton.checked) {
            filter.resetProperty("blurStart");
            filter.set("blurStart", blurStartMiddle);
        } else if (position !== null) {
            filter.set("blurStart", value, position);
        }
    }
    function updateProperty_blurEnd(position) {
        if (blockUpdate)
        return;
        var value = blurEndSlider.value;
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
            blurEndStart = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
            blurEndEnd = value;
            else
            blurEndMiddle = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty("blurEnd");
            blurEndKeyframesButton.checked = false;
            if (filter.animateIn > 0) {
                filter.set("blurEnd", blurEndStart, 0);
                filter.set("blurEnd", blurEndMiddle, filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set("blurEnd", blurEndMiddle, filter.duration - filter.animateOut);
                filter.set("blurEnd", blurEndEnd, filter.duration - 1);
            }
        } else if (!blurEndKeyframesButton.checked) {
            filter.resetProperty("blurEnd");
            filter.set("blurEnd", blurEndMiddle);
        } else if (position !== null) {
            filter.set("blurEnd", value, position);
        }
    }
    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }
    width: 350
    height: 200
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set("hfov0", -90);
        } else {
            hfov0Middle = filter.getDouble("hfov0", filter.animateIn);
            if (filter.animateIn > 0)
            hfov0Start = filter.getDouble("hfov0", 0);
            if (filter.animateOut > 0)
            hfov0End = filter.getDouble("hfov0", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("hfov1", 90);
        } else {
            hfov1Middle = filter.getDouble("hfov1", filter.animateIn);
            if (filter.animateIn > 0)
            hfov1Start = filter.getDouble("hfov1", 0);
            if (filter.animateOut > 0)
            hfov1End = filter.getDouble("hfov1", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("vfov0", -45);
        } else {
            vfov0Middle = filter.getDouble("vfov0", filter.animateIn);
            if (filter.animateIn > 0)
            vfov0Start = filter.getDouble("vfov0", 0);
            if (filter.animateOut > 0)
            vfov0End = filter.getDouble("vfov0", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("vfov1", 45);
        } else {
            vfov1Middle = filter.getDouble("vfov1", filter.animateIn);
            if (filter.animateIn > 0)
            vfov1Start = filter.getDouble("vfov1", 0);
            if (filter.animateOut > 0)
            vfov1End = filter.getDouble("vfov1", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("blurStart", 0.1);
        } else {
            blurStartMiddle = filter.getDouble("blurStart", filter.animateIn);
            if (filter.animateIn > 0)
            blurStartStart = filter.getDouble("blurStart", 0);
            if (filter.animateOut > 0)
            blurStartEnd = filter.getDouble("blurStart", filter.duration - 1);
        }
        if (filter.isNew) {
            filter.set("blurEnd", 1.0);
        } else {
            blurEndMiddle = filter.getDouble("blurEnd", filter.animateIn);
            if (filter.animateIn > 0)
            blurEndStart = filter.getDouble("blurEnd", 0);
            if (filter.animateOut > 0)
            blurEndEnd = filter.getDouble("blurEnd", filter.duration - 1);
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
            parameters: ["hfov0", "hfov1", "vfov0", "vfov1"]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty('hfov0');
                filter.resetProperty('hfov1');
                filter.resetProperty('vfov0');
                filter.resetProperty('vfov1');
                filter.resetProperty('blurStart');
                filter.resetProperty('blurEnd');
            }
            onPresetSelected: {
                hfov0Middle = filter.getDouble("hfov0", filter.animateIn);
                if (filter.animateIn > 0)
                hfov0Start = filter.getDouble("hfov0", 0);
                if (filter.animateOut > 0)
                hfov0End = filter.getDouble("hfov0", filter.duration - 1);
                hfov1Middle = filter.getDouble("hfov1", filter.animateIn);
                if (filter.animateIn > 0)
                hfov1Start = filter.getDouble("hfov1", 0);
                if (filter.animateOut > 0)
                hfov1End = filter.getDouble("hfov1", filter.duration - 1);
                vfov0Middle = filter.getDouble("vfov0", filter.animateIn);
                if (filter.animateIn > 0)
                vfov0Start = filter.getDouble("vfov0", 0);
                if (filter.animateOut > 0)
                vfov0End = filter.getDouble("vfov0", filter.duration - 1);
                vfov1Middle = filter.getDouble("vfov1", filter.animateIn);
                if (filter.animateIn > 0)
                vfov1Start = filter.getDouble("vfov1", 0);
                if (filter.animateOut > 0)
                vfov1End = filter.getDouble("vfov1", filter.duration - 1);
                if (filter.animateIn > 0)
                blurStartStart = filter.getDouble("blurStart", 0);
                if (filter.animateOut > 0)
                blurStartEnd = filter.getDouble("blurStart", filter.duration - 1);
                if (filter.animateIn > 0)
                blurEndStart = filter.getDouble("blurEnd", 0);
                if (filter.animateOut > 0)
                blurEndEnd = filter.getDouble("blurEnd", filter.duration - 1);
                setControls(null);
            }
        }
        Label {
            text: qsTr('Horizontal')
            Layout.alignment: Qt.AlignLeft
            Layout.columnSpan: 4
        }
        Label {
            text: qsTr('Start')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hfov0Slider
            minimumValue: -180
            maximumValue: 180
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_hfov0(getPosition())
        }
        Shotcut.UndoButton {
            id: hfov0Undo
            onClicked: hfov0Slider.value = 180
        }
        Shotcut.KeyframesButton {
            id: hfov0KeyframesButton
            onToggled: {
                var value = hfov0Slider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("hfov0");
                        hfov0Slider.enabled = true;
                    }
                    filter.clearSimpleAnimation("hfov0");
                    blockUpdate = false;
                    filter.set("hfov0", value, getPosition());
                } else {
                    filter.resetProperty("hfov0");
                    filter.set("hfov0", value);
                }
            }
        }
        Label {
            text: qsTr('End')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hfov1Slider
            minimumValue: -180
            maximumValue: 180
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_hfov1(getPosition())
        }
        Shotcut.UndoButton {
            id: hfov1Undo
            onClicked: hfov1Slider.value = 200
        }
        Shotcut.KeyframesButton {
            id: hfov1KeyframesButton
            onToggled: {
                var value = hfov1Slider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("hfov1");
                        hfov1Slider.enabled = true;
                    }
                    filter.clearSimpleAnimation("hfov1");
                    blockUpdate = false;
                    filter.set("hfov1", value, getPosition());
                } else {
                    filter.resetProperty("hfov1");
                    filter.set("hfov1", value);
                }
            }
        }
        Label {
            text: qsTr('Vertical')
            Layout.alignment: Qt.AlignLeft
            Layout.columnSpan: 4
        }
        Label {
            text: qsTr('Start')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: vfov0Slider
            minimumValue: -90
            maximumValue: 90
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_vfov0(getPosition())
        }
        Shotcut.UndoButton {
            id: vfov0Undo
            onClicked: vfov0Slider.value = 140
        }
        Shotcut.KeyframesButton {
            id: vfov0KeyframesButton
            onToggled: {
                var value = vfov0Slider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("vfov0");
                        vfov0Slider.enabled = true;
                    }
                    filter.clearSimpleAnimation("vfov0");
                    blockUpdate = false;
                    filter.set("vfov0", value, getPosition());
                } else {
                    filter.resetProperty("vfov0");
                    filter.set("vfov0", value);
                }
            }
        }
        Label {
            text: qsTr('End')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: vfov1Slider
            minimumValue: -90
            maximumValue: 90
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 1
            onValueChanged: updateProperty_vfov1(getPosition())
        }
        Shotcut.UndoButton {
            id: vfov1Undo
            onClicked: vfov1Slider.value = 160
        }
        Shotcut.KeyframesButton {
            id: vfov1KeyframesButton
            onToggled: {
                var value = vfov1Slider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("vfov1");
                        vfov1Slider.enabled = true;
                    }
                    filter.clearSimpleAnimation("vfov1");
                    blockUpdate = false;
                    filter.set("vfov1", value, getPosition());
                } else {
                    filter.resetProperty("vfov1");
                    filter.set("vfov1", value);
                }
            }
        }
        Label {
            text: qsTr('Blur Start')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: blurStartSlider
            minimumValue: 0
            maximumValue: 2.0
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 0.01
            onValueChanged: updateProperty_blurStart(getPosition())
        }
        Shotcut.UndoButton {
            id: blurStartUndo
            onClicked: blurStartSlider.value = 0.1
        }
        Shotcut.KeyframesButton {
            id: blurStartKeyframesButton
            onToggled: {
                var value = blurStartSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("blurStart");
                        blurStartSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("blurStart");
                    blockUpdate = false;
                    filter.set("blurStart", value, getPosition());
                } else {
                    filter.resetProperty("blurStart");
                    filter.set("blurStart", value);
                }
            }
        }
        Label {
            text: qsTr('End')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: blurEndSlider
            minimumValue: 0
            maximumValue: 2.0
            spinnerWidth: 120
            suffix: ' deg'
            decimals: 3
            stepSize: 0.01
            onValueChanged: updateProperty_blurEnd(getPosition())
        }
        Shotcut.UndoButton {
            id: blurEndUndo
            onClicked: blurEndSlider.value = 1.0
        }
        Shotcut.KeyframesButton {
            id: blurEndKeyframesButton
            onToggled: {
                var value = blurEndSlider.value;
                if (checked) {
                    blockUpdate = true;
                    if (filter.animateIn > 0 || filter.animateOut > 0) {
                        filter.resetProperty("blurEnd");
                        blurEndSlider.enabled = true;
                    }
                    filter.clearSimpleAnimation("blurEnd");
                    blockUpdate = false;
                    filter.set("blurEnd", value, getPosition());
                } else {
                    filter.resetProperty("blurEnd");
                    filter.set("blurEnd", value);
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
