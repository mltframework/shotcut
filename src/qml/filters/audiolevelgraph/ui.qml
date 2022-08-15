/*
 * Copyright (c) 2021-2022 Meltytech, LLC
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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    property string rectProperty: "rect"
    property rect filterRect: filter.getRect(rectProperty)
    property var defaultParameters: [rectProperty, 'type', 'color.1', 'color.2', 'color.3', 'color.4', 'color.5', 'color.6', 'color.7', 'color.8', 'color.9', 'color.10', 'bgcolor', 'thickness', 'fill', 'mirror', 'reverse', 'channels', 'segments', 'segment_gap']
    property bool _disableUpdate: true

    function setFilter() {
        var x = rectX.value;
        var y = rectY.value;
        var w = rectW.value;
        var h = rectH.value;
        if (x !== filterRect.x || y !== filterRect.y || w !== filterRect.width || h !== filterRect.height) {
            filterRect.x = x;
            filterRect.y = y;
            filterRect.width = w;
            filterRect.height = h;
            filter.set(rectProperty, filterRect);
        }
    }

    function setControls() {
        _disableUpdate = true;
        fgGradient.colors = filter.getGradient('color');
        bgColor.value = filter.get('bgcolor');
        thicknessSlider.value = filter.getDouble('thickness');
        mirrorCheckbox.checked = filter.get('mirror') == 1;
        reverseCheckbox.checked = filter.get('reverse') == 1;
        channelsSlider.value = filter.getDouble('channels');
        segmentsSlider.value = filter.getDouble('segments');
        segmentGapSlider.value = filter.getDouble('segment_gap');
        rectX.value = filterRect.x;
        rectY.value = filterRect.y;
        rectW.value = filterRect.width;
        rectH.value = filterRect.height;
        typeCombo.currentIndex = typeCombo.valueToIndex();
        segmentsSlider.enabled = segmentGapSlider.enabled = (typeCombo.currentIndex == 1);
        _disableUpdate = false;
    }

    width: 400
    height: 350
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(rectProperty, '0%/0%:10%x100%');
            filter.set('type', 'bar');
            filter.set('color.1', '#ffff0000');
            filter.set('color.2', '#ffffff00');
            filter.set('color.3', '#ff00ff00');
            filter.set('color.4', '#ff00ff00');
            filter.set('color.5', '#ff00ff00');
            filter.set('color.6', '#ff00ff00');
            filter.set('color.7', '#ff00ff00');
            filter.set('bgcolor', '#00ffffff');
            filter.set('thickness', '15');
            filter.set('mirror', '0');
            filter.set('reverse', '0');
            filter.set('channels', '2');
            filter.set('segments', '7');
            filter.set('segment_gap', '8');
            filter.savePreset(defaultParameters);
        }
        setControls();
    }

    GridLayout {
        columns: 5
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: defaultParameters
            Layout.columnSpan: 4
            onPresetSelected: setControls()
            onBeforePresetLoaded: {
                // Clear all gradient colors before loading the new values
                filter.setGradient('color', []);
            }
        }

        Label {
            text: qsTr('Type')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: typeCombo

            property var values: ['bar', 'segment']

            function valueToIndex() {
                var w = filter.get('type');
                for (var i = 0; i < values.length; ++i) if (values[i] === w) {
                    break;
                }
                if (i === values.length)
                    i = 0;

                return i;
            }

            Layout.columnSpan: 4
            model: [qsTr('Bar'), qsTr('Segment')]
            onActivated: {
                filter.set('type', values[index]);
                segmentsSlider.enabled = segmentGapSlider.enabled = (typeCombo.currentIndex == 1);
            }
        }

        Label {
            text: qsTr('Graph Colors')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.GradientControl {
            id: fgGradient

            Layout.columnSpan: 4
            onGradientChanged: {
                if (_disableUpdate)
                    return ;

                filter.setGradient('color', colors);
            }
        }

        Label {
            text: qsTr('Background Color')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ColorPicker {
            id: bgColor

            Layout.columnSpan: 4
            eyedropper: true
            alpha: true
            onValueChanged: filter.set('bgcolor', value)
        }

        Label {
            text: qsTr('Thickness')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: thicknessSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            suffix: ' px'
            onValueChanged: filter.set("thickness", value)

            Shotcut.HoverTip {
                text: 'Set the thickness of the bars (in pixels)'
            }

        }

        Shotcut.UndoButton {
            onClicked: thicknessSlider.value = 15
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Layout.columnSpan: 4

            Shotcut.DoubleSpinBox {
                id: rectX

                value: filterRect.x
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.x !== value)
                        setFilter();

                }
            }

            Label {
                text: ','
                Layout.minimumWidth: 20
                horizontalAlignment: Qt.AlignHCenter
            }

            Shotcut.DoubleSpinBox {
                id: rectY

                value: filterRect.y
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.y !== value)
                        setFilter();

                }
            }

        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Layout.columnSpan: 4

            Shotcut.DoubleSpinBox {
                id: rectW

                value: filterRect.width
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.width !== value)
                        setFilter();

                }
            }

            Label {
                text: 'x'
                Layout.minimumWidth: 20
                horizontalAlignment: Qt.AlignHCenter
            }

            Shotcut.DoubleSpinBox {
                id: rectH

                value: filterRect.height
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -1e+09
                to: 1e+09
                onValueModified: {
                    if (filterRect.height !== value)
                        setFilter();

                }
            }

        }

        Label {
            Layout.alignment: Qt.AlignRight
        }

        CheckBox {
            id: mirrorCheckbox

            Layout.columnSpan: 4
            text: qsTr('Mirror the levels.')
            onClicked: filter.set('mirror', checked ? 1 : 0)
        }

        Label {
            Layout.alignment: Qt.AlignRight
        }

        CheckBox {
            id: reverseCheckbox

            Layout.columnSpan: 4
            text: qsTr('Reverse the levels.')
            onClicked: filter.set('reverse', checked ? 1 : 0)

            Shotcut.HoverTip {
                text: 'Reverse the order of channels.'
            }

        }

        Label {
            text: qsTr('Channels')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: channelsSlider

            Layout.columnSpan: 3
            minimumValue: 1
            maximumValue: 10
            decimals: 0
            onValueChanged: filter.set("channels", value)

            Shotcut.HoverTip {
                text: 'The number of audio channels to show.'
            }

        }

        Shotcut.UndoButton {
            onClicked: channelsSlider.value = 2
        }

        Label {
            text: qsTr('Segments')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: segmentsSlider

            Layout.columnSpan: 3
            minimumValue: 2
            maximumValue: 100
            decimals: 0
            onValueChanged: filter.set("segments", value)

            Shotcut.HoverTip {
                text: 'The number of segments in the segment graph'
            }

        }

        Shotcut.UndoButton {
            onClicked: segmentGapSlider.value = 8
        }

        Label {
            text: qsTr('Segment Gap')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: segmentGapSlider

            Layout.columnSpan: 3
            minimumValue: 0
            maximumValue: 100
            decimals: 0
            onValueChanged: filter.set("segment_gap", value)

            Shotcut.HoverTip {
                text: 'Space between segments in the segment graph (in pixels)'
            }

        }

        Shotcut.UndoButton {
            onClicked: segmentGapSlider.value = 8
        }

        Item {
            Layout.fillHeight: true
        }

    }

    Connections {
        function onChanged() {
            var newValue = filter.getRect(rectProperty);
            if (filterRect !== newValue) {
                filterRect = newValue;
                setControls();
            }
        }

        target: filter
    }

}
