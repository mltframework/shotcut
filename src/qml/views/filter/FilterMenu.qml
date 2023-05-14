/*
 * Copyright (c) 2014-20232 Meltytech, LLC
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut
import org.shotcut.qml as Shotcut

Rectangle {
    id: filterWindow

    signal filterSelected(int index)

    function open() {
        filterWindow.visible = true;
        searchField.focus = true;
    }

    function close() {
        filterWindow.visible = false;
        searchField.text = '';
        searchField.focus = false;
    }

    visible: false
    onVisibleChanged: {
        if (metadatamodel.filter === Shotcut.MetadataModel.FavoritesFilter)
            favButton.checked = true;
        else if (metadatamodel.filter === Shotcut.MetadataModel.VideoFilter)
            vidButton.checked = true;
        else if (metadatamodel.filter === Shotcut.MetadataModel.AudioFilter)
            audButton.checked = true;
        else if (metadatamodel.filter === Shotcut.MetadataModel.LinkFilter)
            lnkButton.checked = true;
    }
    color: activePalette.window

    SystemPalette {
        id: activePalette
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8

        RowLayout {
            id: searchBar

            property var savedFilter

            Layout.fillWidth: true

            TextField {
                id: searchField

                Layout.fillWidth: true
                focus: true
                placeholderText: qsTr("search")
                selectByMouse: true
                text: metadatamodel.search
                onTextChanged: {
                    if (length !== 1 && text !== metadatamodel.search)
                        metadatamodel.search = text;
                    if (length > 0) {
                        parent.savedFilter = typeGroup.checkedButton;
                        favButton.checked = vidButton.checked = audButton.checked = false;
                    } else {
                        parent.savedFilter.checked = true;
                    }
                }
                Keys.onReturnPressed: event => {
                    menuListView.itemSelected(menuListView.currentIndex);
                    event.accepted = true;
                }
                Keys.onEnterPressed: event => Keys.onReturnPressed(event)
                Keys.onEscapePressed: {
                    if (text !== '')
                        text = '';
                    else
                        filterWindow.close();
                }
                Keys.onUpPressed: menuListView.selectPrevious()
                Keys.onDownPressed: menuListView.selectNext()
            }

            ToolButton {
                id: clearButton

                padding: 2
                implicitWidth: 20
                implicitHeight: 20
                icon.name: 'edit-clear'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-clear.png'
                hoverEnabled: true
                onClicked: searchField.text = ''

                Shotcut.HoverTip {
                    text: qsTr('Clear search')
                }
            }

            Label {
                width: 10
            }

            Shotcut.Button {
                id: closeButton

                icon.name: 'window-close'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/window-close.png'
                padding: 2
                implicitWidth: 20
                implicitHeight: 20
                onClicked: filterWindow.close()

                Shotcut.HoverTip {
                    text: qsTr('Close menu')
                }
            }

            Label {
                width: 40
            }
        }

        RowLayout {
            id: toolBar

            Layout.fillWidth: true

            ButtonGroup {
                id: typeGroup
            }

            Shotcut.ToggleButton {
                id: favButton

                checked: true
                implicitWidth: settings.playerGPU ? 20 : 80
                icon.name: 'bookmarks'
                icon.source: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                text: qsTr('Favorite')
                display: settings.playerGPU ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
                ButtonGroup.group: typeGroup
                onClicked: {
                    if (checked) {
                        metadatamodel.filter = Shotcut.MetadataModel.FavoritesFilter;
                        searchField.text = '';
                        checked = true;
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Show favorite filters')
                }
            }

            Shotcut.ToggleButton {
                id: gpuButton

                visible: settings.playerGPU
                checked: true
                implicitWidth: 60
                icon.name: 'cpu'
                icon.source: 'qrc:///icons/oxygen/32x32/devices/cpu.png'
                text: 'GPU'
                ButtonGroup.group: typeGroup
                onClicked: {
                    if (checked) {
                        metadatamodel.filter = Shotcut.MetadataModel.GPUFilter;
                        searchField.text = '';
                        checked = true;
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Show GPU video filters')
                }
            }

            Shotcut.ToggleButton {
                id: vidButton

                implicitWidth: 80
                icon.name: 'video-television'
                icon.source: 'qrc:///icons/oxygen/32x32/devices/video-television.png'
                text: qsTr('Video')
                ButtonGroup.group: typeGroup
                onClicked: {
                    if (checked) {
                        metadatamodel.filter = Shotcut.MetadataModel.VideoFilter;
                        searchField.text = '';
                        checked = true;
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Show video filters')
                }
            }

            Shotcut.ToggleButton {
                id: audButton

                implicitWidth: 80
                icon.name: 'speaker'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/speaker.png'
                text: qsTr('Audio')
                ButtonGroup.group: typeGroup
                onClicked: {
                    if (checked) {
                        metadatamodel.filter = Shotcut.MetadataModel.AudioFilter;
                        searchField.text = '';
                        checked = true;
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Show audio filters')
                }
            }

            Shotcut.ToggleButton {
                id: lnkButton

                implicitWidth: 80
                visible: attachedfiltersmodel.supportsLinks
                icon.name: 'chronometer'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/chronometer.png'
                text: qsTr('Time')
                ButtonGroup.group: typeGroup
                onClicked: {
                    if (checked) {
                        metadatamodel.filter = Shotcut.MetadataModel.LinkFilter;
                        searchField.text = '';
                        checked = true;
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Show time filters')
                }
            }

            Shotcut.ToggleButton {
                id: setButton

                implicitWidth: 80
                icon.name: 'server-database'
                icon.source: 'qrc:///icons/oxygen/32x32/places/server-database.png'
                text: qsTr('Sets')
                ButtonGroup.group: typeGroup
                onClicked: {
                    if (checked) {
                        metadatamodel.filter = Shotcut.MetadataModel.FilterSetFilter;
                        searchField.text = '';
                        checked = true;
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Show filter sets')
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: filterWindow.height - toolBar.height - searchBar.height - iconKeywordsRow.height - parent.spacing * (parent.children.length - 1)
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.horizontal.height: 0
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ScrollBar.vertical.visible: contentHeight > height
            ScrollBar.vertical.width: 16
            ScrollBar.vertical.background: Rectangle {
                color: Qt.lighter(parent.palette.alternateBase)
            }

            ListView {
                id: menuListView

                function itemSelected(index) {
                    if (index > -1) {
                        filterWindow.close();
                        filterSelected(index);
                    }
                }

                function selectNext() {
                    do {
                        currentIndex = Math.min(currentIndex + 1, count - 1);
                    } while (currentItem !== null && !currentItem.visible && currentIndex < count - 1)
                }

                function selectPrevious() {
                    do {
                        menuListView.currentIndex = Math.max(menuListView.currentIndex - 1, 0);
                    } while (!menuListView.currentItem.visible && menuListView.currentIndex > 0)
                }

                anchors.fill: parent
                model: metadatamodel
                boundsBehavior: Flickable.StopAtBounds
                maximumFlickVelocity: 600
                currentIndex: -1
                focus: true
                onCountChanged: {
                    currentIndex = -1;
                }

                delegate: FilterMenuDelegate {
                }
            }
        }

        RowLayout {
            id: iconKeywordsRow

            Layout.preferredHeight: 60

            AnimatedImage {
                id: icon

                property var current: metadatamodel.get(menuListView.currentIndex)

                source: current ? (current.icon.toString().length ? current.icon : current.isAudio ? 'qrc:///icons/oxygen/32x32/actions/speaker.png' : current.type === Shotcut.Metadata.Link ? 'qrc:///icons/oxygen/32x32/actions/chronometer.png' : '') : ''
                asynchronous: true
                Layout.preferredWidth: parent.Layout.preferredHeight * sourceSize.width / sourceSize.height
                Layout.preferredHeight: parent.Layout.preferredHeight
                fillMode: Image.PreserveAspectFit
                onPlayingChanged: {
                    if (!playing)
                        playing = true;
                }
            }

            Label {
                id: keywordsLabel

                text: icon.current ? (icon.current.type === Shotcut.Metadata.FilterSet && icon.current.mlt_service.length === 0) ? qsTr('Delete a custom filter set by right-clicking it.') : icon.current.keywords : ''
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                Layout.preferredWidth: filterWindow.width - parent.Layout.preferredHeight - 20
            }
        }
    }
}
