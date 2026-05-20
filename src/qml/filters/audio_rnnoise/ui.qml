// Copyright (C) 2026 Meltytech, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

Item {
    property string mixProperty: 'mix'
    property bool blockUpdate: true

    width: 350
    height: 100

    function setControls() {
        blockUpdate = true;
        sliderMix.value = filter.getDouble(mixProperty) * sliderMix.maximumValue;
        blockUpdate = false;
    }

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(mixProperty, 1.0);
            filter.savePreset(preset.parameters);
        }
        setControls();
    }

    Connections {
        function onChanged() { setControls(); }
        target: filter
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 3

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: [mixProperty]
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Suppression')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The amount of noise suppression to apply. 100% outputs the fully denoised signal. Reduce this value to blend in the original audio if the denoised result sounds too processed or artefact-ridden.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderMix

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(mixProperty) * maximumValue
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set(mixProperty, value / maximumValue);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderMix.value = 100
        }

        Item {
            Layout.columnSpan: 3
            Layout.fillHeight: true
        }
    }
}
