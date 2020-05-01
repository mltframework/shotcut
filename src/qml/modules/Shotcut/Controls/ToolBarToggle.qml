/*
 * Copyright (c) 2020 Meltytech, LLC
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
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import Shotcut.Controls 1.0 as Shotcut

CheckBox {
    property string iconName
    property url iconSource
    property alias tooltip: tooltip.text
    property bool checked: false
    implicitWidth: 26
    implicitHeight: 22

    anchors.verticalCenter: parent.verticalCenter
    style: CheckBoxStyle {
        background: Rectangle {
            radius: 3
            SystemPalette { id: activePalette }
            color: control.checked? activePalette.highlight : activePalette.button
        }
        indicator: ToolButton {
            implicitWidth: control.implicitWidth
            implicitHeight: control.implicitHeight
            iconName: control.iconName
            iconSource: control.iconSource
        }
    }
    Shotcut.ToolTip { id: tooltip }
}
