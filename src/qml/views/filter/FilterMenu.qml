/*
 * Copyright (c) 2014-2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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
import QtQuick.Window 2.2
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.1
import 'FilterMenu.js' as Logic
import org.shotcut.qml 1.0 as Shotcut

Window {
    id: filterWindow
    visible: false

    property bool hasFocus: activeFocusItem != null

    signal filterSelected(int index)

    function popup(triggerItem) {
        var menuRect = Logic.calcMenuRect(triggerItem, toolBar.height + 2)
        filterWindow.x = Math.min(Math.max(menuRect.x, 0), Screen.width - menuRect.width)
        filterWindow.y = Math.min(Math.max(menuRect.y, 0), Screen.height - menuRect.height)
        filterWindow.height = menuRect.height
        filterWindow.show()
        filterWindow.requestActivate()

    }

    color: Qt.darker(activePalette.window, 1.5) // Border color
    flags: Qt.ToolTip
    width: 220
    height: 200
    onHasFocusChanged: if (!hasFocus) filterWindow.close()

    SystemPalette { id: activePalette }

    Rectangle {
        id: menuColorRect
        anchors.fill: parent
        anchors.margins: 1
        color: activePalette.base

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 2

            ScrollView {
                width: parent.width
                height: filterWindow.height - toolBar.height - 2
                ListView {
                    id: menuListView

                    function itemSelected(index) {
                        filterWindow.close()
                        filterSelected(index)
                    }

                    anchors.fill: parent
                    model: metadatamodel
                    delegate: FilterMenuDelegate {}
                    boundsBehavior: Flickable.StopAtBounds
                    snapMode: ListView.SnapToItem
                    currentIndex: -1
                    focus: true
                }
            }

            Rectangle {
                id: separatorBar
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                height: 1
                color: Qt.darker(activePalette.window, 1.5)
            }

            RowLayout {
                id: toolBar
                height: 30
                width: parent.width

                ExclusiveGroup { id: typeGroup }

                ToolButton {
                    id: favButton
                    implicitWidth: 28
                    implicitHeight: 24
                    checkable: true
                    checked: true
                    iconName: 'bookmarks'
                    iconSource: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                    tooltip: qsTr('Show favorite filters')
                    exclusiveGroup: typeGroup
                    onCheckedChanged: {
                        if (checked) {
                            metadatamodel.filter = Shotcut.MetadataModel.FavoritesFilter
                        }
                    }
                }
                ToolButton {
                    id: vidButton
                    implicitWidth: 28
                    implicitHeight: 24
                    checkable: true
                    iconName: 'video-television'
                    iconSource: 'qrc:///icons/oxygen/32x32/devices/video-television.png'
                    tooltip: qsTr('Show video filters')
                    exclusiveGroup: typeGroup
                    onCheckedChanged: {
                        if (checked) {
                            metadatamodel.filter = Shotcut.MetadataModel.VideoFilter
                        }
                    }
                }
                ToolButton {
                    id: audButton
                    implicitWidth: 28
                    implicitHeight: 24
                    checkable: true
                    iconName: 'speaker'
                    iconSource: 'qrc:///icons/oxygen/32x32/actions/speaker.png'
                    tooltip: qsTr('Show audio filters')
                    exclusiveGroup: typeGroup
                    onCheckedChanged: {
                        if (checked) {
                            metadatamodel.filter = Shotcut.MetadataModel.AudioFilter
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                ToolButton {
                    id: closeButton
                    implicitWidth: 28
                    implicitHeight: 24
                    iconName: 'window-close'
                    iconSource: 'qrc:///icons/oxygen/32x32/actions/window-close.png'
                    tooltip: qsTr('Close menu')
                    onClicked: filterWindow.close()
                }
            }
        }
    }
}
