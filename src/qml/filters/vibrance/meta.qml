/*
 * Copyright (c) 2024 Meltytech, LLC
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
    name: qsTr('Vibrance')
    keywords: qsTr('color intensity saturation vibe', 'search keywords for the Vibrance video filter') + ' vibrance #rgba #10bit #color'
    mlt_service: 'avfilter.vibrance'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['av.intensity', 'av.rbal', 'av.gbal', 'av.bbal']
        parameters: [
            Parameter {
                name: qsTr('Intensity')
                property: 'av.intensity'
                isCurve: true
                minimum: -2
                maximum: 2
            },
            Parameter {
                name: qsTr('Red')
                property: 'av.rbal'
                isCurve: true
                minimum: -10
                maximum: 10
            },
            Parameter {
                name: qsTr('Green')
                property: 'av.gbal'
                isCurve: true
                minimum: -10
                maximum: 10
            },
            Parameter {
                name: qsTr('Blue')
                property: 'av.bbal'
                isCurve: true
                minimum: -10
                maximum: 10
            }
        ]
    }
}
