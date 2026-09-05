/*
 * Copyright (c) 2013-2026 Meltytech, LLC
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

    // Minimum pixel width for a tick label, so ticks are spaced widely enough to never overlap.
    readonly property real minTickSpacing: fontMetrics.boundingRect("00:00:00").width + 16
    readonly property real intervalSeconds: Math.max(1, Math.ceil(minTickSpacing / (profile.fps * timeScale)))
    readonly property real tickSpacing: intervalSeconds * profile.fps * timeScale
    // Clamp to rulerTop.width (the actual producer duration width) so ticks do not render past the end of a short/empty producer.
    // The tickSpacing * 3 look-ahead avoids tick pop-in while scrolling.
    readonly property real tickAreaEnd: Math.min(tracksFlickable.contentX + tracksFlickable.width + tickSpacing * 3, rulerTop.width)
    readonly property int firstTick: tickSpacing > 0 ? Math.max(0, Math.floor(tracksFlickable.contentX / tickSpacing) - 1) : 0
    readonly property int tickCount: (tickSpacing > 0 && tickAreaEnd > firstTick * tickSpacing) ? Math.ceil((tickAreaEnd - firstTick * tickSpacing) / tickSpacing) : 0

    height: 28
    color: activePalette.base

    FontMetrics {
        id: fontMetrics
    }

    Repeater {
        id: repeater
        model: rulerTop.tickCount

        Rectangle {
            readonly property int tickIndex: index + rulerTop.firstTick

            // right edge
            anchors.bottom: rulerTop.bottom
            height: 18
            width: 1
            color: activePalette.windowText
            x: tickIndex * rulerTop.tickSpacing

            Label {
                anchors.left: parent.right
                anchors.leftMargin: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                color: activePalette.windowText
                text: application.clockFromFrames(parent.tickIndex * rulerTop.intervalSeconds * profile.fps + 2).substr(0, 8)
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

    Connections {
        function onTimeFormatChanged() {
            const m = repeater.model;
            repeater.model = 0;
            repeater.model = m;
        }

        target: settings
    }
}
