/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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

Rectangle {
    property alias value: slider.value

    SystemPalette { id: activePalette }

    color: activePalette.window
    width: 200
    height: 14

    Slider {
        id: slider
        orientation: Qt.Horizontal
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            leftMargin: 4
            rightMargin: 4
        }
        from: 0
        to: 3.0
        value: 1
        focusPolicy: Qt.NoFocus
        function setScaleFactor() {
            timeScale = Math.pow(value, 3) + 0.01
        }
        onValueChanged: {
            if (!pressed)
                setScaleFactor()
        }
        onPressedChanged: {
            if (!pressed) {
                var targetX = tracksFlickable.contentX + tracksFlickable.width / 2
                var offset = targetX - tracksFlickable.contentX
                var before = timeScale

                setScaleFactor()
                
                tracksFlickable.contentX = (targetX * timeScale / before) - offset

                redrawWaveforms()

                if (settings.timelineScrollZoom)
                    scrollZoomTimer.restart()
            }
        }
    }
}
