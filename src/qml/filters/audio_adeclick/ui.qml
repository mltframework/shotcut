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
    property bool blockControls: false

    function setControls() {
        if (blockControls)
            return;
        sliderWindow.value = filter.getDouble('av.window');
        sliderThreshold.value = filter.getDouble('av.threshold');
    }

    width: 200
    height: 130
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('av.window', 50);
            filter.set('av.overlap', 75);
            filter.set('av.aorder', 2);
            filter.set('av.threshold', 2);
            filter.set('av.burst', 2);
            filter.set('av.method', 'add');
            filter.savePreset(preset.parameters);
        }
        setControls();
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

            parameters: ['av.window', 'av.overlap', 'av.order', 'av.threshold', 'av.burst', 'av.method']
            Layout.columnSpan: 2
            onPresetSelected: {
                setControls();
            }
        }

        Label {
            text: qsTr('Window')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr('The size of the window, in milliseconds, which will be processed at once.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderWindow

            minimumValue: 10
            maximumValue: 100
            stepSize: 1
            decimals: 0
            suffix: ' ms'
            onValueChanged: {
                if (blockControls)
                    return;
                blockControls = true;
                filter.set('av.window', value);
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderWindow.value = 50
        }

        Label {
            text: qsTr('Threshold')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr('The strength of impulsive noise which is going to be removed. The lower value, the more samples will be detected as impulsive noise.')
            }
        }

        Shotcut.SliderSpinner {
            id: sliderThreshold

            minimumValue: 2
            maximumValue: 100
            decimals: 0
            stepSize: 1
            onValueChanged: {
                if (blockControls)
                    return;
                blockControls = true;
                filter.set('av.threshold', value);
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderThreshold.value = 2
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
            setControls();
        }

        function onOutChanged() {
            setControls();
        }

        function onPropertyChanged(name) {
            setControls();
        }

        target: filter
    }
}
