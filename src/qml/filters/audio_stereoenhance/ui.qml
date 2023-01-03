/*
 * Copyright (c) 2022 Meltytech, LLC
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

    function toDb(value) {
        return Math.round(20 * Math.log10(value) * 100) / 100;
    }

    function fromDb(value) {
        return Math.pow(10, value / 20);
    }

    function setControls() {
        if (blockControls)
            return;
        sliderLeftDelay.value = filter.getDouble('av.left_delay');
        sliderLeftLevel.value = toDb(filter.getDouble('av.left_gain'));
        sliderRightDelay.value = filter.getDouble('av.right_delay');
        sliderRightLevel.value = toDb(filter.getDouble('av.right_gain'));
        sliderOutputLevel.value = toDb(filter.getDouble('av.level_out'));
        sourceCombo.currentIndex = sourceCombo.valueToIndex();
    }

    width: 200
    height: 130
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('av.level_in', 1);
            filter.set('av.level_out', fromDb(-3));
            filter.set('av.side_gain', 1);
            filter.set('av.middle_source', 'mid');
            filter.set('av.middle_phase', 0);
            filter.set('av.left_delay', 0);
            filter.set('av.left_balance', 1);
            filter.set('av.left_gain', 1);
            filter.set('av.left_phase', 1);
            filter.set('av.right_delay', 10);
            filter.set('av.right_balance', -1);
            filter.set('av.right_gain', 1);
            filter.set('av.right_phase', 1);
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

            parameters: ['av.level_in', 'av.level_out', 'av.side_gain', 'av.middle_source', 'av.middle_phase', 'av.left_delay', 'av.left_balance', 'av.left_gain', 'av.left_phase', 'av.right_delay', 'av.right_balance', 'av.right_gain', 'av.right_phase']
            Layout.columnSpan: 2
            onPresetSelected: {
                setControls();
            }
        }

        Label {
            text: qsTr('Source')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: sourceCombo

            property var values: ['left', 'right', 'mid', 'side']

            function valueToIndex() {
                var w = filter.get('av.middle_source');
                for (var i = 0; i < values.length; ++i)
                    if (values[i] === w) {
                        break;
                    }
                if (i === values.length)
                    i = 0;
                return i;
            }

            Layout.columnSpan: 2
            model: [qsTr('Left'), qsTr('Right'), qsTr('Middle (L+R)'), qsTr('Side (L-R)')]
            onActivated: {
                blockControls = true;
                filter.set('av.middle_source', values[index]);
                blockControls = false;
            }
        }

        Label {
            text: qsTr('Left delay')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderLeftDelay

            minimumValue: 0
            maximumValue: 40
            stepSize: 0.1
            decimals: 1
            suffix: ' ms'
            onValueChanged: {
                if (blockControls)
                    return;
                blockControls = true;
                filter.set('av.left_delay', value);
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderLeftDelay.value = 0
        }

        Label {
            text: qsTr('Left delay gain')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderLeftLevel

            minimumValue: -16
            maximumValue: 16
            suffix: ' dB'
            decimals: 1
            stepSize: 0.1
            onValueChanged: {
                if (blockControls)
                    return;
                blockControls = true;
                filter.set('av.left_gain', fromDb(value));
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderLeftLevel.value = 0
        }

        Label {
            text: qsTr('Right delay')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderRightDelay

            minimumValue: 0
            maximumValue: 40
            stepSize: 0.1
            decimals: 1
            suffix: ' ms'
            onValueChanged: {
                if (blockControls)
                    return;
                blockControls = true;
                filter.set('av.right_delay', value);
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderRightDelay.value = 10
        }

        Label {
            text: qsTr('Right delay gain')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderRightLevel

            minimumValue: -16
            maximumValue: 16
            suffix: ' dB'
            decimals: 1
            stepSize: 0.1
            onValueChanged: {
                if (blockControls)
                    return;
                blockControls = true;
                filter.set('av.right_gain', fromDb(value));
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderRightLevel.value = 0
        }

        Label {
            text: qsTr('Output gain')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderOutputLevel

            minimumValue: -16
            maximumValue: 16
            suffix: ' dB'
            decimals: 1
            stepSize: 0.1
            onValueChanged: {
                if (blockControls)
                    return;
                blockControls = true;
                filter.set('av.level_out', fromDb(value));
                blockControls = false;
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderOutputLevel.value = -3
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
