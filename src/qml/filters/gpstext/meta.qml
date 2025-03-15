/*
 * Copyright (c) 2022-2025 Meltytech, LLC
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
    name: qsTr("GPS Text")
    keywords: qsTr('gpx', 'search keywords for the GPS Text video filter') + ' gps text #rgba'
    mlt_service: 'gpstext'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['geometry', 'fgcolour', 'olcolour', 'bgcolour', 'opacity']
        parameters: [
            Parameter {
                name: qsTr('Position / Size')
                property: 'geometry'
                isRectangle: true
            },
            Parameter {
                name: qsTr('Font color')
                property: 'fgcolour'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Outline')
                property: 'olcolour'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Background')
                property: 'bgcolour'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Opacity')
                property: 'opacity'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
