/*
 * Copyright (c) 2017 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.org>
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

Flickable {
    property string rectProperty: "rect"
    property var _locale: Qt.locale(application.numericLocale)

    width: 400
    height: 200
    interactive: false
    clip: true
    property real zoom: (video.zoom > 0)? video.zoom : 1.0
    property rect filterRect: filter.getRect(rectProperty)
    contentWidth: video.rect.width * zoom
    contentHeight: video.rect.height * zoom
    contentX: video.offset.x
    contentY: video.offset.y

    Component.onCompleted: {
        rectangle.setHandles(filter.getRect(rectProperty))
    }

    DropArea { anchors.fill: parent }

    Item {
        id: videoItem
        x: video.rect.x
        y: video.rect.y
        width: video.rect.width
        height: video.rect.height
        scale: zoom

        RectangleControl {
            id: rectangle
            widthScale: video.rect.width / profile.width
            heightScale: video.rect.height / profile.height
            handleSize: Math.max(Math.round(8 / zoom), 4)
            borderSize: Math.max(Math.round(1.33 / zoom), 1)
            onWidthScaleChanged: setHandles(filter.getRect(rectProperty))
            onHeightScaleChanged: setHandles(filter.getRect(rectProperty))
            onRectChanged:  {
                filterRect.x = Math.round(rect.x / rectangle.widthScale)
                filterRect.y = Math.round(rect.y / rectangle.heightScale)
                filterRect.width = Math.round(rect.width / rectangle.widthScale)
                filterRect.height = Math.round(rect.height / rectangle.heightScale)
                filter.set(rectProperty, '%1%/%2%:%3%x%4%'
                           .arg((filterRect.x / profile.width * 100).toLocaleString(_locale))
                           .arg((filterRect.y / profile.height * 100).toLocaleString(_locale))
                           .arg((filterRect.width / profile.width * 100).toLocaleString(_locale))
                           .arg((filterRect.height / profile.height * 100).toLocaleString(_locale)))
            }
        }
    }

    Connections {
        target: filter
        onChanged: {
            var newRect = filter.getRect(rectProperty)
            if (filterRect !== newRect) {
                filterRect = newRect
                rectangle.setHandles(filterRect)
            }
        }
    }
}
