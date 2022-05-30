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
import Shotcut.Controls 1.0 as Shotcut

Shotcut.VuiBase {
    property string rectProperty: "rect"
    property real zoom: (video.zoom > 0)? video.zoom : 1.0
    property rect filterRect: filter ? filter.getRect(rectProperty) : Qt.rect(0, 0, 0, 0)

    Component.onCompleted: {
        application.showStatusMessage(qsTr('Click in rectangle + hold Shift to drag'))
        rectangle.setHandles(filter.getRect(rectProperty))
    }

    Flickable {
        anchors.fill: parent
        interactive: false
        clip: true
        contentWidth: video.rect.width * zoom
        contentHeight: video.rect.height * zoom
        contentX: video.offset.x
        contentY: video.offset.y

        Item {
            id: videoItem
            x: video.rect.x
            y: video.rect.y
            width: video.rect.width
            height: video.rect.height
            scale: zoom

            Shotcut.RectangleControl {
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
                    filter.set(rectProperty, filterRect)
                }
            }
        }
    }

    Connections {
        target: filter
        function onChanged() {
            var newRect = filter.getRect(rectProperty)
            if (filterRect !== newRect) {
                filterRect = newRect
                rectangle.setHandles(filterRect)
            }
            videoItem.enabled = filter.get('disable') !== '1'
        }
    }
}
