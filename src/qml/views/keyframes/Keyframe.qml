/*
 * Copyright (c) 2018 Meltytech, LLC
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

import QtQuick 2.0
import org.shotcut.qml 1.0
import QtQuick.Controls 1.0
import Shotcut.Controls 1.0
import QtQuick.Window 2.2

Rectangle {
    id: keyframeRoot
    property int position: 0
    property int interpolation: KeyframesModel.DiscreteInterpolation // rectangle for discrete
    property bool isSelected: false
    property string value: ''
    property int parameterIndex

    signal clicked(var keyframe)

    SystemPalette { id: activePalette }

    x: position * timeScale - width/2
    height: 8
    width: height
    color: isSelected? 'red' : activePalette.buttonText
    border.color: isSelected? 'red' : activePalette.button
    border.width: 1
    radius: (interpolation === KeyframesModel.SmoothInterpolation)? height/2 : 0 // circle for smooth
    rotation: (interpolation === KeyframesModel.LinearInterpolation)? 45 : 0    // diamond for linear

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            if (mouse.button === Qt.LeftButton)
                parent.clicked(keyframeRoot)
            else
                menu.popup()
        }
    }
    ToolTip { text: value }

    Menu {
        id: menu
        MenuItem {
            text: qsTr('Remove')
            onTriggered: parameters.remove(parameterIndex, index)
        }
        onPopupVisibleChanged: {
            if (visible && application.OS !== 'OS X' && __popupGeometry.height > 0) {
                // Try to fix menu running off screen. This only works intermittently.
                menu.__yOffset = Math.min(0, Screen.height - (__popupGeometry.y + __popupGeometry.height + 40))
                menu.__xOffset = Math.min(0, Screen.width - (__popupGeometry.x + __popupGeometry.width))
            }
        }
    }
}
