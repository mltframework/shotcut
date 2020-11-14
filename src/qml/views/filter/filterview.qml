/*
 * Copyright (c) 2014-2020 Meltytech, LLC
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
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Rectangle {
    id: root
    property int selectedIndex: -1
    signal currentFilterRequested(int attachedIndex)
    
    function clearCurrentFilter() {
        if (filterConfig.item) {
            filterConfig.item.width = 1
            filterConfig.item.height = 1
        }
        filterConfig.source = ""
    }
    
    function setCurrentFilter(index) {
        clearCurrentFilter()
        attachedFilters.setCurrentFilter(index)
        selectedIndex = index
        filterConfig.source = metadata ? metadata.qmlFilePath : ""
    }

    function openFilterMenu() {
        if (attachedfiltersmodel.isProducerSelected)
            filterMenu.open()
    }

    color: activePalette.window
    width: 400

    onWidthChanged: _setLayout()
    onHeightChanged: _setLayout()
    
    function _setLayout() {
        if (height > width - attachedFilters.minimumWidth) {
            root.state = "portrait"
        } else {
            root.state = "landscape"
        }
    }
    
    SystemPalette { id: activePalette }
    
    Rectangle {
        id: titleBackground
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: titleLabel.bottom
            topMargin: 10
            leftMargin: 10
            rightMargin: 10
        }
        color: activePalette.highlight
        visible: attachedfiltersmodel.producerTitle != ""
    }

    Label {
        id: titleLabel
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 10
            leftMargin: 10
            rightMargin: 10
        }
        text: attachedfiltersmodel.producerTitle
        elide: Text.ElideLeft
        color: activePalette.highlightedText
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
    }


    GridLayout {
        id: attachedContainer
        columns: children.length - 1
        anchors {
            top: titleBackground.bottom
            left: parent.left
            leftMargin: 10
            rightMargin: 10
            topMargin: 6
            bottomMargin: 4
        }

        AttachedFilters {
            id: attachedFilters
            property int minimumWidth: application.OS === 'Windows'? 350 : 250
            Layout.columnSpan: parent.columns
            Layout.fillWidth: true
            Layout.fillHeight: true
            onFilterClicked: {
                root.currentFilterRequested(index)
            }
            Label {
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
            iconSource: 'qrc:///icons/oxygen/32x32/actions/list-add.png'
            enabled: attachedfiltersmodel.isProducerSelected
            opacity: enabled ? 1.0 : 0.5
            tooltip: qsTr('Add a filter')
            onClicked: {
                if (application.confirmOutputFilter()) {
                    filterMenu.open()
                }
            }
        }
        Button {
            id: removeButton
            Layout.minimumWidth: height
            iconName: 'list-remove'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
            enabled: selectedIndex > -1
            opacity: enabled ? 1.0 : 0.5
            tooltip: qsTr('Remove selected filter')
            onClicked: {
                attachedfiltersmodel.remove(selectedIndex)
            }
        }
        Button { // separator
            enabled: false
            implicitWidth: 1
            implicitHeight: 20
        }
        Button {
            id: copyButton
            Layout.minimumWidth: height
            iconName: 'edit-copy'
            enabled: selectedIndex > -1
            opacity: enabled ? 1.0 : 0.5
            iconSource: 'qrc:///icons/oxygen/32x32/actions/edit-copy.png'
            tooltip: qsTr('Copy the filters')
            onClicked: application.copyFilters()
        }
        Button {
            id: pasteButton
            Layout.minimumWidth: height
            enabled: application.hasFiltersOnClipboard && attachedfiltersmodel.isProducerSelected
            opacity: enabled ? 1.0 : 0.5
            iconName: 'edit-paste'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/edit-paste.png'
            tooltip: qsTr('Paste filters')
            onClicked: application.pasteFilters()
        }
        Button { // separator
            enabled: false
            implicitWidth: 1
            implicitHeight: 20
        }
        Button {
            id: moveUpButton
            Layout.minimumWidth: height
            enabled: selectedIndex > 0
            opacity: enabled ? 1.0 : 0.5
            iconName: 'lift'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/lift.png'
            tooltip: qsTr('Move filter up')
            onClicked: attachedfiltersmodel.move(selectedIndex, --selectedIndex)
        }
        Button {
            id: moveDownButton
            Layout.minimumWidth: height
            enabled: selectedIndex > -1 && selectedIndex + 1 < attachedfiltersmodel.rowCount()
            opacity: enabled ? 1.0 : 0.5
            iconName: 'overwrite'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/overwrite.png'
            tooltip: qsTr('Move filter down')
            onClicked: attachedfiltersmodel.move(selectedIndex, ++selectedIndex)
        }
        Button { // separator
            enabled: false
            implicitWidth: 1
            implicitHeight: 20
        }
        Button {
            id: deselectButton
            Layout.minimumWidth: height
            iconName: 'window-close'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/window-close.png'
            enabled: selectedIndex > -1
            opacity: enabled ? 1.0 : 0.5
            tooltip: qsTr('Deselect the filter')
            onClicked: {
                clearCurrentFilter()
                attachedFilters.setCurrentFilter(-2)
                selectedIndex = -1
                filter.deselect()
            }
        }
        Item {
            Layout.fillWidth: true
        }
    }

    ScrollView {
        id: filterConfigScrollView
        function expandWidth() {
            if (filterConfig.item) {
                filterConfig.item.width =
                    Math.max(filterConfig.minimumWidth,
                             filterConfigScrollView.width - 20 /* scroll bar */)
            }
        }
        onWidthChanged: expandWidth()
        Loader {
            id: filterConfig
            enabled: !filterMenu.visible
            property int minimumWidth: 0
            onLoaded: {
                minimumWidth = item.width
                filterConfigScrollView.expandWidth()
            }
        }
    }
        
    FilterMenu {
        id: filterMenu
        anchors {
            top: titleBackground.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: attachedContainer.anchors.topMargin
        }
        onFilterSelected: {
            attachedfiltersmodel.add(metadatamodel.get(index))
        }
    }

    states: [
        State {
            name: "landscape"
            AnchorChanges {
                target: filterConfigScrollView
                anchors {
                    top: titleBackground.bottom
                    bottom: root.bottom
                    left: attachedContainer.right
                    right: root.right
                }
            }
            PropertyChanges {
                target: attachedContainer
                width: attachedFilters.minimumWidth
                height: root.height -
                    titleBackground.height - titleBackground.anchors.topMargin - titleBackground.anchors.bottomMargin -
                    attachedContainer.anchors.topMargin - attachedContainer.anchors.bottomMargin
            }
        },
        State {
            name: "portrait"
            AnchorChanges {
                target: filterConfigScrollView
                anchors {
                    top: attachedContainer.bottom
                    bottom: root.bottom
                    left: root.left
                    right: root.right
                }
            }
            PropertyChanges {
                target: attachedContainer
                width: titleBackground.width
                height: 165
            }
        }
    ]

    Connections {
        target: attachedfiltersmodel
        onIsProducerSelectedChanged: filterMenu.close()
    }
}
