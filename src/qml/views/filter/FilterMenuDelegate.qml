/*
 * Copyright (c) 2014-2023 Meltytech, LLC
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
import "FilterMenu.js" as Logic
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut
import org.shotcut.qml as Shotcut

Rectangle {
    id: wrapper

    visible: isVisible
    height: visible ? Logic.ITEM_HEIGHT : 0
    color: activePalette.base

    SystemPalette {
        id: activePalette
    }

    Row {
        height: Logic.ITEM_HEIGHT

        Rectangle {
            color: activePalette.base
            anchors.top: parent.top
            width: parent.height
            height: parent.height

            ToolButton {
                id: favButton

                implicitWidth: 20
                implicitHeight: 18
                padding: 1
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                opacity: favorite ? 1 : 0.3
                icon.name: 'bookmarks'
                icon.source: 'qrc:///icons/oxygen/32x32/places/bookmarks.png'
                onClicked: favorite = !favorite
            }
        }

        Rectangle {
            id: itemBackground

            color: wrapper.ListView.isCurrentItem ? activePalette.highlight : activePalette.base
            width: wrapper.ListView.view.width - favButton.width
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            ToolButton {
                id: itemIcon

                implicitWidth: 20
                implicitHeight: 18
                padding: 1
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                enabled: false
                icon.name: needsGpu ? 'cpu' : isAudio ? 'speaker' : pluginType === Shotcut.Metadata.Link ? 'chronometer' : pluginType === Shotcut.Metadata.FilterSet ? 'server-database' : 'video-television'
                icon.source: needsGpu ? 'qrc:///icons/oxygen/32x32/devices/cpu.png' : isAudio ? 'qrc:///icons/oxygen/32x32/actions/speaker.png' : pluginType === Shotcut.Metadata.Link ? 'qrc:///icons/oxygen/32x32/actions/chronometer.png' : pluginType === Shotcut.Metadata.FilterSet ? 'qrc:///icons/oxygen/32x32/places/server-database.png' : 'qrc:///icons/oxygen/32x32/devices/video-television.png'
            }

            Label {
                id: itemText

                anchors.left: itemIcon.right
                anchors.right: itemBackground.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                text: name
                color: wrapper.ListView.isCurrentItem ? activePalette.highlightedText : activePalette.text
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: mouseArea

                anchors.fill: parent
                hoverEnabled: wrapper.height > 0
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: mouse => {
                    if (mouse.button === Qt.LeftButton)
                        wrapper.ListView.view.itemSelected(index);
                    else if (pluginType === Shotcut.Metadata.FilterSet && service.length === 0)
                        confirmDialog.show();
                }
                onEntered: {
                    wrapper.ListView.view.currentIndex = index;
                }
            }
        }
    }

    Window {
        id: confirmDialog

        flags: Qt.Dialog
        color: activePalette.window
        modality: Qt.ApplicationModal
        title: qsTr('Delete Filter Set')
        width: 400
        minimumWidth: 400
        height: 90
        minimumHeight: 90
        Component.onCompleted: confirmDialogOk.forceActiveFocus(Qt.TabFocusReason)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            Label {
                text: qsTr("Are you sure you want to delete this?\n%1")
                maximumLineCount: 1
            }
            Label {
                text: name
                elide: Text.ElideRight
                Layout.preferredWidth: confirmDialog.width - parent.anchors.margins * 2
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                Shotcut.Button {
                    id: confirmDialogOk

                    text: qsTr('OK')
                    focus: true
                    onClicked: {
                        metadatamodel.deleteFilterSet(name);
                        confirmDialog.close();
                    }
                    Keys.onEscapePressed: confirmDialog.close()
                }

                Shotcut.Button {
                    text: qsTr('Cancel')
                    onClicked: confirmDialog.close()
                    Keys.onEscapePressed: confirmDialog.close()
                }
            }
            Item {
                Layout.fillHeight: true
            }
        }
    }
}
