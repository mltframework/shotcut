/*
 * Copyright (c) 2023 Meltytech, LLC
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
import QtQuick.Window
import Shotcut.Controls as Shotcut

Window {
    id: nameDialog

    function acceptName() {
        application.copyEnabledFilters();
        let name = nameField.text.trim();
        if (name.length) {
            metadatamodel.saveFilterSet(name);
        }
        nameDialog.close();
    }

    SystemPalette {
        id: dialogPalette

        colorGroup: SystemPalette.Active
    }

    flags: Qt.Dialog
    color: dialogPalette.window
    modality: Qt.ApplicationModal
    title: qsTr('Copy Filters')
    width: 300
    height: 115
    minimumWidth: 300
    minimumHeight: 115
    Component.onCompleted: nameField.forceActiveFocus(Qt.TabFocusReason)
    onVisibilityChanged: nameField.text = ''

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        Label {
            text: qsTr("Enter a name to save a filter set, or\nleave blank to use the clipboard:")
        }

        TextField {
            id: nameField

            Layout.fillWidth: true
            placeholderText: qsTr('optional')
            selectByMouse: true
            onAccepted: nameDialog.acceptName()
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Escape) {
                    nameDialog.close();
                    event.accepted = true;
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            focus: true

            Shotcut.Button {
                text: qsTr('OK')
                onClicked: nameDialog.acceptName()
            }

            Shotcut.Button {
                text: qsTr('Cancel')
                onClicked: nameDialog.close()
            }
        }
    }
}
