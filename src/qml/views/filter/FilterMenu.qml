/*
 * Copyright (c) 2014 Meltytech, LLC
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
import QtQuick.Window 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.1
import 'FilterMenu.js' as Logic

Loader {
    signal filterSelected(int index)
    
    function popup(triggerItem) {
        sourceComponent = menuComponent
        item.popup(triggerItem)
    }
    
    Connections {
        target: item
        onItemSelected: {
            filterSelected(index)
            sourceComponent = null
        }
        onHasFocusChanged: {
            if (!item.hasFocus)
                sourceComponent = null
        }
    }
    
    Component {
        id: menuComponent
        
        Window {
            id: filterWindow
            
            property bool hasFocus: activeFocusItem != null
            
            signal itemSelected(int index)

            function popup(triggerItem) {
                var menuRect = Logic.calcMenuRect(triggerItem, menuListView.showType, toolBar.height + 2)
                filterWindow.x = menuRect.x
                filterWindow.y = menuRect.y
                filterWindow.height = menuRect.height
                filterWindow.visible = true
                filterWindow.requestActivate()
                
            }

            color: Qt.darker(activePalette.window, 1.5) // Border color
            flags: Qt.ToolTip
            width: 220
            height: 200

            Component.onCompleted: {
                menuListView.itemSelected.connect(filterWindow.itemSelected)
            }
            
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
                            
                            property var showType: Logic.visibility.FAVORITE
                            
                            signal itemSelected(int index)
                            
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
                        width: parent.width - 20
                        height: 1
                        color: Qt.darker(activePalette.window, 1.5)
                    }

                    RowLayout {
                        id: toolBar
                        height: 30
                        
                        ExclusiveGroup { id: typeGroup }
                        
                        ToolButton {
                            id: favButton
                            implicitWidth: 28
                            implicitHeight: 24
                            checkable: true
                            checked: true
                            iconName: 'bookmarks'
                            iconSource: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                            tooltip: qsTr('Show Favorite Filters')
                            exclusiveGroup: typeGroup
                            onCheckedChanged: {
                                if (checked) {
                                    menuListView.showType = Logic.visibility.FAVORITE
                                }
                            }
                        }
                        ToolButton {
                            id: gpuButton
                            visible: settings.playerGPU
                            width: visible ? undefined : 0
                            implicitWidth: 28
                            implicitHeight: 24
                            checkable: true
                            iconName: 'cpu'
                            iconSource: 'qrc:///icons/oxygen/32x32/devices/cpu.png'
                            tooltip: qsTr('Show GPU Filters')
                            exclusiveGroup: typeGroup
                            onCheckedChanged: {
                                if (checked) {
                                    menuListView.showType = Logic.visibility.GPU
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
                            tooltip: qsTr('Show Video Filters')
                            exclusiveGroup: typeGroup
                            onCheckedChanged: {
                                if (checked) {
                                    menuListView.showType = Logic.visibility.VIDEO
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
                            tooltip: qsTr('Show Audio Filters')
                            exclusiveGroup: typeGroup
                            onCheckedChanged: {
                                if (checked) {
                                    menuListView.showType = Logic.visibility.AUDIO
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}