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

import QtQuick 2.0
import QtQuick.Controls 1.1

Rectangle {
    id: attachedFilters
    
    property int selectedIndex: attachedFiltersView.currentIndex
    
    color: activePalette.base

    SystemPalette { id: activePalette }

    Component {
        id: filterDelegate
        
        Rectangle {
            id: background
            
            // Trick to make the model item available to the dragItem
            property var rowData: model
            
            ListView.onAdd: { attachedFiltersView.currentIndex = attachedFiltersView.count - 1 }

            height: filterDelegateText.implicitHeight
            width: parent ? parent.width : undefined
            color: "transparent"
            border.width: 2
            border.color: attachedFiltersView.dragTarget == index ? activePalette.highlight : "transparent"

            Row {
                height: parent.height
                
                Item {
                    width: 4
                    height: parent.height
                }
                
                CheckBox {
                    id: filterDelegateCheck
                    width: 15
                    anchors.verticalCenter: parent.verticalCenter
                    checkedState: model.checkState
                    onClicked: {
                        model.checkState = !model.checkState
                    }
                }
                
                Item {
                    width: 4
                    height: parent.height
                }
                
                Text { 
                    id: filterDelegateText
                    text: model.display
                    color: attachedFiltersView.currentIndex == index ? activePalette.highlightedText : activePalette.windowText
                    width: attachedFiltersView.width - 23
        
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        onClicked: {
                            attachedFiltersView.currentIndex = index
                        }
                        onDoubleClicked: {
                            model.checkState = !model.checkState
                            filterDelegateCheck.checkedState = model.checkState
                        }
                    }
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        ListView {
            id: attachedFiltersView
            
            property var dragTarget: -1
            
            function handleReset () {
                currentIndex = -1
            }
            
            anchors.fill: parent
            model: attachedfiltersmodel
            delegate: filterDelegate
            boundsBehavior: Flickable.StopAtBounds
            snapMode: ListView.SnapToItem
            currentIndex: -1
            highlight: Rectangle { 
                        color: activePalette.highlight
                        width: parent ? parent.width : undefined
                       }
            focus: true
            
            Component.onCompleted: {
                model.modelReset.connect(handleReset)
            }
            
            MouseArea {
                property int oldIndex: -1
                property var grabPos: Qt.point(0,0)
                
                function beginDrag() {
                    var grabbedItem = attachedFiltersView.itemAt(mouseX, mouseY)
                    oldIndex = attachedFiltersView.indexAt(mouseX, mouseY)
                    grabPos = Qt.point(mouseX - grabbedItem.x, mouseY - grabbedItem.y)
                    dragItem.index = oldIndex
                    dragItem.model = grabbedItem.rowData
                    dragItem.sourceComponent = filterDelegate
                    dragItem.x = mouseX - grabPos.x
                    dragItem.y = mouseY - grabPos.y
                    attachedFiltersView.currentIndex = oldIndex
                    cursorShape = Qt.DragMoveCursor
                    autoScrollTimer.running = true
                }
                
                function endDrag() {
                    oldIndex = -1
                    attachedFiltersView.dragTarget = -1
                    dragItem.sourceComponent = null
                    cursorShape = Qt.ArrowCursor
                    autoScrollTimer.running = false
                }
                
                propagateComposedEvents: true
                anchors.fill: parent
                z: 1
                
                onPressAndHold: {
                    if (oldIndex == -1) {
                        beginDrag()
                    }
                }
                onReleased: {
                    if(oldIndex != -1 
                    && attachedFiltersView.dragTarget != -1
                    && oldIndex != attachedFiltersView.dragTarget) {
                        attachedfiltersmodel.move(oldIndex, attachedFiltersView.dragTarget)
                    }
                    endDrag()
                    mouse.accepted = true;
                }
                onPositionChanged: {
                    if (oldIndex == -1) {
                        beginDrag()
                    }
                    dragItem.x = mouseX - grabPos.x
                    dragItem.y = mouseY - grabPos.y
                    attachedFiltersView.dragTarget = attachedFiltersView.indexAt(mouseX, mouseY + attachedFiltersView.contentY)
                }
                onCanceled: endDrag()
                
                Timer {
                    id: autoScrollTimer
                    interval: 500; 
                    running: false;
                    repeat: true
                    onTriggered: {
                        // Make sure previous and next indices are always visible
                        var nextIndex = attachedFiltersView.dragTarget + 1
                        var prevIndex = attachedFiltersView.dragTarget - 1
                        if( nextIndex < attachedFiltersView.count ) {
                            attachedFiltersView.positionViewAtIndex(nextIndex, ListView.Contain)
                        }
                        if( prevIndex >= 0 ) {
                            attachedFiltersView.positionViewAtIndex(prevIndex, ListView.Contain)
                        }
                        attachedFiltersView.dragTarget = attachedFiltersView.indexAt(parent.mouseX, parent.mouseY + attachedFiltersView.contentY)
                    }
                }
            }
            
            Loader {
                id: dragItem
                
                // Emulate the delegate properties added by ListView
                property var model : Object()
                property int index
                
                onLoaded: {
                    item.color = activePalette.highlight
                    item.opacity = 0.5
                    item.width = attachedFiltersView.width
                }
            }
        }
    }
}