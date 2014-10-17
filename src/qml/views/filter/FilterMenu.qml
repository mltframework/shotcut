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
import QtQuick.Controls.Styles 1.1

Window {
    id: filterWindow
    
    property bool _showAll: false
    property int _itemHeight: 30
    
    signal filterSelected(int index)
    
    function popup(triggerItem) {
        var menuRect = _menuRect(triggerItem)
        filterWindow.x = menuRect.x
        filterWindow.y = menuRect.y
        filterWindow.height = menuRect.height
        filterWindow.visible = true
        filterWindow.requestActivate()
        menuListView.currentIndex = -1
    }

    function _menuRect(triggerItem) {
        var result = Qt.rect(0, 0, 0, 0)
        var itemPos = triggerItem.mapToItem(null,0,0)
        var triggerPos = Qt.point(itemPos.x + view.pos.x, itemPos.y + view.pos.y)
        var mainWinRect = application.mainWinRect
        
        // Calculate the max possible height of the menu
        var i = 0
        var visibleItems = 0;
        for( i = 0; i < metadatamodel.rowCount(); i++ ) {
            var meta = metadatamodel.get(i)
            if( !meta.isHidden && (meta.isFavorite || _showAll) ) {
                visibleItems++
            }
        }
        var maxHeight = (visibleItems * _itemHeight) + padRect.height + moreButton.height
        result.height = Math.min(maxHeight, mainWinRect.height)
        
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

    color: activePalette.window
    flags: Qt.ToolTip
    width: 200
    height: 200
    visible: false
    
    onActiveFocusItemChanged: {
        visible = activeFocusItem != null
    }
    
    SystemPalette { id: activePalette }
    
    Component {
        id: filterMenuDelegate
        
        Item {
            visible: !hidden && (favorite || _showAll)
            height: visible ? _itemHeight : 0
        
            Row {
                id: filterItemRow
                height: _itemHeight
                
                Button {
                    id: favButton
                    height: parent.height - 5
                    width: height
                    anchors.verticalCenter: parent.verticalCenter
                    opacity: favorite ? 1.0 : 0.5
                    iconName: 'bookmarks'
                    iconSource: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                    onClicked: favorite = !favorite
                }
                
                Item {
                    id: filterItemPad
                    width: 4
                    height: parent.height
                }
                
                Text {
                    id: filterItemText
                    text: name
                    width: menuListView.width - favButton.width - filterItemPad.width
                    color: activePalette.text
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    verticalAlignment: Text.AlignVCenter
                    
                    Rectangle {
                        id: filterItemTextBackground
                        anchors.fill: parent
                        color: filterWindow.color
                        z: filterItemText.z - 1
                    }
        
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        z: 1
                        hoverEnabled: true
                        onClicked: {
                            filterWindow.visible = false
                            filterWindow.filterSelected(index)
                        }
                        onEntered: {
                            filterItemTextBackground.color = activePalette.highlight
                            filterItemText.color = activePalette.highlightedText
                        }
                        onExited: {
                            filterItemTextBackground.color = filterWindow.color
                            filterItemText.color = activePalette.text
                        }
                    }
                }
            }
        }
    }

    Column {
        width: parent.width
                
        ScrollView {
            width: parent.width
            height: filterWindow.height - moreButton.height
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
        
        Rectangle {
            id: padRect
            color: activePalette.base
            height: 2
            width: parent.width
        }
        
        Button {
            id: moreButton
            width: parent.width
            text: qsTr('More')
            onClicked: {
                _showAll = !_showAll
                if (_showAll) {
                    text =  qsTr('Less')
                } else {
                    text = qsTr('More')
                }
            }
        }
    }
}