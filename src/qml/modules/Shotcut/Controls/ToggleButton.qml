/*
 * Copyright (c) 2018-2020 Meltytech, LLC
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
    padding: 2
    checkable: true
    hoverEnabled: true
    display: AbstractButton.TextBesideIcon

    SystemPalette { id: activePalette }
    palette.buttonText: activePalette.buttonText

    background: Rectangle {
        radius: 3
        color: parent.checked ? activePalette.highlight : activePalette.button
        border.color: activePalette.shadow
        border.width: parent.checked ? 0 : 1
    }

    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()
}
