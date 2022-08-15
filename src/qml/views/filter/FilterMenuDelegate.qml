/*
 * Copyright (c) 2014-2020 Meltytech, LLC
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

import "FilterMenu.js" as Logic
import QtQuick 2.2
import QtQuick.Controls 2.12
import org.shotcut.qml 1.0 as Shotcut

Rectangle {
    id: wrapper

    visible: isVisible
    height: visible ? Logic.ITEM_HEIGHT : 0
    color: activePalette.base

    SystemPalette {
        id: activePalette
    }

    Row {
        height: Logic.ITEM_HEIGHT

        Rectangle {
            color: activePalette.base
            anchors.top: parent.top
            width: parent.height
            height: parent.height

            ToolButton {
                id: favButton

                implicitWidth: 20
                implicitHeight: 18
                padding: 1
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                opacity: favorite ? 1 : 0.3
                icon.name: 'bookmarks'
                icon.source: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                onClicked: favorite = !favorite
            }

        }

        Rectangle {
            id: itemBackground

            color: wrapper.ListView.isCurrentItem ? activePalette.highlight : activePalette.base
            width: wrapper.ListView.view.width - favButton.width
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            ToolButton {
                id: itemIcon

                implicitWidth: 20
                implicitHeight: 18
                padding: 1
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                enabled: false
                icon.name: needsGpu ? 'cpu' : isAudio ? 'speaker' : pluginType == Shotcut.Metadata.Link ? 'chronometer' : 'video-television'
                icon.source: needsGpu ? 'qrc:///icons/oxygen/32x32/devices/cpu.png' : isAudio ? 'qrc:///icons/oxygen/32x32/actions/speaker.png' : pluginType == Shotcut.Metadata.Link ? 'qrc:///icons/oxygen/32x32/actions/chronometer.png' : 'qrc:///icons/oxygen/32x32/devices/video-television.png'
            }

            Label {
                id: itemText

                anchors.left: itemIcon.right
                anchors.right: itemBackground.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                text: name
                color: wrapper.ListView.isCurrentItem ? activePalette.highlightedText : activePalette.text
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: mouseArea

                anchors.fill: parent
                hoverEnabled: wrapper.height > 0
                onClicked: {
                    wrapper.ListView.view.itemSelected(index);
                }
                onEntered: {
                    wrapper.ListView.view.currentIndex = index;
                }
            }

        }

    }

}
