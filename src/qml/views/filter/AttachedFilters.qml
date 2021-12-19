/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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
import QtQml.Models 2.12
import org.shotcut.qml 1.0 as Shotcut

Rectangle {
    id: attachedFilters
    
    signal filterClicked(int index)
    
    function setCurrentFilter(index) {
        indexDelay.index = index
        indexDelay.running = true
    }
    
    Timer {
        id: indexDelay
        property int index: 0
        interval: 1
        onTriggered: {
            // Delay the index setting to allow model updates to complete
            attachedFiltersView.currentIndex = index
        }
    }
    
    color: activePalette.base

    SystemPalette { id: activePalette }

    Component {
        id: filterDelegate
        
        Rectangle {
            id: background
            
            // Trick to make the model item available to the dragItem
            property var modelData: model
            property var viewData: ListView
            property int _dragTarget: ListView.view ? ListView.view.dragTarget : -1
            property int _currentIndex: ListView.currentIndex ? ListView.currentIndex : -1

            height: filterDelegateText.implicitHeight
            width: parent ? parent.width : undefined
            color: "transparent"
            border.width: 2
            border.color: _dragTarget === model.index ? activePalette.highlight : "transparent"

            Row {
                height: parent.height
                
                Item {
                    width: 4
                    height: parent.height
                }
                
                CheckBox {
                    id: filterDelegateCheck
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: model.pluginType != Shotcut.Metadata.Link
                    opacity: enabled ? 1.0 : 0.5
                    checkState: model.checkState
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            model.checkState = !model.checkState
                        }
                    }
                }
                
                Item {
                    width: 4
                    height: parent.height
                }
                
                Label { 
                    id: filterDelegateText
                    text: model.display
                    color: _currentIndex === model.index ? activePalette.highlightedText : activePalette.windowText
                    width: background.ListView.width - 23
        
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        onDoubleClicked: {
                            model.checkState = !model.checkState
                        }
                        onClicked: filterClicked(model.index)
                    }
                }
            }
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
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.horizontal.height: 0
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn
        ScrollBar.vertical.visible: contentHeight > height
        ScrollBar.vertical.width: 16

        ListView {
            id: attachedFiltersView
            
            property int dragTarget: -1
            
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
            section.property: "typeDisplay"
            section.delegate: sectionDelegate
            spacing: 4
            highlightMoveVelocity: 1000

            Component.onCompleted: {
                model.modelReset.connect(positionViewAtBeginning)
            }
            
            onCurrentIndexChanged: {
                possiblySelectFirstFilter();
                positionViewAtIndex(currentIndex, ListView.Contain);
            }
            onCountChanged: possiblySelectFirstFilter();

            function possiblySelectFirstFilter() {
                if (count > 0 && currentIndex == -1) {
                    currentIndex = 0;
                    filterClicked(currentIndex);
                }
            }

            MouseArea {
                property int oldIndex: -1
                property point grabPos: Qt.point(0,0)
                property string grabSection: ""
                
                function beginDrag() {
                    var grabbedItem = attachedFiltersView.itemAt(mouseX, mouseY)
                    oldIndex = attachedFiltersView.indexAt(mouseX, mouseY)
                    grabPos = Qt.point(mouseX - grabbedItem.x, mouseY - grabbedItem.y)
                    grabSection = grabbedItem.viewData.section
                    dragItem.model = grabbedItem.modelData
                    dragItem.sourceComponent = filterDelegate
                    dragItem.x = mouseX - grabPos.x
                    dragItem.y = mouseY - grabPos.y
                    filterClicked(oldIndex)
                    attachedFiltersView.dragTarget = oldIndex
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
                
                function updateDragTarget() {
                    var mouseItem = attachedFiltersView.itemAt(mouseX, mouseY)
                    if (mouseItem && mouseItem.viewData.section === grabSection) {
                        dragItem.x = mouseX - grabPos.x
                        dragItem.y = mouseY - grabPos.y - attachedFiltersView.contentY
                        attachedFiltersView.dragTarget = attachedFiltersView.indexAt(mouseX, mouseY)
                    } 
                }
                
                propagateComposedEvents: true
                anchors.fill: attachedFiltersView.contentItem
                z: 1
                
                onClicked: {
                    filterClicked(attachedFiltersView.indexAt(mouseX, mouseY))
                    mouse.accepted = false
                }
                
                onPressAndHold: {
                    if (oldIndex === -1) {
                        beginDrag()
                    }
                }
                onReleased: {
                    if (oldIndex !== -1
                            && attachedFiltersView.dragTarget !== -1
                            && oldIndex !== attachedFiltersView.dragTarget) {
                        attachedfiltersmodel.move(oldIndex, attachedFiltersView.dragTarget)
                    }
                    endDrag()
                    mouse.accepted = true;
                }
                onPositionChanged: {
                    if (oldIndex === -1) {
                        beginDrag()
                    }
                    updateDragTarget()
                }
                onCanceled: endDrag()
                
                Timer {
                    id: autoScrollTimer
                    interval: 500
                    running: false
                    repeat: true
                    onTriggered: {
                        // Make sure previous and next indices are always visible
                        var nextIndex = attachedFiltersView.dragTarget + 1
                        var prevIndex = attachedFiltersView.dragTarget - 1
                        if (nextIndex < attachedFiltersView.count) {
                            attachedFiltersView.positionViewAtIndex(nextIndex, ListView.Contain)
                            parent.updateDragTarget()
                        }
                        if (prevIndex >= 0) {
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
