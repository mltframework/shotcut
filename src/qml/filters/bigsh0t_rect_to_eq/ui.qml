

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut


Item {
    width: 350
    height: 130
    property bool blockUpdate: true

    property double hfovStart : 0.0; property double hfovMiddle : 0.0; property double hfovEnd : 0.0;
    property double vfovStart : 0.0; property double vfovMiddle : 0.0; property double vfovEnd : 0.0;
    property int interpolationValue : 0;

    function updateSimpleKeyframes() {
        if (filter.animateIn <= 0 && filter.animateOut <= 0) {
            // When disabling simple keyframes. Clear out the keyframes before proceeding.
            filter.blockSignals = true
            if (!hfovKeyframesButton.checked && filter.keyframeCount("hfov") > 0) {
                filter.resetProperty("hfov")
            }
            if (!vfovKeyframesButton.checked && filter.keyframeCount("vfov") > 0) {
                filter.resetProperty("vfov")
            }
            filter.blockSignals = false
        } else if (filter.animateIn > 0 || filter.animateOut > 0) {
            // When enabling simple keyframes, initialize the keyframes with the current value
            if (filter.keyframeCount("hfov") <= 0) {
                hfovStart = hfovMiddle = hfovEnd = filter.getDouble("hfov")
            }
            if (filter.keyframeCount("vfov") <= 0) {
                vfovStart = vfovMiddle = vfovEnd = filter.getDouble("vfov")
            }
        }
        updateProperty_hfov(null)
        updateProperty_vfov(null)
        updateProperty_interpolation()
    }

    Connections {
        target: filter
        onChanged: setControls()
        onInChanged: updateSimpleKeyframes()
        onOutChanged: updateSimpleKeyframes()
        onAnimateInChanged: updateSimpleKeyframes()
        onAnimateOutChanged: updateSimpleKeyframes()
    }

    Component.onCompleted: {
        if (filter.isNew) { filter.set("hfov", 90); } else { hfovMiddle = filter.getDouble("hfov", filter.animateIn); if (filter.animateIn > 0) { hfovStart = filter.getDouble("hfov", 0); } if (filter.animateOut > 0) { hfovEnd = filter.getDouble("hfov", filter.duration - 1); } }
        if (filter.isNew) { filter.set("vfov", 60); } else { vfovMiddle = filter.getDouble("vfov", filter.animateIn); if (filter.animateIn > 0) { vfovStart = filter.getDouble("vfov", 0); } if (filter.animateOut > 0) { vfovEnd = filter.getDouble("vfov", filter.duration - 1); } }
        if (filter.isNew) { filter.set("interpolation", 1); } else { interpolationValue = filter.get("interpolation"); }

        if (filter.isNew) {
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        hfovSlider.value = filter.getDouble("hfov", position)
        hfovKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("hfov") > 0
        vfovSlider.value = filter.getDouble("vfov", position)
        VfovKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("vfov") > 0
        interpolationComboBox.currentIndex = filter.get("interpolation")
        blockUpdate = false
    }

    function updateProperty_hfov (position) { if (blockUpdate) return; var value = hfovSlider.value; if (position !== null) { if (position <= 0 && filter.animateIn > 0) { hfovStart = value; } else if (position >= filter.duration - 1 && filter.animateOut > 0) { hfovEnd = value; } else { hfovMiddle = value; } } if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("hfov"); hfovKeyframesButton.checked = false; if (filter.animateIn > 0) { filter.set("hfov", hfovStart, 0); filter.set("hfov", hfovMiddle, filter.animateIn - 1); } if (filter.animateOut > 0) { filter.set("hfov", hfovMiddle, filter.duration - filter.animateOut); filter.set("hfov", hfovEnd, filter.duration - 1); } } else if (!hfovKeyframesButton.checked) { filter.resetProperty("hfov"); filter.set("hfov", hfovMiddle); } else if (position !== null) { filter.set("hfov", value, position); } }
    function updateProperty_vfov (position) { if (blockUpdate) return; var value = vfovSlider.value; if (position !== null) { if (position <= 0 && filter.animateIn > 0) { vfovStart = value; } else if (position >= filter.duration - 1 && filter.animateOut > 0) { vfovEnd = value; } else { vfovMiddle = value; } } if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("vfov"); vfovKeyframesButton.checked = false; if (filter.animateIn > 0) { filter.set("vfov", vfovStart, 0); filter.set("vfov", vfovMiddle, filter.animateIn - 1); } if (filter.animateOut > 0) { filter.set("vfov", vfovMiddle, filter.duration - filter.animateOut); filter.set("vfov", vfovEnd, filter.duration - 1); } } else if (!vfovKeyframesButton.checked) { filter.resetProperty("vfov"); filter.set("vfov", vfovMiddle); } else if (position !== null) { filter.set("vfov", value, position); } }
    function updateProperty_interpolation () { if (blockUpdate) return; var value = interpolationComboBox.currentIndex; filter.set("interpolation", value); }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
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
            parameters: ["hfov", "vfov", "interpolation"]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty('hfov')
                filter.resetProperty('vfov')
                filter.resetProperty('interpolation')
            }
            onPresetSelected: {
                hfovMiddle = filter.getDouble("hfov", filter.animateIn); if (filter.animateIn > 0) { hfovStart = filter.getDouble("hfov", 0); } if (filter.animateOut > 0) { hfovEnd = filter.getDouble("hfov", filter.duration - 1); }
                vfovMiddle = filter.getDouble("vfov", filter.animateIn); if (filter.animateIn > 0) { vfovStart = filter.getDouble("vfov", 0); } if (filter.animateOut > 0) { vfovEnd = filter.getDouble("vfov", filter.duration - 1); }
                interpolationValue = filter.get("interpolation");
                setControls(null);
            }
        }

        Label {
            text: qsTr('Interpolation')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            currentIndex: 0
            model: ["Nearest-neighbor", "Bilinear"]
            id: interpolationComboBox
            onCurrentIndexChanged: updateProperty_interpolation()
        }
        Shotcut.UndoButton {
            id: interpolationUndo
            onClicked: interpolationComboBox.currentIndex = 0
        }
        Item { Layout.fillWidth: true }

        Label {
            text: qsTr('Horizontal')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: hfovSlider
            minimumValue: 0
            maximumValue: 180
            spinnerWidth: 120; suffix: ' deg'; decimals: 3; stepSize: 1;
            onValueChanged: updateProperty_hfov(getPosition())
        }
        Shotcut.UndoButton {
            id: hfovUndo
            onClicked: hfovSlider.value = 90
        }
        Shotcut.KeyframesButton { id: hfovKeyframesButton; onToggled: { var value = hfovSlider.value; if (checked) { blockUpdate = true; if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("hfov"); hfovSlider.enabled = true; } filter.clearSimpleAnimation("hfov"); blockUpdate = false; filter.set("hfov", value, getPosition()); } else { filter.resetProperty("hfov"); filter.set("hfov", value); } } }

        Label {
            text: qsTr('Vertical')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: vfovSlider
            minimumValue: 0
            maximumValue: 180
            spinnerWidth: 120; suffix: ' deg'; decimals: 3; stepSize: 1;
            onValueChanged: updateProperty_vfov(getPosition())
        }
        Shotcut.UndoButton {
            id: vfovUndo
            onClicked: vfovSlider.value = 60
        }
        Shotcut.KeyframesButton { id: vfovKeyframesButton; onToggled: { var value = vfovSlider.value; if (checked) { blockUpdate = true; if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("vfov"); vfovSlider.enabled = true; } filter.clearSimpleAnimation("vfov"); blockUpdate = false; filter.set("vfov", value, getPosition()); } else { filter.resetProperty("vfov"); filter.set("vfov", value); } } }
        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        onPropertyChanged: setControls()
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
