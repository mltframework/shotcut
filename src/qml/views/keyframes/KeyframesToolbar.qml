/*
 * Copyright (c) 2016-2020 Meltytech, LLC
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
    height: settings.smallIcons? 28 : menuButton.height + 4
    anchors.margins: 0

    RowLayout {
        ToolButton {
            id: hiddenButton
            visible: false
        }
        ToolButton {
            id: menuButton
            implicitWidth: settings.smallIcons? 18 : hiddenButton.implicitWidth
            implicitHeight: implicitWidth
            action: Action {
                id: menuAction
                tooltip: qsTr('Display a menu of additional actions')
                iconName: 'show-menu'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
                onTriggered: menu.popup()
            }
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: settings.smallIcons? 14 : (hiddenButton.implicitHeight - 8)
        }
        ToolButton {
            implicitWidth: settings.smallIcons? 18 : hiddenButton.implicitWidth
            implicitHeight: implicitWidth
            action: Action {
                tooltip: qsTr('Set the filter start')
                text: '['
                onTriggered: filter.in = producer.position + producer.in
            }
        }
        ToolButton {
            implicitWidth: settings.smallIcons? 18 : hiddenButton.implicitWidth
            implicitHeight: implicitWidth
            action: Action {
                tooltip: qsTr('Set the filter end')
                text: ']'
                onTriggered: filter.out = producer.position + producer.in
            }
        }
        ToolButton {
            implicitWidth: settings.smallIcons? 18 : hiddenButton.implicitWidth
            implicitHeight: implicitWidth
            action: Action {
                tooltip: qsTr('Set the first simple keyframe')
                text: '{'
                onTriggered: filter.animateIn = producer.position + producer.in - filter.in
            }
        }
        ToolButton {
            implicitWidth: settings.smallIcons? 18 : hiddenButton.implicitWidth
            implicitHeight: implicitWidth
            action: Action {
                tooltip: qsTr('Set the second simple keyframe')
                text: '}'
                onTriggered: filter.animateOut = filter.out - (producer.position + producer.in)
            }
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: settings.smallIcons? 14 : (hiddenButton.implicitHeight - 8)
        }
        ToolButton {
            implicitWidth: settings.smallIcons? 18 : hiddenButton.implicitWidth
            implicitHeight: implicitWidth
            action: Action {
                id: zoomOutAction
                tooltip: qsTr("Zoom keyframes out (Alt+-)")
                iconName: 'zoom-out'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/zoom-out.png'
                onTriggered: root.zoomOut()
            }
        }
        ZoomSlider {
            id: scaleSlider
        }
        ToolButton {
            implicitWidth: settings.smallIcons? 18 : hiddenButton.implicitWidth
            implicitHeight: implicitWidth
            action: Action {
                id: zoomInAction
                tooltip: qsTr("Zoom keyframes in (Alt++)")
                iconName: 'zoom-in'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/zoom-in.png'
                onTriggered: root.zoomIn()
            }
        }
        ToolButton {
            implicitWidth: settings.smallIcons? 18 : hiddenButton.implicitWidth
            implicitHeight: implicitWidth
            action: Action {
                id: zoomFitAction
                tooltip: qsTr('Zoom keyframes to fit (Alt+0)')
                iconName: 'zoom-fit-best'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/zoom-fit-best.png'
                onTriggered: root.zoomToFit()
            }
        }
    }
}
