/*
 * Copyright (c) 2023 Meltytech, LLC
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
    id: root
    property bool blockUpdate: true
    property double min: -10
    property double max: 10

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        if (blockUpdate)
            return;
        var outPosition = getPosition();
        blockUpdate = true;
        speedSpinner.value = filter.getDouble('speed_map', outPosition);
        var current = filter.get('image_mode');
        for (var i = 0; i < imageModeModel.count; ++i) {
            if (imageModeModel.get(i).value === current) {
                modeCombo.currentIndex = i;
                break;
            }
        }
        pitchCheckbox.checked = filter.get('pitch') === '1';
        var speed = filter.getDouble('speed');
        blockUpdate = false;
    }

    width: 200
    height: 225
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('speed_map', 1.0, 0);
            filter.set('speed_map', 1.0, filter.duration - 1);
            filter.set('image_mode', 'nearest');
            filter.set('pitch', 0);
            filter.savePreset(preset.parameters);
            application.showStatusMessage(qsTr('Hold %1 to drag a keyframe vertical only or %2 to drag horizontal only').arg(application.OS === 'macOS' ? '⌘' : 'Ctrl').arg(application.OS === 'macOS' ? '⌥' : 'Alt'));
        }
        blockUpdate = false;
        timer.restart();
    }

    Connections {
        function onInChanged() {
            timer.restart();
        }

        function onOutChanged() {
            timer.restart();
        }

        function onPropertyChanged() {
            timer.restart();
        }

        target: filter
    }

    Timer {
        id: timer

        interval: 200
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            setControls();
        }
    }

    Connections {
        function onPositionChanged() {
            timer.restart();
        }

        target: producer
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset
            Layout.columnSpan: parent.columns - 1
            parameters: ['speed_map', 'image_mode', 'pitch']
            onBeforePresetLoaded: {
                filter.resetProperty(parameters[0]);
            }
            onPresetSelected: {
                timer.restart();
                mapKeyframesButton.checked = filter.keyframeCount(parameters[0]) > 0;
            }
        }

        Label {
            text: qsTr('Speed')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Map the specified speed to the current time. Use keyframes to vary the speed mappings over time.')
            }
        }

        Shotcut.SliderSpinner {
            id: speedSpinner
            minimumValue: root.min
            maximumValue: root.max
            decimals: 2
            suffix: ' x'
            onValueChanged: {
                if (blockUpdate)
                    return;
                filter.set('speed_map', speedSpinner.value, getPosition());
                timer.restart();
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                speedSpinner.value = 1.0;
            }
        }

        Label {
            text: qsTr('Image mode')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Use the specified image selection mode. Nearest will output the image that is nearest to the mapped time. Blend will blend all images that occur during the mapped time.')
            }
        }

        Shotcut.ComboBox {
            id: modeCombo
            Layout.columnSpan: parent.columns - 2
            implicitWidth: 180
            textRole: "text"
            onCurrentIndexChanged: {
                if (blockUpdate)
                    return;
                filter.set('image_mode', imageModeModel.get(currentIndex).value);
            }

            model: ListModel {
                id: imageModeModel

                ListElement {
                    text: qsTr('Nearest')
                    value: 'nearest'
                }

                ListElement {
                    text: qsTr('Blend')
                    value: 'blend'
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: modeCombo.currentIndex = 0
        }

        Label {
        }

        CheckBox {
            id: pitchCheckbox
            Layout.columnSpan: parent.columns - 2
            text: qsTr('Enable pitch compensation')
            onCheckedChanged: {
                if (blockUpdate)
                    return;
                filter.set('pitch', checked);
            }
        }

        Shotcut.UndoButton {
            onClicked: pitchCheckbox.checked = false
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
