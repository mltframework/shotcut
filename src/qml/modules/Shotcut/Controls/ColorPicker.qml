/*
 * Copyright (c) 2014-2020 Meltytech, LLC
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

import QtQuick 2.2
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0 as Shotcut

RowLayout {
    property string value: "white"
    property bool alpha: false
    property alias eyedropper: pickerButton.visible
    
    signal pickStarted
    signal pickCancelled
    
    SystemPalette { id: activePalette; colorGroup: SystemPalette.Active }
    
    Shotcut.ColorPickerItem {
        id: pickerItem
        onColorPicked: {
            value = color
            pickerButton.checked = false
        }
        onCancelled: pickCancelled()
    }
    
    Shotcut.Button {
        id: colorButton
        implicitWidth: 20
        implicitHeight: 20
        background: Rectangle {
            border.width: 1
            border.color: 'gray'
            radius: pickerButton.background.radius
            color: value
        }
        Shotcut.HoverTip { text: qsTr('Click to open color dialog') }
        onClicked: colorDialog.visible = true
    }
    
    ColorDialog {
        id: colorDialog
        title: qsTr("Please choose a color")
        showAlphaChannel: alpha
        color: value
        onAccepted: {
            // Make a copy of the current value.
            var myColor = Qt.darker(value, 1.0)
            // Ignore alpha when comparing.
            myColor.a = currentColor.a
            // If the user changed color but left alpha at 0,
            // they probably want to reset alpha to opaque.
            if (currentColor.a === 0 && (!Qt.colorEqual(currentColor, myColor) ||
                                         (Qt.colorEqual(currentColor, 'transparent') && Qt.colorEqual(value, 'transparent'))))
                currentColor.a = 1.0
            // Assign the new color value. Unlike docs say, using currentColor
            // is actually more cross-platform compatible.
            value = currentColor
        }
        modality: application.dialogModality
    }
    
    Shotcut.Button {
        id: pickerButton
        icon.name: 'color-picker'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/color-picker.png'
        Shotcut.HoverTip { text: '<p>' + qsTr("Pick a color on the screen. By pressing the mouse button and then moving your mouse you can select a section of the screen from which to get an average color.") + '</p>' }
        implicitWidth: 20
        implicitHeight: 20
        checkable: true
        onClicked: {
            pickStarted()
            pickerItem.pickColor()
        }
    }
}
