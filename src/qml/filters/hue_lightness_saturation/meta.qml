/*
 * Copyright (c) 2018-2022 Meltytech, LLC
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
import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Hue/Lightness/Saturation")
    keywords: qsTr('color value desaturate grayscale', 'search keywords for the Hue/Lightness/Saturation video filter') + ' hue lightness saturation'
    mlt_service: 'avfilter.hue'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['av.h', 'av.b', 'av.s']
        parameters: [
            Parameter {
                name: qsTr('Hue')
                property: 'av.h'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Lightness')
                property: 'av.b'
                isCurve: true
                minimum: -10
                maximum: 10
            },
            Parameter {
                name: qsTr('Saturation')
                property: 'av.s'
                isCurve: true
                minimum: 0
                maximum: 5
            }
        ]
    }
}
