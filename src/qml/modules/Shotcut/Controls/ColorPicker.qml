/*
 * Copyright (c) 2014-2022 Meltytech, LLC
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
import QtQuick.Dialogs
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

RowLayout {
    property string value: "white"
    property bool alpha: false
    property alias eyedropper: pickerButton.visible

    signal pickStarted
    signal pickCancelled

    SystemPalette {
        id: activePalette

        colorGroup: SystemPalette.Active
    }

    Shotcut.ColorPickerItem {
        id: pickerItem

        onColorPicked: color => {
            value = color;
            pickerButton.checked = false;
        }
        onCancelled: pickCancelled()
    }

    Shotcut.Button {
        id: colorButton

        implicitWidth: 20
        implicitHeight: 20
        onClicked: colorDialog.visible = true

        Shotcut.HoverTip {
            text: qsTr('Click to open color dialog')
        }

        background: Rectangle {
            border.width: 1
            border.color: 'gray'
            radius: pickerButton.background.radius
            color: value
        }
    }

    ColorDialog {
        id: colorDialog

        title: qsTr("Please choose a color")
        options: ColorDialog.ShowAlphaChannel
        selectedColor: value
        onAccepted: {
            // Make a copy of the current value.
            var myColor = Qt.darker(value, 1);
            // Ignore alpha when comparing.
            myColor.a = selectedColor.a;
            // If the user changed color but left alpha at 0,
            // they probably want to reset alpha to opaque.
            if (selectedColor.a === 0 && (!Qt.colorEqual(selectedColor, myColor) || (Qt.colorEqual(selectedColor, 'transparent') && Qt.colorEqual(value, 'transparent'))))
                selectedColor.a = 1;

            // Assign the new color value. Unlike docs say, using selectedColor
            // is actually more cross-platform compatible.
            value = selectedColor;
        }
        modality: application.dialogModality
    }

    Shotcut.Button {
        id: pickerButton

        icon.name: 'color-picker'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/color-picker.png'
        implicitWidth: 20
        implicitHeight: 20
        checkable: true
        onClicked: {
            pickStarted();
            pickerItem.pickColor();
        }

        Shotcut.HoverTip {
            text: '<p>' + qsTr("Pick a color on the screen. By pressing the mouse button and then moving your mouse you can select a section of the screen from which to get an average color.") + '</p>'
        }
    }
}
