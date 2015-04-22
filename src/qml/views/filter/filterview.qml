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

import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Rectangle {
    id: root
    
    signal currentFilterRequested(int attachedIndex)
    
    function clearCurrentFilter() {
        filterConfig.source = ""
    }
    
    function setCurrentFilter(index) {
        attachedFilters.setCurrentFilter(index)
        removeButton.selectedIndex = index
        filterConfig.source = metadata ? metadata.qmlFilePath : ""
    }

    color: activePalette.window
    width: 400
    
    onWidthChanged: _setLayout()
    onHeightChanged: _setLayout()
    
    function _setLayout() {
        if (height > width - 200) {
            root.state = "portrait"
        } else {
            root.state = "landscape"
        }
    }
    
    SystemPalette { id: activePalette }
    
    FilterMenu {
        id: filterMenu
        onFilterSelected: {
            attachedfiltersmodel.add(metadatamodel.get(index))
        }
    }

    GridLayout {
        id: attachedContainer
        columns: 3
        anchors.top: parent.top
        anchors.left: parent.left

        Text {
            text: qsTr("Track: %1").arg(attachedfiltersmodel.trackTitle)
            height: visible ? implicitHeight : 0
            visible: attachedfiltersmodel.trackTitle != ""
            wrapMode: Text.Wrap
            color: activePalette.text
            Layout.columnSpan: 3
            Layout.fillWidth: true
        }
        
        AttachedFilters {
            id: attachedFilters
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.fillHeight: true
            onFilterClicked: {
                root.currentFilterRequested(index)
            }
            Text {
                anchors.centerIn: parent
                text: qsTr("Nothing selected")
                color: activePalette.text
                visible: !attachedfiltersmodel.isProducerSelected
            }
        }

        Button {
            id: addButton
            Layout.minimumWidth: height
            iconName: 'list-add'
            enabled: attachedfiltersmodel.ready
            opacity: enabled ? 1.0 : 0.5
            tooltip: qsTr('Add a filter')
            onClicked: filterMenu.popup(addButton)
        }
        Button {
            id: removeButton
            
            property int selectedIndex: -1
            
            Layout.minimumWidth: height
            iconName: 'list-remove'
            enabled: selectedIndex > -1 ? true : false
            opacity: enabled ? 1.0 : 0.5
            tooltip: qsTr('Remove selected filter')
            onClicked: {
                attachedfiltersmodel.remove(selectedIndex)
            }
        }
        Item {
            Layout.fillWidth: true
        }
    }

    Loader {
        id: filterConfig
    }
        
    states: [
        State {
            name: "landscape"
            AnchorChanges {
                target: filterConfig
                anchors.top: root.top
                anchors.bottom: root.bottom
                anchors.left: attachedContainer.right
                anchors.right: root.right
            }
            PropertyChanges { target: attachedContainer; width: 200; height: root.height }
        },
        State {
            name: "portrait"
            AnchorChanges {
                target: filterConfig
                anchors.top: attachedContainer.bottom
                anchors.bottom: root.bottom
                anchors.left: root.left
                anchors.right: root.right
            }
            PropertyChanges { target: attachedContainer; width: root.width; height: 165 }
        }
    ]
}
