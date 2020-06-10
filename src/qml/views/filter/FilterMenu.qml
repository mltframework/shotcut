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
 
import QtQuick 2.2
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.1
import org.shotcut.qml 1.0 as Shotcut
import Shotcut.Controls 1.0 as ShotcutControls

Rectangle {
    id: filterWindow
    visible: false
    property color checkedColor: Qt.rgba(activePalette.highlight.r, activePalette.highlight.g, activePalette.highlight.b, 0.4)

    signal filterSelected(int index)

    function open() {
        filterWindow.visible = true
        searchField.focus = true
    }

    function close() {
        filterWindow.visible = false
        searchField.text = ''
        searchField.focus = false
    }

    color: activePalette.window

    SystemPalette { id: activePalette }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8

        RowLayout {
            id: searchBar
            Layout.fillWidth: true
            property var savedFilter

            TextField {
                id: searchField
                Layout.fillWidth: true
                focus: true
                placeholderText: qsTr("search")
                text: metadatamodel.search
                onTextChanged: {
                    if (length !== 1 && text !== metadatamodel.search) {
                        metadatamodel.search = text
                    }
                    if (length > 0) {
                        parent.savedFilter = typeGroup.current
                        favButton.checked = vidButton.checked = audButton.checked = false
                    } else {
                        parent.savedFilter.checked = true
                    }
                }
                Keys.onReturnPressed: {
                    menuListView.itemSelected(menuListView.currentIndex)
                    event.accepted = true
                }
                Keys.onEnterPressed: Keys.onReturnPressed(event)
                Keys.onEscapePressed: {
                    if (text !== '')
                        text = ''
                    else
                        filterWindow.close()
                }
                Keys.onUpPressed: menuListView.selectPrevious()
                Keys.onDownPressed: menuListView.selectNext()
            }
            ToolButton {
                id: clearButton
                implicitWidth: 20
                implicitHeight: 20
                iconName: 'edit-clear'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/edit-clear.png'
                tooltip: qsTr('Clear search')
                onClicked: searchField.text = ''
            }
        }

        RowLayout {
            id: toolBar
            Layout.fillWidth: true

            ExclusiveGroup { id: typeGroup }

            ShotcutControls.ToggleButton {
                id: favButton
                checked: true
                implicitWidth: 80
                iconName: 'bookmarks'
                iconSource: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                text: qsTr('Favorite')
                tooltip: qsTr('Show favorite filters')
                exclusiveGroup: typeGroup
                onClicked: if (checked) metadatamodel.filter = Shotcut.MetadataModel.FavoritesFilter
            }
            ShotcutControls.ToggleButton {
                id: vidButton
                implicitWidth: 80
                iconName: 'video-television'
                iconSource: 'qrc:///icons/oxygen/32x32/devices/video-television.png'
                text: qsTr('Video')
                tooltip: qsTr('Show video filters')
                exclusiveGroup: typeGroup
                onClicked: if (checked) metadatamodel.filter = Shotcut.MetadataModel.VideoFilter
            }
            ShotcutControls.ToggleButton {
                id: audButton
                implicitWidth: 80
                iconName: 'speaker'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/speaker.png'
                text: qsTr('Audio')
                tooltip: qsTr('Show audio filters')
                exclusiveGroup: typeGroup
                onClicked: if (checked) metadatamodel.filter = Shotcut.MetadataModel.AudioFilter
            }
            Button { // separator
                enabled: false
                implicitWidth: 1
                implicitHeight: 20
            }
            Button {
                id: closeButton
                iconName: 'window-close'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/window-close.png'
                tooltip: qsTr('Close menu')
                onClicked: filterWindow.close()
            }
            Item {
                Layout.fillWidth: true
            }
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: filterWindow.height - toolBar.height - searchBar.height - parent.anchors.margins * 2

            ListView {
                id: menuListView

                function itemSelected(index) {
                    if (index > -1) {
                        filterWindow.close()
                        filterSelected(index)
                    }
                }

                function selectNext() {
                    do {
                        currentIndex = Math.min(currentIndex + 1, count - 1)
                    } while (currentItem !== null && !currentItem.visible && currentIndex < count - 1)
                }

                function selectPrevious() {
                    do {
                        menuListView.currentIndex = Math.max(menuListView.currentIndex - 1, 0)
                    } while (!menuListView.currentItem.visible && menuListView.currentIndex > 0)
                }

                anchors.fill: parent
                model: metadatamodel
                delegate: FilterMenuDelegate {}
                boundsBehavior: Flickable.StopAtBounds
                currentIndex: -1
                focus: true

                onCountChanged: {
                    currentIndex = -1
                    selectNext()
                }
            }
        }
    }
}
