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

Grid {
    id: root

    property alias from: slider.from
    property alias value: slider.value
    property alias to: slider.to
    property alias orientation: slider.orientation
    property int decimals: 1
    property int orientation: Qt.Horizontal

    padding: 0
    spacing: 0
    rows: 3
    columns: 1
    layoutDirection: Qt.LeftToRight
    state: orientation == Qt.Horizontal ? "horizontal" : "vertical"
    states: [
        State {
            name: "horizontal"

            PropertyChanges {
                target: root
                rows: 3
                columns: 1
                layoutDirection: Qt.LeftToRight
            }

            PropertyChanges {
                target: slider
                orientation: Qt.Horizontal
            }

        },
        State {
            name: "vertical"

            PropertyChanges {
                target: root
                rows: 1
                columns: 3
                layoutDirection: Qt.RightToLeft
            }

            PropertyChanges {
                target: slider
                orientation: Qt.Vertical
            }

        }
    ]

    SystemPalette {
        id: activePalette
    }

    Slider {
        id: slider

        enabled: false
        orientation: root.orientation
        implicitHeight: orientation == Qt.Horizontal ? 18 : parent.height
        implicitWidth: orientation == Qt.Horizontal ? parent.width : 20

        background: Rectangle {
            radius: 3
            color: activePalette.alternateBase
            border.color: 'gray'
            border.width: 1
            height: parent.height

            Rectangle {
                radius: parent.radius
                width: orientation == Qt.Horizontal ? 4 : parent.width
                height: orientation == Qt.Horizontal ? parent.height : 4
                x: orientation == Qt.Horizontal ? (parent.width - 5) * slider.visualPosition : 0
                y: orientation == Qt.Horizontal ? 0 : (parent.height - 5) * slider.visualPosition
                color: activePalette.highlight

                anchors {
                    top: orientation == Qt.Horizontal ? parent.top : undefined
                    bottom: orientation == Qt.Horizontal ? parent.bottom : undefined
                    left: orientation == Qt.Horizontal ? undefined : parent.left
                    right: orientation == Qt.Horizontal ? undefined : parent.right
                    margins: 1
                }

            }

        }

        handle: Rectangle {
        }

    }

    Item {
        id: tickItem

        implicitHeight: orientation == Qt.Horizontal ? 7 : parent.height
        implicitWidth: orientation == Qt.Horizontal ? parent.width : 7

        Repeater {
            model: 11

            Rectangle {
                property int lineSize: (index % 5) == 0 ? 6 : 3

                y: orientation == Qt.Horizontal ? 1 : index * tickItem.height / 10
                x: orientation == Qt.Horizontal ? index * tickItem.width / 10 : 1
                width: orientation == Qt.Horizontal ? 1 : lineSize
                height: orientation == Qt.Horizontal ? lineSize : 1
                color: 'gray'
            }

        }

    }

    Item {
        function widestText() {
            var widest = 0;
            for (var i = 0; i < children.length; i++) {
                if (children[i].implicitWidth > widest)
                    widest = children[i].implicitWidth;

            }
            return widest;
        }

        implicitHeight: orientation == Qt.Horizontal ? textItem.implicitHeight : parent.height
        implicitWidth: orientation == Qt.Horizontal ? parent.width : widestText()

        Text {
            id: textItem

            anchors.fill: parent
            color: activePalette.text
            text: Number(slider.from).toLocaleString(Qt.locale(), 'f', decimals)
            horizontalAlignment: orientation == Qt.Horizontal ? Text.AlignLeft : Text.AlignRight
            verticalAlignment: orientation == Qt.Horizontal ? Text.AlignVCenter : Text.AlignBottom
        }

        Text {
            anchors.fill: parent
            color: activePalette.text
            text: Number((slider.to + slider.from) / 2).toLocaleString(Qt.locale(), 'f', decimals)
            horizontalAlignment: orientation == Qt.Horizontal ? Text.AlignHCenter : Text.AlignRight
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            anchors.fill: parent
            color: activePalette.text
            text: Number(slider.to).toLocaleString(Qt.locale(), 'f', decimals)
            horizontalAlignment: Text.AlignRight
            verticalAlignment: orientation == Qt.Horizontal ? Text.AlignVCenter : Text.AlignTop
        }

    }

}
