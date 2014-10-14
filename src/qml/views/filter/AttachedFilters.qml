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
    height: 100

    SystemPalette { id: activePalette }

    Component {
        id: filterDelegate
        
        Row {
            height: filterDelegateText.implicitHeight
            ListView.onAdd: { attachedFiltersView.currentIndex = attachedFiltersView.count - 1 }
            
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
                    z: 1
                    hoverEnabled: true
                    onClicked: {
                        attachedFiltersView.currentIndex = index
                        attachedFiltersView.forceActiveFocus()
                    }
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        ListView {
            id: attachedFiltersView
            
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
        }
    }
}