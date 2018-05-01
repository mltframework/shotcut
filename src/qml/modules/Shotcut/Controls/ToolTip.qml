/*
 * Copyright (c) 2014-2018 Meltytech, LLC
 * Author: Brian Matherly <pez4brian@yahoo.com>
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

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQml 2.2
import QtQuick.Layouts 1.1
import org.shotcut.qml 1.0 as Shotcut

Item {
    property alias text: toolTipText.text
    property alias isVisible: toolTipMouseArea.enabled
    property alias cursorShape: toolTipMouseArea.cursorShape
    property alias containsMouse: toolTipMouseArea.containsMouse

    anchors.fill: parent

    MouseArea {
        id: toolTipMouseArea
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        onEntered: {
            toolTipWindow.beginDisplay()
        }
        onExited: {
            toolTipWindow.endDisplay()
        }
        onEnabledChanged: {
            if (!enabled) toolTipWindow.close()
        }
    }

    Window {
        id: toolTipWindow
        width: toolTipRect.width + 2
        height: toolTipRect.height + 2
        color: application.toolTipTextColor
        visible: false
        flags: Qt.ToolTip
        
        function beginDisplay() {
            if (!toolTipWindow.visible) {
                tipTimer.interval = 1000
                tipTimer.running = true
            } else {
                tipTimer.running = false
            }
        }

        function endDisplay() {
            if (toolTipWindow.visible) {
                tipTimer.interval = 500
                tipTimer.running = true
            } else {
                tipTimer.running = false
            }
        }

        Timer {
            id: tipTimer
            onTriggered: {
                if (!toolTipWindow.visible) {
                    var cursorPoint = application.mousePos
                    toolTipWindow.x = cursorPoint.x
                    toolTipWindow.y = cursorPoint.y + 15
                }
                if (toolTipWindow.visible) {
                    toolTipWindow.close()
                }
                else if (toolTipMouseArea.enabled) {
                    toolTipWindow.show()
                }
            }
        }

        Rectangle {
            id: toolTipRect
            anchors.centerIn: parent
            color: application.toolTipBaseColor
            implicitWidth: Math.min(350, toolTipText.implicitWidth + 4)
            height: toolTipText.contentHeight + 4
            Text {
                id: toolTipText
                anchors.fill: parent
                anchors.margins: 2
                color: application.toolTipTextColor
                clip: false
                wrapMode: Text.WordWrap
            }
        }
    }
}
