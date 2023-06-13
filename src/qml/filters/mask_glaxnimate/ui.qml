/*
 * Copyright (c) 2022-2023 Meltytech, LLC
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
import QtQuick.Dialogs
import QtQuick.Layouts
import Shotcut.Controls as Shotcut
import org.shotcut.qml as Shotcut

Item {
    id: shapeRoot

    function setControls() {
        shapeFile.url = filter.get('filter.resource');
        invertCheckBox.checked = filter.getDouble('filter.invert') === 1;
        reverseCheckBox.checked = filter.getDouble('filter.invert_mask') === 1;
        var currentOp = filter.get('filter.alpha_operation');
        for (var i = 0; i < operationModel.count; ++i) {
            if (operationModel.get(i).value === currentOp) {
                operationCombo.currentIndex = i;
                break;
            }
        }
    }

    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('filter', 'shape');
            filter.set('filter.use_luminance', 0);
            filter.set('filter.use_mix', 0);
            filter.set('filter.audio_match', 0);
            filter.set('filter.invert', 0);
            filter.set('filter.invert_mask', 0);
            filter.set('filter.alpha_operation', 'overwrite');
        }
        setControls();
    }

    Shotcut.File {
        id: shapeFile

        onUrlChanged: {
            if (fileName.length > 0) {
                fileLabel.text = fileName;
                fileLabelTip.text = filePath;
            }
            watch();
        }
        onFileChanged: filter.set('filter.producer.refresh', 1)
    }

    Shotcut.FileDialog {
        id: fileDialog

        fileMode: Shotcut.FileDialog.OpenFile
        onAccepted: {
            shapeFile.url = fileDialog.selectedFile;
            if (fileDialog.fileMode === Shotcut.FileDialog.SaveFile) {
                // Force file extension to ".rawr"
                var filename = shapeFile.url;
                var extension = ".rawr";
                var extIndex = filename.indexOf(extension, filename.length - extension.length);
                if (extIndex == -1) {
                    filename += extension;
                }
                shapeFile.url = filename;
                producer.newGlaxnimateFile(filename);
            }
            filter.set('filter.resource', shapeFile.url);
            fileLabelTip.text = shapeFile.filePath;
            settings.openPath = shapeFile.path;
            if (fileDialog.fileMode === Shotcut.FileDialog.SaveFile)
                producer.launchGlaxnimate(shapeFile.url);
        }
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Layout.alignment: Qt.AlignRight

            Shotcut.Button {
                text: qsTr('New...')
                onClicked: {
                    var filename = application.getNextProjectFile('rawr');
                    if (filename) {
                        producer.newGlaxnimateFile(filename);
                        shapeFile.url = filename;
                        filter.set('filter.resource', shapeFile.url);
                        settings.openPath = shapeFile.path;
                        producer.launchGlaxnimate(shapeFile.url);
                    } else {
                        fileDialog.fileMode = Shotcut.FileDialog.SaveFile;
                        fileDialog.title = qsTr('New Animation File');
                        fileDialog.open();
                    }
                }
            }

            Shotcut.Button {
                text: qsTr('Open...')
                onClicked: {
                    fileDialog.fileMode = Shotcut.FileDialog.OpenFile;
                    fileDialog.title = qsTr('Open Animation File');
                    fileDialog.open();
                }
            }
        }

        Label {
            id: fileLabel

            Layout.columnSpan: parent.columns - 1
            text: qsTr('Click <b>New...</b> or <b>Open...</b> to use this filter')

            Shotcut.HoverTip {
                id: fileLabelTip
            }
        }

        Item {
            width: 1
        }

        RowLayout {
            Layout.columnSpan: parent.columns - 1

            Shotcut.Button {
                text: qsTr('Edit...')
                onClicked: producer.launchGlaxnimate(filter.get('filter.resource'))
            }

            Shotcut.Button {
                text: qsTr('Reload')
                onClicked: {
                    filter.set('filter.producer.refresh', 1);
                }
            }
        }

        Item {
            width: 1
        }

        RowLayout {
            Layout.columnSpan: parent.columns - 1

            CheckBox {
                id: invertCheckBox

                text: qsTr('Invert')
                onClicked: filter.set('filter.invert', checked)
            }

            Shotcut.UndoButton {
                onClicked: {
                    invertCheckBox.checked = false;
                    filter.set('filter.invert', 0);
                }
            }

            Item {
                width: 1
            }

            CheckBox {
                id: reverseCheckBox

                text: qsTr('Reverse')
                onClicked: filter.set('filter.invert_mask', checked)
            }

            Shotcut.UndoButton {
                onClicked: {
                    reverseCheckBox.checked = false;
                    filter.set('filter.invert_mask', checked);
                }
            }
        }

        Label {
            text: qsTr('Operation')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Layout.columnSpan: parent.columns - 1

            Shotcut.ComboBox {
                id: operationCombo

                implicitWidth: 180
                textRole: 'text'
                visible: filter.isAtLeastVersion(4)
                onActivated: filter.set('filter.alpha_operation', operationModel.get(currentIndex).value)

                model: ListModel {
                    id: operationModel

                    ListElement {
                        text: qsTr('Overwrite')
                        value: 'overwrite'
                    }

                    ListElement {
                        text: qsTr('Maximum')
                        value: 'maximum'
                    }

                    ListElement {
                        text: qsTr('Minimum')
                        value: 'minimum'
                    }

                    ListElement {
                        text: qsTr('Add')
                        value: 'add'
                    }

                    ListElement {
                        text: qsTr('Subtract')
                        value: 'subtract'
                    }
                }
            }

            Shotcut.UndoButton {
                onClicked: {
                    operationCombo.currentIndex = 0;
                    filter.set('filter.alpha_operation', 'overwrite');
                }
            }
        }

        Shotcut.TipBox {
            Layout.columnSpan: parent.columns
            Layout.fillWidth: true
            Layout.margins: 10
            text: qsTr('Tip: Mask other video filters by adding filters after this one followed by <b>Mask: Apply</b>')
        }

        Label {
            Layout.fillHeight: true
        }
    }
}
