/*
 * Copyright (c) 2013-2020 Meltytech, LLC
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
import Shotcut.Controls 1.0 as Shotcut

Rectangle {
    property real timeScale: 1.0
    property int adjustment: 0
    property real intervalSeconds: ((timeScale > 5)? 1 : (5 * Math.max(1, Math.floor(1.5 / timeScale)))) + adjustment
    signal editMarkerRequested(int index)
    signal deleteMarkerRequested(int index)

    SystemPalette { id: activePalette }

    id: rulerTop
    height: 28
    color: activePalette.base

    Repeater {
        model: parent.width / (intervalSeconds * profile.fps * timeScale)
        Rectangle {
            anchors.bottom: rulerTop.bottom
            height: 18
            width: 1
            color: activePalette.windowText
            x: index * intervalSeconds * profile.fps * timeScale
            visible: ((x + width)   > tracksFlickable.contentX) && // right edge
                      (x            < tracksFlickable.contentX + tracksFlickable.width) // left edge

            Label {
                anchors.left: parent.right
                anchors.leftMargin: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                color: activePalette.windowText
                text: application.timecode(index * intervalSeconds * profile.fps + 2).substr(0, 8)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onExited: bubbleHelp.hide()
        onPositionChanged: {
            var text = application.timecode(mouse.x / timeScale)
            bubbleHelp.show(mouse.x + bubbleHelp.width - 8, mouse.y + 65, text)
        }
    }

    Shotcut.MarkerBar {
        anchors.top: rulerTop.top
        anchors.left: parent.left
        anchors.right: parent.right
        timeScale: root.timeScale
        model: markers
        onEditRequested: {
            parent.editMarkerRequested(index)
        }
        onDeleteRequested: {
            parent.deleteMarkerRequested(index)
        }
        onExited: bubbleHelp.hide()
        onMouseStatusChanged: {
            var msg = "<center>" + text
            if (start === end) {
                msg += "<br>" + application.timecode(start)
            } else {
                msg += "<br>" + application.timecode(start) + " - " + application.timecode(end)
                msg += "<br>" + application.timecode(end - start + 1)
            }
            msg += "</center>"
            bubbleHelp.show(mouseX + bubbleHelp.width - 8, mouseY + 87, msg)
        }
        onSeekRequested: timeline.position = pos
    }

    Connections {
        target: profile
        onProfileChanged: {
            // Force a repeater model change to update the labels.
            ++adjustment
            --adjustment
        }
    }
}
