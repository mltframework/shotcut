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

import QtQuick 2.7

Item {
    width: 640
    height: 360

    Component.onCompleted: {
        webvfx.imageTypeMap = { "sourceImage" : webvfx.SourceImageType };
        webvfx.readyRender(true);
    }

    Rectangle {
        id: rectangle
        x: 100
        y: 100
        width: 100
        height: 100
        color: 'red'
        radius: 10
    }

    Connections {
        target: webvfx
        onRenderRequested: {
            var rect = webvfx.getRectParameter('rect')
            rectangle.x = rect.x
            rectangle.y = rect.y
            rectangle.width = rect.width
            rectangle.height = rect.height
            rectangle.color = webvfx.getStringParameter('color')
            rectangle.border.color = webvfx.getStringParameter('border.color')
            rectangle.border.width = webvfx.getStringParameter('border.width')
            rectangle.radius = webvfx.getNumberParameter('radius') * 0.5 * Math.min(rect.width, rect.height)
	    }
    }
}
