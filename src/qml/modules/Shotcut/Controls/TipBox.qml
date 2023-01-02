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
import QtQuick 2.0

Rectangle {
    property alias text: label.text

    color: activePalette.alternateBase
    radius: 5
    height: Math.ceil(text.length / 80) * label.height + 2 * label.y

    SystemPalette {
        id: activePalette
    }

    Text {
        id: label

        x: 10
        y: 10
        width: parent.width - 2 * x
        wrapMode: Text.WordWrap
        color: activePalette.windowText
    }
}
