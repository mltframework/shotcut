/*
 * Copyright (c) 2018 Meltytech, LLC
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

import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0

Item {
    property string rectProperty: "rect"
    property rect filterRect: filter.getRect(rectProperty)
    property var defaultParameters: [rectProperty, 'color.1', 'bgcolor', 'thickness', 'fill', 'window']

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
        var x = parseFloat(rectX.text)
        var y = parseFloat(rectY.text)
        var w = parseFloat(rectW.text)
        var h = parseFloat(rectH.text)
        if (x !== filterRect.x ||
            y !== filterRect.y ||
            w !== filterRect.width ||
            h !== filterRect.height) {
            filterRect.x = x
            filterRect.y = y
            filterRect.width = w
            filterRect.height = h
            filter.set(rectProperty, '%L1%/%L2%:%L3%x%L4%'
                       .arg(x / profile.width * 100)
                       .arg(y / profile.height * 100)
                       .arg(w / profile.width * 100)
                       .arg(h / profile.height * 100))
        }
    }

    function setControls() {
        fgColor.value = filter.get('color.1')
        bgColor.value = filter.get('bgcolor')
        thicknessSlider.value = filter.getDouble('thickness')
        fillCheckbox.checked = filter.get('fill') == 1
        combineCheckbox.checked = filter.get('show_channel') == -1
        windowSlider.value = filter.getDouble('window')
    }

    GridLayout {
        columns: 5
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: [rectProperty]
            Layout.columnSpan: 4
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Waveform Color')
            Layout.alignment: Qt.AlignRight
        }
        ColorPicker {
            Layout.columnSpan: 4
            id: fgColor
            eyedropper: true
            alpha: true
            onValueChanged: filter.set('color.1', value)
        }

        Label {
            text: qsTr('Background Color')
            Layout.alignment: Qt.AlignRight
        }
        ColorPicker {
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
        SliderSpinner {
            Layout.columnSpan: 3
            id: thicknessSlider
            minimumValue: 0
            maximumValue: 20
            decimals: 0
            suffix: ' px'
            onValueChanged: filter.set("thickness", value)
        }
        UndoButton {
            onClicked: thicknessSlider.value = 1
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            TextField {
                id: rectX
                text: filterRect.x
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
            Label { text: ',' }
            TextField {
                id: rectY
                text: filterRect.y
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            TextField {
                id: rectW
                text: filterRect.width
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
            Label { text: 'x' }
            TextField {
                id: rectH
                text: filterRect.height
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
        }
        
        Label {
            text: qsTr('Fill')
            Layout.alignment: Qt.AlignRight
        }
        CheckBox {
            Layout.columnSpan: 4
            id: fillCheckbox
            text: qsTr('Fill the area under the spectrum.')
            onClicked: filter.set('fill', checked ? 1 : 0)
        }

        Label {
            text: qsTr('Combine')
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
        SliderSpinner {
            Layout.columnSpan: 3
            id: windowSlider
            minimumValue: 0
            maximumValue: 1000
            suffix: ' ms'
            decimals: 0
            onValueChanged: filter.set("window", value)
        }
        UndoButton {
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
            if (filterRect !== newValue)
                filterRect = newValue
        }
    }
}
