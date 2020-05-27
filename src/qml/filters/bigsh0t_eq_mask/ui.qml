

import QtQuick 2.1
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0


Item {
    width: 350
    height: 200
    property bool blockUpdate: true

    property double hfov0Start : 0.0; property double hfov0Middle : 0.0; property double hfov0End : 0.0;
    property double hfov1Start : 0.0; property double hfov1Middle : 0.0; property double hfov1End : 0.0;
    property double vfov0Start : 0.0; property double vfov0Middle : 0.0; property double vfov0End : 0.0;
    property double vfov1Start : 0.0; property double vfov1Middle : 0.0; property double vfov1End : 0.0;

    Connections { target: filter; onChanged: setControls(); onInChanged: { updateProperty_hfov0 (null); } onOutChanged: { updateProperty_hfov0 (null); } onAnimateInChanged: { updateProperty_hfov0 (null); } onAnimateOutChanged: { updateProperty_hfov0 (null); } }
    Connections { target: filter; onChanged: setControls(); onInChanged: { updateProperty_hfov1 (null); } onOutChanged: { updateProperty_hfov1 (null); } onAnimateInChanged: { updateProperty_hfov1 (null); } onAnimateOutChanged: { updateProperty_hfov1 (null); } }
    Connections { target: filter; onChanged: setControls(); onInChanged: { updateProperty_vfov0 (null); } onOutChanged: { updateProperty_vfov0 (null); } onAnimateInChanged: { updateProperty_vfov0 (null); } onAnimateOutChanged: { updateProperty_vfov0 (null); } }
    Connections { target: filter; onChanged: setControls(); onInChanged: { updateProperty_vfov1 (null); } onOutChanged: { updateProperty_vfov1 (null); } onAnimateInChanged: { updateProperty_vfov1 (null); } onAnimateOutChanged: { updateProperty_vfov1 (null); } }

    Component.onCompleted: {
        if (filter.isNew) { filter.set("hfov0", 180); } else { hfov0Middle = filter.getDouble("hfov0", filter.animateIn); if (filter.animateIn > 0) { hfov0Start = filter.getDouble("hfov0", 0); } if (filter.animateOut > 0) { hfov0End = filter.getDouble("hfov0", filter.duration - 1); } }
        if (filter.isNew) { filter.set("hfov1", 200); } else { hfov1Middle = filter.getDouble("hfov1", filter.animateIn); if (filter.animateIn > 0) { hfov1Start = filter.getDouble("hfov1", 0); } if (filter.animateOut > 0) { hfov1End = filter.getDouble("hfov1", filter.duration - 1); } }
        if (filter.isNew) { filter.set("vfov0", 140); } else { vfov0Middle = filter.getDouble("vfov0", filter.animateIn); if (filter.animateIn > 0) { vfov0Start = filter.getDouble("vfov0", 0); } if (filter.animateOut > 0) { vfov0End = filter.getDouble("vfov0", filter.duration - 1); } }
        if (filter.isNew) { filter.set("vfov1", 160); } else { vfov1Middle = filter.getDouble("vfov1", filter.animateIn); if (filter.animateIn > 0) { vfov1Start = filter.getDouble("vfov1", 0); } if (filter.animateOut > 0) { vfov1End = filter.getDouble("vfov1", filter.duration - 1); } }

        if (filter.isNew) {
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        hfov0Slider.value = filter.getDouble("hfov0", position)
        hfov1Slider.value = filter.getDouble("hfov1", position)
        vfov0Slider.value = filter.getDouble("vfov0", position)
        vfov1Slider.value = filter.getDouble("vfov1", position)
        blockUpdate = false
    }

    function updateProperty_hfov0 (position) { if (blockUpdate) return; var value = hfov0Slider.value; if (position !== null) { if (position <= 0 && filter.animateIn > 0) { hfov0Start = value; } else if (position >= filter.duration - 1 && filter.animateOut > 0) { hfov0End = value; } else { hfov0Middle = value; } } if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("hfov0"); hfov0KeyframesButton.checked = false; if (filter.animateIn > 0) { filter.set("hfov0", hfov0Start, 0); filter.set("hfov0", hfov0Middle, filter.animateIn - 1); } if (filter.animateOut > 0) { filter.set("hfov0", hfov0Middle, filter.duration - filter.animateOut); filter.set("hfov0", hfov0End, filter.duration - 1); } } else if (!hfov0KeyframesButton.checked) { filter.resetProperty("hfov0"); filter.set("hfov0", hfov0Middle); } else if (position !== null) { filter.set("hfov0", value, position); } }
    function updateProperty_hfov1 (position) { if (blockUpdate) return; var value = hfov1Slider.value; if (position !== null) { if (position <= 0 && filter.animateIn > 0) { hfov1Start = value; } else if (position >= filter.duration - 1 && filter.animateOut > 0) { hfov1End = value; } else { hfov1Middle = value; } } if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("hfov1"); hfov1KeyframesButton.checked = false; if (filter.animateIn > 0) { filter.set("hfov1", hfov1Start, 0); filter.set("hfov1", hfov1Middle, filter.animateIn - 1); } if (filter.animateOut > 0) { filter.set("hfov1", hfov1Middle, filter.duration - filter.animateOut); filter.set("hfov1", hfov1End, filter.duration - 1); } } else if (!hfov1KeyframesButton.checked) { filter.resetProperty("hfov1"); filter.set("hfov1", hfov1Middle); } else if (position !== null) { filter.set("hfov1", value, position); } }
    function updateProperty_vfov0 (position) { if (blockUpdate) return; var value = vfov0Slider.value; if (position !== null) { if (position <= 0 && filter.animateIn > 0) { vfov0Start = value; } else if (position >= filter.duration - 1 && filter.animateOut > 0) { vfov0End = value; } else { vfov0Middle = value; } } if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("vfov0"); vfov0KeyframesButton.checked = false; if (filter.animateIn > 0) { filter.set("vfov0", vfov0Start, 0); filter.set("vfov0", vfov0Middle, filter.animateIn - 1); } if (filter.animateOut > 0) { filter.set("vfov0", vfov0Middle, filter.duration - filter.animateOut); filter.set("vfov0", vfov0End, filter.duration - 1); } } else if (!vfov0KeyframesButton.checked) { filter.resetProperty("vfov0"); filter.set("vfov0", vfov0Middle); } else if (position !== null) { filter.set("vfov0", value, position); } }
    function updateProperty_vfov1 (position) { if (blockUpdate) return; var value = vfov1Slider.value; if (position !== null) { if (position <= 0 && filter.animateIn > 0) { vfov1Start = value; } else if (position >= filter.duration - 1 && filter.animateOut > 0) { vfov1End = value; } else { vfov1Middle = value; } } if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("vfov1"); vfov1KeyframesButton.checked = false; if (filter.animateIn > 0) { filter.set("vfov1", vfov1Start, 0); filter.set("vfov1", vfov1Middle, filter.animateIn - 1); } if (filter.animateOut > 0) { filter.set("vfov1", vfov1Middle, filter.duration - filter.animateOut); filter.set("vfov1", vfov1End, filter.duration - 1); } } else if (!vfov1KeyframesButton.checked) { filter.resetProperty("vfov1"); filter.set("vfov1", vfov1Middle); } else if (position !== null) { filter.set("vfov1", value, position); } }

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
        Preset {
            id: preset
            parameters: ["hfov0", "hfov1", "vfov0", "vfov1"]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty('hfov0')
                filter.resetProperty('hfov1')
                filter.resetProperty('vfov0')
                filter.resetProperty('vfov1')
            }
            onPresetSelected: {
                hfov0Middle = filter.getDouble("hfov0", filter.animateIn); if (filter.animateIn > 0) { hfov0Start = filter.getDouble("hfov0", 0); } if (filter.animateOut > 0) { hfov0End = filter.getDouble("hfov0", filter.duration - 1); }
                hfov1Middle = filter.getDouble("hfov1", filter.animateIn); if (filter.animateIn > 0) { hfov1Start = filter.getDouble("hfov1", 0); } if (filter.animateOut > 0) { hfov1End = filter.getDouble("hfov1", filter.duration - 1); }
                vfov0Middle = filter.getDouble("vfov0", filter.animateIn); if (filter.animateIn > 0) { vfov0Start = filter.getDouble("vfov0", 0); } if (filter.animateOut > 0) { vfov0End = filter.getDouble("vfov0", filter.duration - 1); }
                vfov1Middle = filter.getDouble("vfov1", filter.animateIn); if (filter.animateIn > 0) { vfov1Start = filter.getDouble("vfov1", 0); } if (filter.animateOut > 0) { vfov1End = filter.getDouble("vfov1", filter.duration - 1); }
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
        SliderSpinner {
            id: hfov0Slider
            minimumValue: 0
            maximumValue: 360
            spinnerWidth: 120; suffix: ' deg'; decimals: 3; stepSize: 1;
            onValueChanged: updateProperty_hfov0(getPosition())
        }
        KeyframesButton { id: hfov0KeyframesButton; checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("hfov0") > 0; onToggled: { var value = hfov0Slider.value; if (checked) { blockUpdate = true; if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("hfov0"); hfov0Slider.enabled = true; } filter.clearSimpleAnimation("hfov0"); blockUpdate = false; filter.set("hfov0", value, getPosition()); } else { filter.resetProperty("hfov0"); filter.set("hfov0", value); } } }
        UndoButton {
            id: hfov0Undo
            onClicked: hfov0Slider.value = 180
        }

        Label {
            text: qsTr('End')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: hfov1Slider
            minimumValue: 0
            maximumValue: 360
            spinnerWidth: 120; suffix: ' deg'; decimals: 3; stepSize: 1;
            onValueChanged: updateProperty_hfov1(getPosition())
        }
        KeyframesButton { id: hfov1KeyframesButton; checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("hfov1") > 0; onToggled: { var value = hfov1Slider.value; if (checked) { blockUpdate = true; if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("hfov1"); hfov1Slider.enabled = true; } filter.clearSimpleAnimation("hfov1"); blockUpdate = false; filter.set("hfov1", value, getPosition()); } else { filter.resetProperty("hfov1"); filter.set("hfov1", value); } } }
        UndoButton {
            id: hfov1Undo
            onClicked: hfov1Slider.value = 200
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
        SliderSpinner {
            id: vfov0Slider
            minimumValue: 0
            maximumValue: 360
            spinnerWidth: 120; suffix: ' deg'; decimals: 3; stepSize: 1;
            onValueChanged: updateProperty_vfov0(getPosition())
        }
        KeyframesButton { id: vfov0KeyframesButton; checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("vfov0") > 0; onToggled: { var value = vfov0Slider.value; if (checked) { blockUpdate = true; if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("vfov0"); vfov0Slider.enabled = true; } filter.clearSimpleAnimation("vfov0"); blockUpdate = false; filter.set("vfov0", value, getPosition()); } else { filter.resetProperty("vfov0"); filter.set("vfov0", value); } } }
        UndoButton {
            id: vfov0Undo
            onClicked: vfov0Slider.value = 140
        }

        Label {
            text: qsTr('End')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: vfov1Slider
            minimumValue: 0
            maximumValue: 360
            spinnerWidth: 120; suffix: ' deg'; decimals: 3; stepSize: 1;
            onValueChanged: updateProperty_vfov1(getPosition())
        }
        KeyframesButton { id: vfov1KeyframesButton; checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("vfov1") > 0; onToggled: { var value = vfov1Slider.value; if (checked) { blockUpdate = true; if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("vfov1"); vfov1Slider.enabled = true; } filter.clearSimpleAnimation("vfov1"); blockUpdate = false; filter.set("vfov1", value, getPosition()); } else { filter.resetProperty("vfov1"); filter.set("vfov1", value); } } }
        UndoButton {
            id: vfov1Undo
            onClicked: vfov1Slider.value = 160
        }
        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
