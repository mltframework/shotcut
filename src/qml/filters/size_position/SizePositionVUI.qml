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
    property string rectProperty
    property string fillProperty
    property string distortProperty
    property string halignProperty
    property string valignProperty
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

    function getAspectRatio() {
        return (filter.get(fillProperty) === '1' && filter.get(distortProperty) === '0')? filter.producerAspect : 0.0
    }

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
            aspectRatio: getAspectRatio()
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
            if (rectangle.aspectRatio !== getAspectRatio()) {
                rectangle.aspectRatio = getAspectRatio()
                rectangle.setHandles(filterRect)
                var rect = rectangle.rectangle
                filter.set(rectProperty, '%1%/%2%:%3%x%4%'
                           .arg((Math.round(rect.x / rectangle.widthScale) / profile.width * 100).toLocaleString(_locale))
                           .arg((Math.round(rect.y / rectangle.heightScale) / profile.height * 100).toLocaleString(_locale))
                           .arg((Math.round(rect.width / rectangle.widthScale) / profile.width * 100).toLocaleString(_locale))
                           .arg((Math.round(rect.height / rectangle.heightScale) / profile.height * 100).toLocaleString(_locale)))
            }
        }
    }
}
