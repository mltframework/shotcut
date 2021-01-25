/*
 * Copyright (c) 2016-2021 Meltytech, LLC
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
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

ToolBar {
    property alias scaleSlider: scaleSlider

    SystemPalette { id: activePalette }

    id: toolbar
    width: 200
    height: settings.smallIcons ? 28 : hiddenButton.implicitHeight + 3

    RowLayout {
        y: 2
        ToolButton {
            id: hiddenButton
            visible: false
            icon.name: 'show-menu'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            Shotcut.HoverTip { text: qsTr('Display a menu of additional actions') }
            focusPolicy: Qt.NoFocus
            action: Action {
                id: menuAction
                icon.name: 'show-menu'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
                onTriggered: menu.popup()
            }
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            Shotcut.HoverTip { text: qsTr('Set the filter start') }
            focusPolicy: Qt.NoFocus
            action: Action {
                icon.name: 'keyframes-filter-in'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/keyframes-filter-in.png'
                onTriggered: filter.in = producer.position + producer.in
            }
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            Shotcut.HoverTip { text: qsTr('Set the filter end') }
            focusPolicy: Qt.NoFocus
            action: Action {
                icon.name: 'keyframes-filter-out'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/keyframes-filter-out.png'
                onTriggered: filter.out = producer.position + producer.in
            }
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            Shotcut.HoverTip { text: qsTr('Set the first simple keyframe') }
            focusPolicy: Qt.NoFocus
            action: Action {
                icon.name: 'keyframes-simple-in'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/keyframes-simple-in.png'
                onTriggered: filter.animateIn = producer.position + producer.in - filter.in
            }
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            Shotcut.HoverTip { text: qsTr('Set the second simple keyframe') }
            focusPolicy: Qt.NoFocus
            action: Action {
                icon.name: 'keyframes-simple-out'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/keyframes-simple-out.png'
                onTriggered: filter.animateOut = filter.out - (producer.position + producer.in)
            }
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            Shotcut.HoverTip { text: qsTr('Zoom keyframes out (Alt+-)') }
            focusPolicy: Qt.NoFocus
            action: Action {
                id: zoomOutAction
                icon.name: 'zoom-out'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-out.png'
                onTriggered: root.zoomOut()
            }
        }
        ZoomSlider {
            id: scaleSlider
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            Shotcut.HoverTip { text: qsTr('Zoom keyframes in (Alt++)') }
            focusPolicy: Qt.NoFocus
            action: Action {
                id: zoomInAction
                icon.name: 'zoom-in'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-in.png'
                onTriggered: root.zoomIn()
            }
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            Shotcut.HoverTip { text: qsTr('Zoom keyframes to fit (Alt+0)') }
            focusPolicy: Qt.NoFocus
            action: Action {
                id: zoomFitAction
                icon.name: 'zoom-fit-best'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-fit-best.png'
                onTriggered: root.zoomToFit()
            }
        }
    }
}
