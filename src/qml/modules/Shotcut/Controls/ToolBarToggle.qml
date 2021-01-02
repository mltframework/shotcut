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
import QtQuick.Controls 2.12
import Shotcut.Controls 1.0 as Shotcut

ToolButton {
    id: control
    property string iconName
    property url iconSource
    property alias tooltip: tooltip.text

    background: Rectangle {
        id: rect
        radius: 3
        width: control.width
        height: control.height
        SystemPalette { id: activePalette }
        color: control.checked? activePalette.highlight : activePalette.button
    }
    action: Action {
        icon.name: iconName
        icon.source: iconSource
    }
    Shotcut.HoverTip { id: tooltip }
}
