/*
 * Copyright (c) 2017-2018 Meltytech, LLC
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

VuiBase {
    property rect filterRect
    property rect boundary: Qt.rect(-10000, -10000, 20000, 20000)
    property real _zoom: (video.zoom > 0)? video.zoom : 1.0

    onFilterRectChanged: {
        rectangle.setHandles(filterRect)
    }

    Flickable {
        anchors.fill: parent
        interactive: false
        clip: true
        contentWidth: video.rect.width * _zoom
        contentHeight: video.rect.height * _zoom
        contentX: video.offset.x
        contentY: video.offset.y

        Item {
            id: videoItem
            x: video.rect.x
            y: video.rect.y
            width: video.rect.width
            height: video.rect.height
            scale: _zoom

            RectangleControl {
                id: rectangle
                widthScale: video.rect.width / profile.width
                heightScale: video.rect.height / profile.height
                handleSize: Math.max(Math.round(8 / _zoom), 4)
                borderSize: Math.max(Math.round(1.33 / _zoom), 1)
                onWidthScaleChanged: {
                    setHandles(filterRect)
                    setBoundary(boundary)
                }
                onHeightScaleChanged: {
                    setHandles(filterRect)
                    setBoundary(boundary)
                }
                onRectChanged:  {
                    filterRect = Qt.rect(Math.round(rect.x / rectangle.widthScale),
                                         Math.round(rect.y / rectangle.heightScale),
                                         Math.round(rect.width / rectangle.widthScale),
                                         Math.round(rect.height / rectangle.heightScale))
                }
            }
        }
    }
}
