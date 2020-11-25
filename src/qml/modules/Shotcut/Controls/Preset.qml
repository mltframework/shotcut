/*
 * Copyright (c) 2013-2020 Meltytech, LLC
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
import QtQuick.Controls 2.12 as Controls2
import QtQuick.Layouts 1.0
import QtQuick.Window 2.1

RowLayout {
    property var parameters: []

    // Tell the parent QML page to update its controls.
    signal beforePresetLoaded()
    signal presetSelected()

    Component.onCompleted: {
        filter.loadPresets()
    }

    Controls2.ComboBox {
        id: presetCombo
        Layout.fillWidth: true
        Layout.minimumWidth: 100
        Layout.maximumWidth: 300
        model: filter.presets
        onCurrentTextChanged: {
            if (currentText.length > 0) {
                // toggling focus works around a weird bug involving sticky
                // input event focus on the ComboBox
                enabled = false
                filter.blockSignals = true
                filter.animateIn = 0
                filter.animateOut = 0
                beforePresetLoaded()
                filter.preset(currentText)
                presetSelected()
                filter.blockSignals = false
                filter.changed()
                filter.animateInChanged()
                filter.animateOutChanged()
                enabled = true
            }
        }
    }
    Button {
        id: saveButton
        iconName: 'list-add'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/list-add.png'
        tooltip: qsTr('Save')
        implicitWidth: 20
        implicitHeight: 20
        onClicked: nameDialog.show()
    }
    Button {
        id: deleteButton
        iconName: 'list-remove'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
        tooltip: qsTr('Delete')
        implicitWidth: 20
        implicitHeight: 20
        onClicked: confirmDialog.show()
    }

    SystemPalette { id: dialogPalette; colorGroup: SystemPalette.Active }
    Window {
        id: nameDialog
        flags: Qt.Dialog
        color: dialogPalette.window
        modality: application.dialogModality
        title: qsTr('Save Preset')
        width: 200
        height: 90

        function acceptName() {
            var params = parameters
            params.push('shotcut:animIn')
            params.push('shotcut:animOut')
            presetCombo.currentIndex = filter.savePreset(params, nameField.text)
            nameDialog.close()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            
            Label {
                text: qsTr('Name:')
            }
            TextField {
                id: nameField
                focus: true
                Layout.fillWidth: true
                onAccepted: nameDialog.acceptName()
                Keys.onPressed: {
                    if (event.key === Qt.Key_Escape) {
                        nameDialog.close()
                        event.accepted = true
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                Layout.alignment: Qt.AlignRight
                Button {
                    text: qsTr('OK')
                    isDefault: true
                    onClicked: nameDialog.acceptName()
                }
                Button {
                    text: qsTr('Cancel')
                    onClicked: nameDialog.close()
                }
            }
        }
    }

    Window {
        id: confirmDialog
        flags: Qt.Dialog
        color: dialogPalette.window
        modality: application.dialogModality
        title: qsTr('Delete Preset')
        width: 300
        height: 90

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            
            Label {
                text: qsTr('Are you sure you want to delete %1?').arg(presetCombo.currentText)
                wrapMode: Text.Wrap
            }
            
            RowLayout {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                Layout.alignment: Qt.AlignRight
                Button {
                    text: qsTr('OK')
                    isDefault: true
                    onClicked: {
                        if (presetCombo.currentText !== ' ') {
                            filter.deletePreset(presetCombo.currentText)
                            presetCombo.currentIndex = 0
                        }
                        confirmDialog.close()
                    }
                }
                Button {
                    text: qsTr('Cancel')
                    onClicked: confirmDialog.close()
                }
            }
        }
    }
}
