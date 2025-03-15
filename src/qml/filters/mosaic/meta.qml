/*
 * Copyright (c) 2019-2025 Meltytech, LLC
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
    name: qsTr("Mosaic")
    keywords: qsTr('pixelize pixelate', 'search keywords for the Mosaic video filter') + ' mosaic #rgba'
    mlt_service: "frei0r.pixeliz0r"
    qml: "ui.qml"
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1']
        parameters: [
            Parameter {
                name: qsTr('Width')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 0.4
            },
            Parameter {
                name: qsTr('Height')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 0.4
            }
        ]
    }
}
