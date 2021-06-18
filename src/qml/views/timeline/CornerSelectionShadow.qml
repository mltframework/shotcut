/*
 * Copyright (c) 2015-2020 Meltytech, LLC
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

Item {
    id: root

    property Item clip
    property bool mirrorGradient: false

    width: 100
    height: clip ? clip.height : 0
    Behavior on opacity { NumberAnimation { duration: 100 } }

    Rectangle {
        id: shadowGradient
        width: parent.height
        height: parent.width
        anchors.centerIn: parent
        rotation: mirrorGradient ? -90 : 90
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: "white" }
        }
        PulsingAnimation {
            target: shadowGradient
            running: root.opacity
        }
    }
}
