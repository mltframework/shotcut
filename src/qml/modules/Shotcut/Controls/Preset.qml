/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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

RowLayout {
    property var parameters: []

    // Tell the parent QML page to update its controls.
    signal beforePresetLoaded()
    signal presetSelected()

    Component.onCompleted: {
        filter.loadPresets();
    }

    Shotcut.ComboBox {
        id: presetCombo

        Layout.fillWidth: true
        Layout.minimumWidth: 100
        Layout.maximumWidth: 300
        model: filter ? filter.presets : 0
        onActivated: {
            if (currentText.length > 0) {
                // toggling focus works around a weird bug involving sticky
                // input event focus on the ComboBox
                enabled = false;
                filter.blockSignals = true;
                filter.animateIn = 0;
                filter.animateOut = 0;
                beforePresetLoaded();
                filter.preset(currentText);
                presetSelected();
                filter.blockSignals = false;
                filter.changed();
                filter.animateInChanged();
                filter.animateOutChanged();
                enabled = true;
            }
        }
    }

    Shotcut.Button {
        id: saveButton

        icon.name: 'list-add'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/list-add.png'
        implicitWidth: 20
        implicitHeight: 20
        onClicked: nameDialog.show()

        Shotcut.HoverTip {
            text: qsTr('Save')
        }

    }

    Shotcut.Button {
        id: deleteButton

        icon.name: 'list-remove'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
        implicitWidth: 20
        implicitHeight: 20
        onClicked: confirmDialog.show()

        Shotcut.HoverTip {
            text: qsTr('Delete')
        }

    }

    SystemPalette {
        id: dialogPalette

        colorGroup: SystemPalette.Active
    }

    Window {
        id: nameDialog

        function acceptName() {
            var params = parameters;
            params.push('shotcut:animIn');
            params.push('shotcut:animOut');
            presetCombo.currentIndex = filter.savePreset(params, nameField.text);
            nameDialog.close();
        }

        flags: Qt.Dialog
        color: dialogPalette.window
        modality: application.dialogModality
        title: qsTr('Save Preset')
        width: 200
        height: 100
        Component.onCompleted: nameField.forceActiveFocus(Qt.TabFocusReason)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            Label {
                text: qsTr('Name:')
            }

            TextField {
                id: nameField

                Layout.fillWidth: true
                selectByMouse: true
                onAccepted: nameDialog.acceptName()
                Keys.onPressed: (event)=> {
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

    Window {
        id: confirmDialog

        flags: Qt.Dialog
        color: dialogPalette.window
        modality: application.dialogModality
        title: qsTr('Delete Preset')
        width: 300
        height: 90
        Component.onCompleted: confirmDialogOk.forceActiveFocus(Qt.TabFocusReason)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            Label {
                text: qsTr('Are you sure you want to delete %1?').arg(presetCombo.currentText)
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                Shotcut.Button {
                    id: confirmDialogOk

                    text: qsTr('OK')
                    focus: true
                    onClicked: {
                        if (presetCombo.currentText !== ' ') {
                            filter.deletePreset(presetCombo.currentText);
                            presetCombo.currentIndex = 0;
                        }
                        confirmDialog.close();
                    }
                }

                Shotcut.Button {
                    text: qsTr('Cancel')
                    onClicked: confirmDialog.close()
                }

            }

        }

    }

}
