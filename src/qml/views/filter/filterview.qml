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
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut
import org.shotcut.qml 1.0 as Shotcut

Rectangle {
    id: root

    property int selectedIndex: Shotcut.Filter.NoCurrentFilter

    signal currentFilterRequested(int attachedIndex)

    function clearCurrentFilter() {
        if (filterConfig.item) {
            filterConfig.item.width = 1;
            filterConfig.item.height = 1;
        }
        filterConfig.source = "";
    }

    function setCurrentFilter(index) {
        clearCurrentFilter();
        attachedFilters.setCurrentFilter(index);
        selectedIndex = index;
        filterConfig.source = metadata ? metadata.qmlFilePath : "";
    }

    function openFilterMenu() {
        if (attachedfiltersmodel.isProducerSelected)
            filterMenu.open();

    }

    function _setLayout() {
        if (height > width - attachedFilters.minimumWidth)
            root.state = "portrait";
        else
            root.state = "landscape";
    }

    color: activePalette.window
    width: 400
    onWidthChanged: _setLayout()
    onHeightChanged: _setLayout()
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
                height: root.height - titleBackground.height - titleBackground.anchors.topMargin - titleBackground.anchors.bottomMargin - attachedContainer.anchors.topMargin - attachedContainer.anchors.bottomMargin
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

    SystemPalette {
        id: activePalette
    }

    Rectangle {
        id: titleBackground

        color: activePalette.highlight
        visible: attachedfiltersmodel.producerTitle != ""

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: titleLabel.bottom
            topMargin: 10
            leftMargin: 10
            rightMargin: 10
        }

    }

    Label {
        id: titleLabel

        text: attachedfiltersmodel.producerTitle
        elide: Text.ElideLeft
        color: activePalette.highlightedText
        font.bold: true
        horizontalAlignment: Text.AlignHCenter

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 10
            leftMargin: 10
            rightMargin: 10
        }

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

            property int minimumWidth: application.OS === 'Windows' ? 350 : 250

            Layout.columnSpan: parent.columns
            Layout.fillWidth: true
            Layout.fillHeight: true
            onFilterClicked: (index)=> {
                root.currentFilterRequested(index);
            }

            Label {
                anchors.centerIn: parent
                text: qsTr("Nothing selected")
                color: activePalette.text
                visible: !attachedfiltersmodel.isProducerSelected
            }

        }

        Shotcut.Button {
            id: addButton

            implicitWidth: height
            icon.name: 'list-add'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/list-add.png'
            enabled: attachedfiltersmodel.isProducerSelected
            opacity: enabled ? 1 : 0.5
            onClicked: {
                if (application.confirmOutputFilter())
                    filterMenu.open();

            }

            Shotcut.HoverTip {
                text: qsTr('Add a filter')
            }

        }

        Shotcut.Button {
            id: removeButton

            implicitWidth: height
            icon.name: 'list-remove'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
            enabled: selectedIndex > Shotcut.Filter.NoCurrentFilter
            opacity: enabled ? 1 : 0.5
            onClicked: {
                attachedfiltersmodel.remove(selectedIndex);
            }

            Shotcut.HoverTip {
                text: qsTr('Remove selected filter')
            }

        }
        // separator

        Shotcut.Button {
            enabled: false
            implicitWidth: 1
            implicitHeight: 20
        }

        Shotcut.Button {
            id: copyButton

            implicitWidth: height
            icon.name: 'edit-copy'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-copy.png'
            enabled: selectedIndex > Shotcut.Filter.NoCurrentFilter
            opacity: enabled ? 1 : 0.5
            onClicked: application.copyFilters()

            Shotcut.HoverTip {
                text: qsTr('Copy the filters')
            }

        }

        Shotcut.Button {
            id: pasteButton

            implicitWidth: height
            enabled: attachedfiltersmodel.isProducerSelected
            opacity: enabled ? 1 : 0.5
            icon.name: 'edit-paste'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-paste.png'
            onClicked: application.pasteFilters()

            Shotcut.HoverTip {
                text: qsTr('Paste filters')
            }

        }
        // separator

        Shotcut.Button {
            enabled: false
            implicitWidth: 1
            implicitHeight: 20
        }

        Shotcut.Button {
            id: moveUpButton

            implicitWidth: height
            enabled: selectedIndex > 0
            opacity: enabled ? 1 : 0.5
            icon.name: 'lift'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/lift.png'
            onClicked: attachedfiltersmodel.move(selectedIndex, --selectedIndex)

            Shotcut.HoverTip {
                text: qsTr('Move filter up')
            }

        }

        Shotcut.Button {
            id: moveDownButton

            implicitWidth: height
            enabled: selectedIndex > Shotcut.Filter.NoCurrentFilter && selectedIndex + 1 < attachedfiltersmodel.rowCount()
            opacity: enabled ? 1 : 0.5
            icon.name: 'overwrite'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/overwrite.png'
            onClicked: attachedfiltersmodel.move(selectedIndex, ++selectedIndex)

            Shotcut.HoverTip {
                text: qsTr('Move filter down')
            }

        }
        // separator

        Shotcut.Button {
            enabled: false
            implicitWidth: 1
            implicitHeight: 20
        }

        Shotcut.Button {
            id: deselectButton

            implicitWidth: height
            icon.name: 'window-close'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/window-close.png'
            enabled: selectedIndex > Shotcut.Filter.NoCurrentFilter
            opacity: enabled ? 1 : 0.5
            onClicked: {
                clearCurrentFilter();
                attachedFilters.setCurrentFilter(Shotcut.Filter.DeselectCurrentFilter);
                selectedIndex = Shotcut.Filter.NoCurrentFilter;
                filter.deselect();
            }

            Shotcut.HoverTip {
                text: qsTr('Deselect the filter')
            }

        }

        Item {
            Layout.fillWidth: true
        }

    }

    Flickable {
        // scroll bar

        id: filterConfigScrollView

        function expandWidth() {
            if (filterConfig.item)
                filterConfig.item.width = Math.max(filterConfig.minimumWidth, filterConfigScrollView.width - 20);

        }

        clip: true
        interactive: false
        anchors.bottomMargin: 16
        anchors.rightMargin: 16
        contentWidth: filterConfig.item ? filterConfig.item.width + 16 : 0
        contentHeight: filterConfig.item ? filterConfig.item.height + 16 : 0
        onWidthChanged: expandWidth()

        Loader {
            id: filterConfig

            property int minimumWidth: 0

            enabled: !filterMenu.visible
            onLoaded: {
                minimumWidth = item.width;
                filterConfigScrollView.expandWidth();
            }
        }

        ScrollBar.horizontal: ScrollBar {
            height: 16
            policy: ScrollBar.AlwaysOn
            visible: filterConfigScrollView.contentWidth > filterConfigScrollView.width
            parent: filterConfigScrollView.parent
            anchors.top: filterConfigScrollView.bottom
            anchors.left: filterConfigScrollView.left
            anchors.right: filterConfigScrollView.right

            background: Rectangle {
                color: parent.palette.alternateBase
            }

        }

        ScrollBar.vertical: ScrollBar {
            width: 16
            policy: ScrollBar.AlwaysOn
            visible: filterConfigScrollView.contentHeight > filterConfigScrollView.height
            parent: filterConfigScrollView.parent
            anchors.top: filterConfigScrollView.top
            anchors.left: filterConfigScrollView.right
            anchors.bottom: filterConfigScrollView.bottom

            background: Rectangle {
                color: parent.palette.alternateBase
            }

        }

    }

    FilterMenu {
        id: filterMenu

        z: 1
        onFilterSelected: (index)=> {
            attachedfiltersmodel.add(metadatamodel.get(index));
        }

        anchors {
            top: titleBackground.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: attachedContainer.anchors.topMargin
        }

    }

    Connections {
        function onIsProducerSelectedChanged() {
            filterMenu.close();
        }

        target: attachedfiltersmodel
    }

}
