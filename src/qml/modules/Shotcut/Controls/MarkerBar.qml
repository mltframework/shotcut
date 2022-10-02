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

Repeater {
    id: markerbar

    property real timeScale: 1
    property var snapper

    signal exited()
    signal mouseStatusChanged(int mouseX, int mouseY, var text, int start, int end)
    signal seekRequested(int pos)

    Marker {
        timeScale: parent.timeScale
        snapper: markerbar.snapper
        start: model.start
        end: model.end
        markerColor: model.color
        text: model.text
        index: model.index
        onExited: markerbar.exited()
        onMouseStatusChanged: markerbar.mouseStatusChanged(mouseX, mouseY, text, start, end)
        onSeekRequested: markerbar.seekRequested(pos)
    }

}
