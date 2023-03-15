/*
 * Copyright (c) 2020-2022 Meltytech, LLC
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

MouseArea {
    property alias text: tip.text
    property alias metrics: fontMetrics

    anchors.fill: parent
    propagateComposedEvents: true
    acceptedButtons: Qt.NoButton

    ToolTip {
        id: tip

        visible: text ? parent.containsMouse & parent.enabled : false
        delay: 1000
        timeout: 5000
        Component.onCompleted: parent.hoverEnabled = true

        FontMetrics {
            id: fontMetrics

            font: tip.font
        }
    }
}
