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

import QtQuick 2.1
import Shotcut.Controls 1.0

RectVui {
    id: rectVui
    boundary: Qt.rect(1, 1, profile.width - 2, profile.height - 2)

    property bool _blockUpdates: false;

    onFilterRectChanged: {
        if (_blockUpdates) return

        rectVui.filterRect.x = Math.min(rectVui.filterRect.x, profile.width - rectVui.filterRect.width - 1)
        rectVui.filterRect.x = Math.max(rectVui.filterRect.x, 1)
        rectVui.filterRect.y = Math.min(rectVui.filterRect.y, profile.height - rectVui.filterRect.height - 1)
        rectVui.filterRect.y = Math.max(rectVui.filterRect.y, 1)
        rectVui.filterRect.width = Math.min(rectVui.filterRect.width, profile.width - rectVui.filterRect.x - 1)
        rectVui.filterRect.width = Math.max(rectVui.filterRect.width, 1)
        rectVui.filterRect.height = Math.min(rectVui.filterRect.height, profile.width - rectVui.filterRect.y - 1)
        rectVui.filterRect.height = Math.max(rectVui.filterRect.height, 1)

        if (rectVui.filterRect.x != filter.get("av.x") ||
            rectVui.filterRect.y != filter.get("av.y") ||
            rectVui.filterRect.width != filter.get("av.w") ||
            rectVui.filterRect.height != filter.get("av.h") )
        {
            _blockUpdates = true
            filter.set("av.x", rectVui.filterRect.x)
            filter.set("av.y", rectVui.filterRect.y)
            filter.set("av.w", rectVui.filterRect.width)
            filter.set("av.h", rectVui.filterRect.height)
            _blockUpdates = false
        }
    }

    Component.onCompleted: {
        if (_blockUpdates) return
        if (rectVui.filterRect.x != filter.get("av.x") ||
            rectVui.filterRect.y != filter.get("av.y") ||
            rectVui.filterRect.width != filter.get("av.w") ||
            rectVui.filterRect.height != filter.get("av.h") )
        {
            _blockUpdates = true
            rectVui.filterRect = Qt.rect(filter.get("av.x"), filter.get("av.y"), filter.get("av.w"), filter.get("av.h"))
            _blockUpdates = false
        }
    }

    Connections {
        target: filter
        onChanged: {
            if (_blockUpdates) return
            if (rectVui.filterRect.x != filter.get("av.x") ||
                rectVui.filterRect.y != filter.get("av.y") ||
                rectVui.filterRect.width != filter.get("av.w") ||
                rectVui.filterRect.height != filter.get("av.h") )
            {
                _blockUpdates = true
                rectVui.filterRect = Qt.rect(filter.get("av.x"), filter.get("av.y"), filter.get("av.w"), filter.get("av.h"))
                _blockUpdates = false
            }
        }
    }
}
