/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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
    width: 400
    height: 200
    interactive: false
    clip: true
    property real zoom: (video.zoom > 0)? video.zoom : 1.0
    contentWidth: video.rect.width * zoom
    contentHeight: video.rect.height * zoom
    contentX: video.offset.x
    contentY: video.offset.y

    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('rect', '0/0:100%x100%')
        }
        rectangle.setHandles(filter.getRect('rect'))
    }

    Connections {
        target: filter
        onChanged: rectangle.setHandles(filter.getRect('rect'))
    }

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
            aspectRatio: profile.aspectRatio
            handleSize: 15 / zoom
            borderSize: 2.0 / zoom
            onRectChanged: {
                filter.set('rect', '%1%/%2%:%3%x%4%'
                           .arg(Math.round(rect.x / widthScale) / profile.width * 100)
                           .arg(Math.round(rect.y / heightScale) / profile.height * 100)
                           .arg(Math.round(rect.width / widthScale) / profile.width * 100)
                           .arg(Math.round(rect.height / heightScale) / profile.height * 100))
                console.log('' + filter.get('rect'))
            }
        }
    }
}
