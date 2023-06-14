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
        thresholdSlider.value = filter.getDouble('discontinuity_threshold');
    }

    width: 480
    height: 325
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('discontinuity_threshold', 2);
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

        property int _prevSeamVal: 0
        property int _seamCountDown: 0

        interval: 200
        running: false
        repeat: true
        onTriggered: {
            if (filter.get('seam_count') != _prevSeamVal) {
                _prevSeamVal = filter.get('seam_count');
                _seamCountDown = 1000 / timer.interval;
                seamIndicator.active = true;
            } else if (_seamCountDown != 0) {
                _seamCountDown--;
            } else {
                seamIndicator.active = false;
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

            parameters: ['discontinuity_threshold']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Discontinuity Threshold')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The threshold to apply a seam to splices')
            }
        }

        Shotcut.SliderSpinner {
            id: thresholdSlider

            minimumValue: 0
            maximumValue: 30
            decimals: 1
            suffix: ' dB'
            spinnerWidth: 100
            value: filter.getDouble('discontinuity_threshold')
            onValueChanged: filter.set('discontinuity_threshold', value)
        }

        Shotcut.UndoButton {
            onClicked: thresholdSlider.value = -2
        }

        Label {
            text: qsTr('Seam Applied')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Status indicator showing when a splice has been seamed.')
            }
        }

        Rectangle {
            id: seamIndicator

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
