/*
 * Copyright (c) 2016-2019 Meltytech, LLC
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
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.0

ToolBar {
    property alias scaleSlider: scaleSlider

    SystemPalette { id: activePalette }

    width: 200
    height: menuButton.height + 4
    anchors.margins: 0

    RowLayout {
        ToolButton {
            id: menuButton
            action: menuAction
        }
        Button { // separator
            enabled: false
            implicitWidth: 1
        }
        ToolButton {
            action: zoomOutAction
        }
        ZoomSlider {
            id: scaleSlider
        }
        ToolButton {
            action: zoomInAction
        }
    }

    Action {
        id: menuAction
        tooltip: qsTr('Display a menu of additional actions')
        iconName: 'format-justify-fill'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/format-justify-fill.png'
        onTriggered: menu.popup()
    }
    Action {
        id: zoomOutAction
        tooltip: qsTr("Zoom timeline out (-)")
        iconName: 'zoom-out'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/zoom-out.png'
        onTriggered: root.zoomOut()
    }
    Action {
        id: zoomInAction
        tooltip: qsTr("Zoom timeline in (+)")
        iconName: 'zoom-in'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/zoom-in.png'
        onTriggered: root.zoomIn()
    }
}
