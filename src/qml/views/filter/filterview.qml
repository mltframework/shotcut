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
    
    signal attachFilterRequested(int metadataIndex)
    signal removeFilterRequested(int attachedIndex)
    signal currentFilterRequested(int attachedIndex)

    color: activePalette.window
    width: 400
    
    SystemPalette { id: activePalette }
    
    FilterMenu {
        id: filterMenu
        onFilterSelected: {
            root.attachFilterRequested(index)
        }
    }

    GridLayout {
        columns: 2
        anchors.fill: parent
        anchors.margins: 8

        AttachedFilters {
            id: attachedFilters
            Layout.columnSpan: 2
            Layout.fillWidth: true
            width: parent.width
            height: 100
            anchors.top: parent.top
            onSelectedIndexChanged: {
                root.currentFilterRequested(selectedIndex)
                filterConfig.source = metadata ? metadata.qmlFilePath : ""
            }
        }

        Button {
            id: addButton
            Layout.fillWidth: true
            iconName: 'list-add'
            tooltip: qsTr('Add a Filter')
            onClicked: filterMenu.popup(addButton)
        }
        Button {
            id: removeButton
            Layout.fillWidth: true
            iconName: 'list-remove'
            enabled: attachedFilters.selectedIndex > -1 ? true : false
            opacity: enabled ? 1.0 : 0.5
            tooltip: qsTr('Remove Selected Filter')
            onClicked: {
                console.log("Remove Filter Requested", attachedFilters.selectedIndex) 
                root.removeFilterRequested(attachedFilters.selectedIndex)
            }
        }

        Item {
            Layout.columnSpan: 2
            Layout.fillHeight: true;
            Layout.fillWidth: true;
            
            Loader {
                id: filterConfig
                anchors.fill: parent
            }
        }
    }
}