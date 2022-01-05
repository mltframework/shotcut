/*
 * Copyright (c) 2018-2022 Meltytech, LLC
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
    property var defaultParameters: [rectProperty, 'color.1', 'color.2', 'color.3', 'color.4', 'color.5', 'color.6', 'color.7', 'color.8', 'color.9', 'color.10', 'bgcolor', 'thickness', 'fill', 'show_channel', 'window']
    property bool _disableUpdate: true

    width: 350
    height: 425

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(rectProperty, '0/50%:50%x50%')
            filter.set('color.1', '#ffffffff')
            filter.set('bgcolor', '#00ffffff')
            filter.set('thickness', '1')
            filter.set('fill', '0')
            filter.set('show_channel', '-1')
            filter.set('window', '0')
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
        fillCheckbox.checked = filter.get('fill') == 1
        combineCheckbox.checked = filter.get('show_channel') == -1
        windowSlider.value = filter.getDouble('window')
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
            text: qsTr('Waveform Color')
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
                onValueModified: setFilter()
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
                onValueModified: setFilter()
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
                onValueModified: setFilter()
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
                onValueModified: setFilter()
            }
        }
        
        Label {
            Layout.alignment: Qt.AlignRight
        }
        CheckBox {
            Layout.columnSpan: 4
            id: fillCheckbox
            text: qsTr('Fill the area under the waveform.')
            onClicked: filter.set('fill', checked ? 1 : 0)
        }

        Label {
            Layout.alignment: Qt.AlignRight
        }
        CheckBox {
            Layout.columnSpan: 4
            id: combineCheckbox
            text: qsTr('Combine all channels into one waveform.')
            onClicked: filter.set('show_channel', checked ? -1 : 0)
        }

        Label {
            text: qsTr('Window')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            Layout.columnSpan: 3
            id: windowSlider
            minimumValue: 0
            maximumValue: 1000
            suffix: ' ms'
            decimals: 0
            onValueChanged: filter.set("window", value)
        }
        Shotcut.UndoButton {
            onClicked: windowSlider.value = 0.4
        }

        Item {
            Layout.fillHeight: true
        }
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
