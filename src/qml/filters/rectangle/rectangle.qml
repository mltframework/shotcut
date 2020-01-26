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
        webvfx.imageTypeMap = { "sourceImage" : webvfx.SourceImageType }
        webvfx.readyRender(true)
    }

    Rectangle {
        id: rectangle
        x: 100
        y: 100
        width: 100
        height: 100
        color: 'red'
        radius: 10
        gradient: Gradient {
            id: gradientView
        }
        property string colors: ''
    }

    Component
    {
        id: stopComponent
        GradientStop {}
    }

    Connections {
        target: webvfx
        onRenderRequested: {
            var rect = webvfx.getRectParameter('rect')
            rectangle.x = rect.x
            rectangle.y = rect.y
            rectangle.width = rect.width
            rectangle.height = rect.height

            var colors = []
            var newStops = []
            var i = 1
            var color = webvfx.getStringParameter('color.' + i++)
            while (color) {
                colors.push(color)
                color = webvfx.getStringParameter('color.' + i++)
            }
            if (colors.toString() !== rectangle.colors) {
                var stepSize = (colors.length > 1)? 1.0 / (colors.length - 1) : 0
                for (i = 0; i < colors.length; i++) {
                    newStops.push(
                        stopComponent.createObject(gradientView, {
                            "position": stepSize * i,
                            "color": colors[i]
                        })
                    )
                }
                gradientView.stops = newStops
                rectangle.colors = colors.toString()
            }

            rectangle.border.color = webvfx.getStringParameter('border.color')
            rectangle.border.width = webvfx.getStringParameter('border.width')
            rectangle.radius = webvfx.getNumberParameter('radius') * 0.5 * Math.min(rect.width, rect.height)
	    }
    }
}
