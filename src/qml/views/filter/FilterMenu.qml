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

Loader {
    id: menuLoader
    
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
    }
    
    Component {
        id: menuComponent
        Window {
            id: filterWindow
            
            property int _itemHeight: 30
            
            signal itemSelected(int index)

            Behavior on height {
                id: heightBehavior
                enabled: false
                animation: NumberAnimation { duration: 500 }
            }
            
            Behavior on y {
                id: yBehavior
                enabled: false
                animation: NumberAnimation { duration: 500 }
            }

            function popup(triggerItem) {
                var menuRect = _menuRect(triggerItem)
                filterWindow.x = menuRect.x
                filterWindow.y = menuRect.y
                filterWindow.height = menuRect.height
                filterWindow.visible = true
                filterWindow.requestActivate()
                menuListView.currentIndex = -1
            }
            
            function _isVisible(isHidden, isFavorite, isAudio) {
                if(isHidden) return false
                if(!isFavorite && favButton.checked) return false
                if(isAudio && !audButton.checked) return false
                if(!isAudio && !vidButton.checked) return false
                return true
            }
            
            function _maxMenuHeight() {
                // Calculate the max possible height of the menu
                var i = 0
                var visibleItems = 0;
                for( i = 0; i < metadatamodel.rowCount(); i++ ) {
                    var meta = metadatamodel.get(i)
                    if(_isVisible(meta.isHidden, meta.isFavorite, meta.isAudio)) {
                        visibleItems++
                    }
                }
                return (visibleItems * _itemHeight) + toolBar.height
            }
        
            function _menuRect(triggerItem) {
                var result = Qt.rect(0, 0, 0, 0)
                var itemPos = triggerItem.mapToItem(null,0,0)
                var triggerPos = Qt.point(itemPos.x + view.pos.x, itemPos.y + view.pos.y)
                var mainWinRect = application.mainWinRect
                
                result.height = Math.min(_maxMenuHeight(), mainWinRect.height)
                
                // Calculate the y position
                result.y = triggerPos.y - result.height / 2 // Ideal position is centered
                if( result.y < mainWinRect.y ) {
                    // Window would be higher than the application window. Move it down
                    result.y = mainWinRect.y
                } else if( result.y + result.height > mainWinRect.y + mainWinRect.height ) {
                    // Window would be lower than the application window. Move it up
                    result.y =  mainWinRect.y + mainWinRect.height - result.height
                }
                
                // Calculate the x position
                result.x = triggerPos.x
                
                return result
            }
            
            function _resizeMenu() {
                // The goal is to change the size of the menu without moving the toolbar
                // at the bottom of the menu
                var bottomY = filterWindow.y + filterWindow.height
                var mainWinRect = application.mainWinRect
                var newHeight = Math.min(_maxMenuHeight(), bottomY - mainWinRect.y)
                var newY = bottomY - newHeight
                heightBehavior.enabled = true
                yBehavior.enabled = true
                filterWindow.y = newY
                filterWindow.height = newHeight
            }

            color: activePalette.window
            flags: Qt.ToolTip
            width: 220
            height: 200
            visible: false
            
            onActiveFocusItemChanged: {
                visible = activeFocusItem != null
            }
            
            SystemPalette { id: activePalette }
            
            Component {
                id: filterMenuDelegate
                
                Item {
                    visible: _isVisible(hidden, favorite, isAudio)
                    height: visible ? _itemHeight : 0
                
                    Row {
                        id: filterItemRow
                        height: _itemHeight
                        
                        ToolButton {
                            id: favButton
                            implicitWidth: 20
                            implicitHeight: 18
                            anchors.verticalCenter: parent.verticalCenter
                            opacity: favorite ? 1.0 : 0.3
                            iconName: 'bookmarks'
                            iconSource: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                            onClicked: favorite = !favorite
                        }
                        
                        Rectangle {
                            id: filterItemBackground
                            color: menuColorRect.color
                            width: menuListView.width - favButton.width
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            
                            ToolButton {
                                id: filterItemIcon
                                implicitWidth: 20
                                implicitHeight: 18
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                enabled: false
                                iconName: isAudio ? 'speaker' : 'video-television'
                                iconSource: isAudio ? 'qrc:///icons/oxygen/32x32/actions/speaker.png' : 'qrc:///icons/oxygen/32x32/devices/video-television.png'
                            }
                            
                            Label {
                                id: filterItemText
                                anchors.left: filterItemIcon.right
                                anchors.right: filterItemBackground.right
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                text: name
                                color: activePalette.text
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                z: filterItemText.z + 1
                                hoverEnabled: true
                                onClicked: {
                                    filterWindow.visible = false
                                    filterWindow.itemSelected(index)
                                }
                                onEntered: {
                                    filterItemBackground.color = activePalette.highlight
                                    filterItemText.color = activePalette.highlightedText
                                }
                                onExited: {
                                    filterItemBackground.color = menuColorRect.color
                                    filterItemText.color = activePalette.text
                                }
                            }
                        }
                    }
                }
            }
        
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
                        height: filterWindow.height - toolBar.height
                        ListView {
                            id: menuListView
                            anchors.fill: parent
                            model: metadatamodel
                            delegate: filterMenuDelegate
                            boundsBehavior: Flickable.StopAtBounds
                            snapMode: ListView.SnapToItem
                            currentIndex: -1
                            focus: true
                        }
                    }
                    
                    ToolBar {
                        id: toolBar
                        RowLayout {
                            ToolButton {
                                id: favButton
                                implicitWidth: 28
                                implicitHeight: 24
                                checkable: true
                                checked: true
                                iconName: 'bookmarks'
                                iconSource: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                                tooltip: checked ? qsTr('Show All') : qsTr('Show Favorite')
                                onCheckedChanged: _resizeMenu()
                            }
                            ToolButton {
                                id: vidButton
                                implicitWidth: 28
                                implicitHeight: 24
                                checkable: true
                                checked: true
                                iconName: 'video-television'
                                iconSource: 'qrc:///icons/oxygen/32x32/devices/video-television.png'
                                tooltip: checked ? qsTr('Hide Video') : qsTr('Show Video')
                                onClicked: if(!checked) audButton.checked = true
                                onCheckedChanged: _resizeMenu()
                            }
                            ToolButton {
                                id: audButton
                                implicitWidth: 28
                                implicitHeight: 24
                                checkable: true
                                checked: true
                                iconName: 'speaker'
                                iconSource: 'qrc:///icons/oxygen/32x32/actions/speaker.png'
                                tooltip: checked ? qsTr('Hide Audio') : qsTr('Show Audio')
                                onClicked: if(!checked) vidButton.checked = true
                                onCheckedChanged: _resizeMenu()
                            }
                        }
                    }
                }
            }
        }
    }
}