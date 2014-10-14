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
    
    property bool showAll: false
    
    signal filterSelected(int index)
    
    function popup() {
        var cursorPoint = application.mousePos
        filterWindow.x = cursorPoint.x
        filterWindow.y = cursorPoint.y - menuListView.height / 2
        filterWindow.visible = true
        filterWindow.requestActivate()
        menuListView.currentIndex = -1
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
            visible: !hidden && (favorite || showAll)
            height: visible ? filterItemRow.height : 0
        
            Row {
                id: filterItemRow
                height: 30
                
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
            color: activePalette.base
            height: 2
            width: parent.width
        }
        
        Button {
            id: moreButton
            width: parent.width
            text: qsTr('More')
            onClicked: {
                showAll = !showAll
                if (showAll) {
                    text =  qsTr('Less')
                } else {
                    text = qsTr('More')
                }
            }
        }
    }
}