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
import QtQml.Models 2.1

Rectangle {
    id: attachedFilters
    
    property int selectedIndex: attachedFiltersView.modelCurrentIndex
    
    color: activePalette.base

    SystemPalette { id: activePalette }

    Component {
        id: filterDelegate
        
        Rectangle {
            id: background
            
            // Trick to make the model item available to the dragItem
            property var modelData: model
            property var viewData: ListView

            height: filterDelegateText.implicitHeight
            width: parent ? parent.width : undefined
            color: "transparent"
            border.width: 2
            border.color: attachedFiltersView.modelDragTarget == model.index ? activePalette.highlight : "transparent"

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
                
                Label { 
                    id: filterDelegateText
                    text: model.display
                    color: attachedFiltersView.modelCurrentIndex == model.index ? activePalette.highlightedText : activePalette.windowText
                    width: attachedFiltersView.width - 23
        
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        onDoubleClicked: {
                            model.checkState = !model.checkState
                            filterDelegateCheck.checkedState = model.checkState
                        }
                    }
                }
            }
        }
    }
    
    DelegateModel {
        id: visualModel
        model: attachedfiltersmodel
        delegate: filterDelegate
        
        signal modelReset()
        signal itemAboutToBeRemoved(int index)
        signal itemAdded(int index)
        
        function getModelIndex(visualIndex) {
            if(visualIndex < 0 || visualIndex >= count) return -1
            var item = items.get(visualIndex)
            if (item) {
                return item.model.index
            } else {
                return -1
            }
        }
        
        function getVisualIndex(modelIndex) {
            for( var i = 0; i < items.count; i++ ) {
                var item = items.get(i)
                if(item.model.index == modelIndex) {
                    return i
                }
            }
            return -1
        }
        
        function _itemIsLessThan(a, b) {
            // First sort by type: gpu, video, audio.
            // Then sort by index.
            if (a.model.type == "gpu" && b.model.type != "gpu") {
                return true
            } else if (a.model.type == "video" && b.model.type == "audio") {
                return true
            } else if( a.model.type == b.model.type && a.model.index < b.model.index ) {
                return true
            } else {
                return false
            }
        }
        
        function _sort() {
            items.changed.disconnect(_sort)
            
            for( var i = 0; i < items.count; i++ ) {
                var item = items.get(i)
                var newIndex = i;

                for( var j = i - 1; j >= 0; j-- ) {
                    var prevItem = items.get(j)
                    if( _itemIsLessThan(item, prevItem) ) {
                        newIndex = j
                    } else {
                        break
                    }
                }
                
                if( newIndex != i ) items.move(i, newIndex, 1)
            }
            
            items.changed.connect(_sort)
        }
        
        Component.onCompleted: {
            model.rowsMoved.connect(_sort)
            model.modelReset.connect(modelReset)
            model.rowsAboutToBeRemoved.connect(rowsAboutToBeRemoved)
            model.rowsInserted.connect(rowsAdded)
            items.changed.connect(_sort)
            _sort()
        }
        
        function rowsAboutToBeRemoved(parent, row) {
            itemAboutToBeRemoved(getVisualIndex(row))
        }
        
        function rowsAdded(parent, row) {
            itemAdded(getVisualIndex(row))
        }
    }
    
    Component {
        id: sectionDelegate

        Item {
            height: sectionText.implicitHeight + 4
            width: parent ? parent.width : undefined
            Rectangle {
                anchors.fill: parent
                color: activePalette.alternateBase
            }
            Label {
                id: sectionText
                anchors.fill: parent
                anchors.topMargin: 2
                anchors.leftMargin: 4
                text: section
                color: activePalette.windowText
                font.bold: true
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        ListView {
            id: attachedFiltersView
            
            property var visualDragTarget: -1
            property var modelDragTarget: visualModel.getModelIndex(visualDragTarget)
            property var modelCurrentIndex: visualModel.getModelIndex(currentIndex)
            
            function setCurrentIndexAfterReset() {
                currentIndex = -1
                positionViewAtBeginning()
            }
            
            function setCurrentIndexBeforeRemove(visualIndex) {
                if (currentIndex == count - 1 ) currentIndex--
                positionViewAtIndex(currentIndex, ListView.Contain)
            }
            
            function setCurrentIndexAfterAdd(visualIndex) {
                currentIndex = visualIndex
                positionViewAtIndex(currentIndex, ListView.Contain)
            }
            
            anchors.fill: parent
            model: visualModel
            boundsBehavior: Flickable.StopAtBounds
            snapMode: ListView.SnapToItem
            currentIndex: -1
            highlight: Rectangle { 
                        color: activePalette.highlight
                        width: parent ? parent.width : undefined
                       }
            focus: true
            section.property: "typeDisplay"
            section.delegate: sectionDelegate
            spacing: 4
            highlightMoveVelocity: 1000

            Component.onCompleted: {
                model.modelReset.connect(setCurrentIndexAfterReset)
                model.itemAboutToBeRemoved.connect(setCurrentIndexBeforeRemove)
                model.itemAdded.connect(setCurrentIndexAfterAdd)
            }

            MouseArea {
                property int oldIndex: -1
                property var grabPos: Qt.point(0,0)
                property var grabSection: ""
                
                function beginDrag() {
                    var grabbedItem = attachedFiltersView.itemAt(mouseX, mouseY)
                    oldIndex = attachedFiltersView.indexAt(mouseX, mouseY)
                    grabPos = Qt.point(mouseX - grabbedItem.x, mouseY - grabbedItem.y)
                    grabSection = grabbedItem.viewData.section
                    dragItem.model = grabbedItem.modelData
                    dragItem.sourceComponent = filterDelegate
                    dragItem.x = mouseX - grabPos.x
                    dragItem.y = mouseY - grabPos.y
                    attachedFiltersView.currentIndex = oldIndex
                    attachedFiltersView.visualDragTarget = oldIndex
                    cursorShape = Qt.DragMoveCursor
                    autoScrollTimer.running = true
                }
                
                function endDrag() {
                    oldIndex = -1
                    attachedFiltersView.visualDragTarget = -1
                    dragItem.sourceComponent = null
                    cursorShape = Qt.ArrowCursor
                    autoScrollTimer.running = false
                }
                
                function updateDragTarget() {
                    var mouseItem = attachedFiltersView.itemAt(mouseX, mouseY)
                    if(mouseItem && mouseItem.viewData.section == grabSection) {
                        dragItem.x = mouseX - grabPos.x
                        dragItem.y = mouseY - grabPos.y - attachedFiltersView.contentY
                        attachedFiltersView.visualDragTarget = attachedFiltersView.indexAt(mouseX, mouseY)
                    } 
                }
                
                propagateComposedEvents: true
                anchors.fill: attachedFiltersView.contentItem
                z: 1
                
                onClicked: {
                    attachedFiltersView.currentIndex = attachedFiltersView.indexAt(mouseX, mouseY)
                    mouse.accepted = false
                }
                
                onPressAndHold: {
                    if (oldIndex == -1) {
                        beginDrag()
                    }
                }
                onReleased: {
                    if(oldIndex != -1 
                    && attachedFiltersView.visualDragTarget != -1
                    && oldIndex != attachedFiltersView.visualDragTarget) {
                        attachedfiltersmodel.move(visualModel.getModelIndex(oldIndex), attachedFiltersView.modelDragTarget)
                        attachedFiltersView.currentIndex = attachedFiltersView.visualDragTarget
                    }
                    endDrag()
                    mouse.accepted = true;
                }
                onPositionChanged: {
                    if (oldIndex == -1) {
                        beginDrag()
                    }
                    updateDragTarget()
                }
                onCanceled: endDrag()
                
                Timer {
                    id: autoScrollTimer
                    interval: 500; 
                    running: false;
                    repeat: true
                    onTriggered: {
                        // Make sure previous and next indices are always visible
                        var nextIndex = attachedFiltersView.visualDragTarget + 1
                        var prevIndex = attachedFiltersView.visualDragTarget - 1
                        if( nextIndex < attachedFiltersView.count ) {
                            attachedFiltersView.positionViewAtIndex(nextIndex, ListView.Contain)
                            parent.updateDragTarget()
                        } else if( prevIndex >= 0 ) {
                            attachedFiltersView.positionViewAtIndex(prevIndex, ListView.Contain)
                            parent.updateDragTarget()
                        }

                    }
                }
            }
            
            Loader {
                id: dragItem
                
                // Emulate the delegate properties added by ListView
                property var model : Object()
                
                onLoaded: {
                    item.color = activePalette.highlight
                    item.opacity = 0.5
                    item.width = attachedFiltersView.width
                }
            }
        }
    }
}
