

import QtQuick 2.1
import QtQuick.Controls 1.4
import QtQuick.Controls 2.12 as Controls2
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0


Item {
    width: 350
    height: 130
    property bool blockUpdate: true

    property double hfovStart : 0.0; property double hfovMiddle : 0.0; property double hfovEnd : 0.0;
    property double vfovStart : 0.0; property double vfovMiddle : 0.0; property double vfovEnd : 0.0;
    property int interpolationValue : 0;

    Connections { target: filter; onChanged: setControls(); onInChanged: { updateProperty_hfov (null); } onOutChanged: { updateProperty_hfov (null); } onAnimateInChanged: { updateProperty_hfov (null); } onAnimateOutChanged: { updateProperty_hfov (null); } }
    Connections { target: filter; onChanged: setControls(); onInChanged: { updateProperty_vfov (null); } onOutChanged: { updateProperty_vfov (null); } onAnimateInChanged: { updateProperty_vfov (null); } onAnimateOutChanged: { updateProperty_vfov (null); } }
    Connections { target: filter; onChanged: setControls(); onInChanged: { updateProperty_interpolation (); } onOutChanged: { updateProperty_interpolation (); } onAnimateInChanged: { updateProperty_interpolation (); } onAnimateOutChanged: { updateProperty_interpolation (); } }

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
        vfovSlider.value = filter.getDouble("vfov", position)
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
        Preset {
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
        Controls2.ComboBox {
            currentIndex: 0
            model: ["Nearest-neighbor", "Bilinear"]
            id: interpolationComboBox
            Layout.columnSpan: 2
            onCurrentIndexChanged: updateProperty_interpolation()
        }
        UndoButton {
            id: interpolationUndo
            onClicked: interpolationComboBox.currentIndex = 0
        }

        Label {
            text: qsTr('Horizontal')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: hfovSlider
            minimumValue: 0
            maximumValue: 180
            spinnerWidth: 120; suffix: ' deg'; decimals: 3; stepSize: 1;
            onValueChanged: updateProperty_hfov(getPosition())
        }
        KeyframesButton { id: hfovKeyframesButton; checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("hfov") > 0; onToggled: { var value = hfovSlider.value; if (checked) { blockUpdate = true; if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("hfov"); hfovSlider.enabled = true; } filter.clearSimpleAnimation("hfov"); blockUpdate = false; filter.set("hfov", value, getPosition()); } else { filter.resetProperty("hfov"); filter.set("hfov", value); } } }
        UndoButton {
            id: hfovUndo
            onClicked: hfovSlider.value = 90
        }

        Label {
            text: qsTr('Vertical')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: vfovSlider
            minimumValue: 0
            maximumValue: 180
            spinnerWidth: 120; suffix: ' deg'; decimals: 3; stepSize: 1;
            onValueChanged: updateProperty_vfov(getPosition())
        }
        KeyframesButton { id: vfovKeyframesButton; checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount("vfov") > 0; onToggled: { var value = vfovSlider.value; if (checked) { blockUpdate = true; if (filter.animateIn > 0 || filter.animateOut > 0) { filter.resetProperty("vfov"); vfovSlider.enabled = true; } filter.clearSimpleAnimation("vfov"); blockUpdate = false; filter.set("vfov", value, getPosition()); } else { filter.resetProperty("vfov"); filter.set("vfov", value); } } }
        UndoButton {
            id: vfovUndo
            onClicked: vfovSlider.value = 60
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
