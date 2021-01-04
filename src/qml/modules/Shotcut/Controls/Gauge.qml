/*
 * Copyright (c) 2021 Meltytech, LLC
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

import QtQuick 2.12
import QtQuick.Controls 2.12

Column {
    property alias from: slider.from
    property alias value: slider.value
    property alias to: slider.to
    property alias orientation: slider.orientation
    property int decimals: 1

    SystemPalette { id: activePalette }
    padding: 0
    spacing: 0

    Slider {
        id: slider
        height: 20
        width: parent.width

        background: Rectangle {
            radius: 3
            color: activePalette.alternateBase
            border.color: 'gray'
            border.width: 1
            height: parent.height

            Rectangle {
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    margins: 1
                }
                radius: parent.radius
                width: 4
                x: (parent.width - 5) * slider.visualPosition
                color: enabled ? activePalette.highlight : activePalette.midlight
            }
        }

        handle: Rectangle {}
    }

    Item {
        id: tickItem
        implicitHeight: 7
        width: parent.width
        Repeater {
            model: 11
            Rectangle {
                y: 1
                x: index * tickItem.width / 10
                width: 1
                height: (index % 5) == 0 ? 6 : 3
                color: 'gray'
            }
        }
    }

    Item {
        implicitHeight: textItem.implicitHeight
        width: parent.width
        Text {
            id: textItem
            anchors.fill: parent
            color: activePalette.text
            text: Number(slider.from).toLocaleString(Qt.locale(), 'f', decimals)
            horizontalAlignment: Text.AlignLeft
        }
        Text {
            anchors.fill: parent
            color: activePalette.text
            text: Number((slider.to - slider.from) / 2.0).toLocaleString(Qt.locale(), 'f', decimals)
            horizontalAlignment: Text.AlignHCenter
        }
        Text {
            anchors.fill: parent
            color: activePalette.text            
            text: Number(slider.to).toLocaleString(Qt.locale(), 'f', decimals)
            horizontalAlignment: Text.AlignRight
        }
    }
}
