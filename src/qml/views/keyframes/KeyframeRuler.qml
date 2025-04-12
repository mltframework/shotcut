/*
 * Copyright (c) 2013-2025 Meltytech, LLC
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
import QtQuick.Controls

Rectangle {
    id: rulerTop

    property real intervalSeconds: (timeScale > 5) ? 1 : (5 * Math.max(1, Math.floor(1.5 / timeScale)))

    height: 28
    color: activePalette.base

    Repeater {
        model: parent.width / (intervalSeconds * profile.fps * timeScale)

        Rectangle {

            // right edge
            anchors.bottom: rulerTop.bottom
            height: 18
            width: 1
            color: activePalette.windowText
            x: index * intervalSeconds * profile.fps * timeScale
            visible: ((x + width) > tracksFlickable.contentX) && (x < tracksFlickable.contentX + tracksFlickable.width) // left edge

            Label {
                anchors.left: parent.right
                anchors.leftMargin: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                color: activePalette.windowText
                text: application.clockFromFrames(index * intervalSeconds * profile.fps + 2).substr(0, 8)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onExited: bubbleHelp.hide()
        onPositionChanged: mouse => {
            var text = application.timeFromFrames(mouse.x / timeScale);
            bubbleHelp.show(text);
        }
    }
}
