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
    function setControls() {
        durationSlider.value = filter.getDouble('fade_duration') / 1000.0;
        colorSwatch.value = filter.getColor('fade_color');
    }

    width: 480
    height: 125
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('fade_audio', 0);
            filter.set('fade_video', 1);
            filter.set('fade_duration', 500);
            filter.set('fade_color', Qt.rgba(1, 1, 1, 1));
            filter.savePreset(preset.parameters, qsTr('Fade to White'));
            // Set default parameter values
            filter.set('fade_duration', 500);
            filter.set('fade_color', Qt.rgba(0, 0, 0, 1));
            filter.savePreset(preset.parameters);
        }
        setControls();
        timer.start();
    }

    SystemPalette {
        id: activePalette
    }

    Timer {
        id: timer

        property int _prevFadeInCount: 0
        property int _prevFadeOutCount: 0

        interval: 200
        running: false
        repeat: true
        onTriggered: {
            if (filter.get('fade_in_count') != _prevFadeInCount) {
                _prevFadeInCount = filter.get('fade_in_count');
                fadeInIndicator.active = true;
            } else {
                fadeInIndicator.active = false;
            }
            if (filter.get('fade_out_count') != _prevFadeOutCount) {
                _prevFadeOutCount = filter.get('fade_out_count');
                fadeOutIndicator.active = true;
            } else {
                fadeOutIndicator.active = false;
            }
        }
    }

    GridLayout {
        id: grid

        anchors.fill: parent
        anchors.margins: 8
        columns: 3
        columnSpacing: 8
        rowSpacing: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: ['fade_duration', 'fade_color']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Fade duration')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The duration of fade to apply at the begining and end of each clip')
            }
        }

        Shotcut.SliderSpinner {
            id: durationSlider

            minimumValue: 0.01
            maximumValue: 1.0
            decimals: 2
            suffix: ' s'
            spinnerWidth: 100
            value: filter.getDouble('fade_duration') / 1000.0
            onValueChanged: filter.set('fade_duration', value * 1000.0)
        }

        Shotcut.UndoButton {
            onClicked: durationSlider.value = -2
        }

        Label {
            text: qsTr('Fade color')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.ColorPicker {
                id: colorSwatch

                property bool isReady: false

                alpha: true
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        filter.set("fade_color", Qt.color(value));
                    }
                }
            }

            Shotcut.Button {
                text: qsTr('Transparent')
                onClicked: colorSwatch.value = Qt.rgba(0, 0, 0, 0)
            }
        }

        Shotcut.UndoButton {
            onClicked: colorSwatch.value = Qt.rgba(0, 0, 0, 1)
        }

        Rectangle {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.minimumHeight: 12
            color: 'transparent'

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 2
                radius: 2
                color: activePalette.text
            }
        }

        Label {
            text: qsTr('Fade in')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Status indicator showing when a fade in has occured.')
            }
        }

        Rectangle {
            id: fadeInIndicator

            property bool active: false

            Layout.columnSpan: 2
            width: 20
            height: width
            radius: 10
            color: activePalette.alternateBase

            Rectangle {
                x: 3
                y: 3
                width: 14
                height: width
                radius: 7
                color: parent.active ? activePalette.highlight : activePalette.base
            }
        }

        Label {
            text: qsTr('Fade out')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Status indicator showing when a fade out has occured.')
            }
        }

        Rectangle {
            id: fadeOutIndicator

            property bool active: false

            Layout.columnSpan: 2
            width: 20
            height: width
            radius: 10
            color: activePalette.alternateBase

            Rectangle {
                x: 3
                y: 3
                width: 14
                height: width
                radius: 7
                color: parent.active ? activePalette.highlight : activePalette.base
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
