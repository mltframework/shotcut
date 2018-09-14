/*
 * Copyright (c) 2014-2018 Meltytech, LLC
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
import QtGraphicalEffects 1.0
import org.shotcut.qml 1.0 as Shotcut

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

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: filterWindow.height - toolBar.height - searchBar.height - parent.anchors.margins * 2

            ListView {
                id: menuListView

                function itemSelected(index) {
                    filterWindow.close()
                    filterSelected(index)
                }

                anchors.fill: parent
                model: metadatamodel
                delegate: FilterMenuDelegate {}
                boundsBehavior: Flickable.StopAtBounds
                currentIndex: -1
                focus: true
            }
        }

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
                Keys.onEscapePressed: {
                    if (text !== '')
                        text = ''
                    else
                        filterWindow.close()
                }
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

            ToolButton {
                id: favButton
                checkable: true
                implicitWidth: 32
                implicitHeight: 28
                checked: true
                iconName: 'bookmarks'
                iconSource: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                tooltip: qsTr('Show favorite filters')
                exclusiveGroup: typeGroup
                onCheckedChanged: {
                    if (checked) {
                        metadatamodel.filter = Shotcut.MetadataModel.FavoritesFilter
                    }
                }
            }
            ToolButton {
                id: vidButton
                implicitWidth: 32
                implicitHeight: 28
                checkable: true
                iconName: 'video-television'
                iconSource: 'qrc:///icons/oxygen/32x32/devices/video-television.png'
                tooltip: qsTr('Show video filters')
                exclusiveGroup: typeGroup
                onCheckedChanged: {
                    if (checked) {
                        metadatamodel.filter = Shotcut.MetadataModel.VideoFilter
                    }
                }
            }
            ToolButton {
                id: audButton
                implicitWidth: 32
                implicitHeight: 28
                checkable: true
                iconName: 'speaker'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/speaker.png'
                tooltip: qsTr('Show audio filters')
                exclusiveGroup: typeGroup
                onCheckedChanged: {
                    if (checked) {
                        metadatamodel.filter = Shotcut.MetadataModel.AudioFilter
                    }
                }
            }
            Button { // separator
                enabled: false
                implicitWidth: 1
                implicitHeight: 20
            }
            ToolButton {
                id: closeButton
                implicitWidth: 32
                implicitHeight: 28
                iconName: 'window-close'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/window-close.png'
                tooltip: qsTr('Close menu')
                onClicked: filterWindow.close()
            }
            ColorOverlay {
                visible: favButton.checked
                anchors.fill: favButton
                source: favButton
                color: checkedColor
                cached: true
            }
            ColorOverlay {
                visible: vidButton.checked
                anchors.fill: vidButton
                source: vidButton
                color: checkedColor
                cached: true
            }
            ColorOverlay {
                visible: audButton.checked
                anchors.fill: audButton
                source: audButton
                color: checkedColor
                cached: true
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }
}
