/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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
import QtQuick.Controls 1.0

Rectangle {
    id: tooltip

    property alias text: label.text
    property bool hovered: parent.hovered

    color: timeline.toolTipBaseColor
    border.color: timeline.toolTipTextColor
    border.width: 1
    anchors {
        left: parent.left
        leftMargin: 4
        top: parent.bottom
        topMargin: 4
    }
    width: label.width + 8
    height: label.height + 8
    states: [
        State { name: 'invisible'; PropertyChanges { target: tooltip; opacity: 0} },
        State { name: 'visible'; PropertyChanges { target: tooltip; opacity: 1} }
    ]
    state: 'invisible'
    transitions: [
        Transition {
            from: 'invisible'
            to: 'visible'
            OpacityAnimator { target: tooltip; duration: 200; easing.type: Easing.InOutQuad }
        },
        Transition {
            from: 'visible'
            to: 'invisible'
            OpacityAnimator { target: tooltip; duration: 200; easing.type: Easing.InOutQuad }
        }
    ]

    Timer {
        id: timer
        interval: 1000
        onTriggered: parent.state = 'visible'
    }

    Label {
        id: label
        color: timeline.toolTipTextColor
        anchors.centerIn: parent
    }

    onHoveredChanged: {
        if (hovered) {
            timer.restart()
        } else {
            timer.stop()
            tooltip.state = 'invisible'
        }
    }
}
