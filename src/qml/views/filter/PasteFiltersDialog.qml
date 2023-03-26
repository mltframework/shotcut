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

    flags: Qt.Dialog
    color: dialogPalette.window
    modality: Qt.ApplicationModal
    title: qsTr('Paste Filters')
    width: 300
    height: 300
    minimumHeight: 100
    minimumWidth: 300
    onVisibilityChanged: presetCombo.currentIndex = 0

    SystemPalette {
        id: dialogPalette

        colorGroup: SystemPalette.Active
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        Label {
            text: qsTr('Choose a saved filter set:')
        }

        RowLayout {

            Shotcut.ComboBox {
                id: presetCombo

                Layout.fillWidth: true
                Layout.minimumWidth: 100
                Layout.maximumWidth: 400
                model: producer.filterSets
            }

            Shotcut.Button {
                id: removeButton

                implicitWidth: height
                icon.name: 'list-remove'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
                enabled: presetCombo.currentIndex > 0
                opacity: enabled ? 1 : 0.5
                onClicked: producer.deleteFilterSet(presetCombo.currentText)

                Shotcut.HoverTip {
                    text: qsTr('Delete filter set')
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
                onClicked: {
                    if (presetCombo.currentIndex > 0) {
                        producer.pasteFilterSet(presetCombo.currentText);
                    }
                    application.pasteFilters();
                    nameDialog.close();
                }
            }

            Shotcut.Button {
                text: qsTr('Cancel')
                onClicked: nameDialog.close()
            }
        }
    }
}
