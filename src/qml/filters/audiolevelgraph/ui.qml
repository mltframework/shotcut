/*
 * Copyright (c) 2021 Meltytech, LLC
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
    property var defaultParameters: [rectProperty, 'type', 'color.1', 'color.2', 'color.3', 'color.4', 'color.5', 'color.6', 'color.7', 'color.8', 'color.9', 'color.10', 'bgcolor', 'thickness', 'fill', 'mirror', 'reverse', 'tension', 'bands', 'frequency_low', 'frequency_high', 'threshold']

    property int _minFreqDelta: 1000
    property bool _disableUpdate: true

    width: 350
    height: 425

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(rectProperty, '0%/0%:10%x100%')
            filter.set('color.1', '#ffff0000')
            filter.set('color.2', '#ffffff00')
            filter.set('color.3', '#ff00ff00')
            filter.set('color.4', '#ff00ff00')
            filter.set('color.5', '#ff00ff00')
            filter.set('color.6', '#ff00ff00')
            filter.set('color.7', '#ff00ff00')
            filter.set('bgcolor', '#00ffffff')
            filter.set('thickness', '15')
            filter.set('mirror', '0')
            filter.set('reverse', '0')
            filter.set('channels', '2')
            filter.savePreset(defaultParameters)
        }
        setControls()
    }

    function setFilter() {
        var x = rectX.value
        var y = rectY.value
        var w = rectW.value
        var h = rectH.value
        if (x !== filterRect.x ||
            y !== filterRect.y ||
            w !== filterRect.width ||
            h !== filterRect.height) {
            filterRect.x = x
            filterRect.y = y
            filterRect.width = w
            filterRect.height = h
            filter.set(rectProperty, filterRect)
        }
    }

    function setControls() {
        _disableUpdate = true
        fgGradient.colors = filter.getGradient('color')
        bgColor.value = filter.get('bgcolor')
        thicknessSlider.value = filter.getDouble('thickness')
        mirrorCheckbox.checked = filter.get('mirror') == 1
        reverseCheckbox.checked = filter.get('reverse') == 1
        channelsSlider.value = filter.getDouble('channels')
        rectX.value = filterRect.x
        rectY.value = filterRect.y
        rectW.value = filterRect.width
        rectH.value = filterRect.height
        _disableUpdate = false
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
                filter.setGradient('color', [])
            }
        }

        Label {
            text: qsTr('Bar Color')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.GradientControl {
            Layout.columnSpan: 4
            id: fgGradient
            onGradientChanged: {
                 if (_disableUpdate) return
                 filter.setGradient('color', colors)
            }
        }

        Label {
            text: qsTr('Background Color')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ColorPicker {
            Layout.columnSpan: 4
            id: bgColor
            eyedropper: true
            alpha: true
            onValueChanged: filter.set('bgcolor', value)
        }

        Label {
            text: qsTr('Thickness')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            Layout.columnSpan: 3
            id: thicknessSlider
            minimumValue: 0
            maximumValue: 20
            decimals: 0
            suffix: ' px'
            onValueChanged: filter.set("thickness", value)
        }
        Shotcut.UndoButton {
            onClicked: thicknessSlider.value = 1
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
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.x !== value) setFilter()
            }
            Label { text: ','; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
            Shotcut.DoubleSpinBox {
                id: rectY
                value: filterRect.y
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.y !== value) setFilter()
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
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.width !== value) setFilter()
            }
            Label { text: 'x'; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
            Shotcut.DoubleSpinBox {
                id: rectH
                value: filterRect.height
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.height !== value) setFilter()
            }
        }

        Label {
            text: qsTr('Mirror')
            Layout.alignment: Qt.AlignRight
        }
        CheckBox {
            Layout.columnSpan: 4
            id: mirrorCheckbox
            text: qsTr('Mirror the spectrum.')
            onClicked: filter.set('mirror', checked ? 1 : 0)
        }

        Label {
            text: qsTr('Reverse')
            Layout.alignment: Qt.AlignRight
        }
        CheckBox {
            Layout.columnSpan: 4
            id: reverseCheckbox
            text: qsTr('Reverse the spectrum.')
            onClicked: filter.set('reverse', checked ? 1 : 0)
        }

        Label {
            text: qsTr('Channels')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            Layout.columnSpan: 3
            id: channelsSlider
            minimumValue: 1
            maximumValue: 10
            decimals: 0
            onValueChanged: filter.set("channels", value)
        }
        Shotcut.UndoButton {
            onClicked: bandsSlider.value = 2
        }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        onChanged: {
            var newValue = filter.getRect(rectProperty)
            if (filterRect !== newValue) {
                filterRect = newValue
                setControls()
            }
        }
    }
}
