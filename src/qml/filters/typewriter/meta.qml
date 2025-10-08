/*
 * Copyright (c) 2025 Meltytech, LLC
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
    name: qsTr('Text: Typewriter')
    keywords: qsTr('type typing cursor terminal console', 'search keywords for the Text: Typewriter video filter') + ' text: text typewriter #rgba'
    mlt_service: 'qtext'
    objectName: 'typewriter'
    qml: 'ui.qml'
    vui: 'vui.qml'
    minimumVersion: '5'
    icon: 'icon.webp'
    
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['opacity']
        parameters: [
            Parameter {
                name: qsTr('Foreground color')
                property: 'fgcolour'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Outline color')
                property: 'olcolour'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Background color')
                property: 'bgcolour'
                isCurve: true
                minimum: 0
                maximum: 1
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
