/*
 * Copyright (c) 2016-2021 Meltytech, LLC
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
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.12
import QtQuick.Window 2.1
import Shotcut.Controls 1.0 as Shotcut
import org.shotcut.qml 1.0 as Shotcut

Item {
    id: lut3dRoot

    property url settingsOpenPath: 'file:///' + settings.openPath

    // This signal is used to workaround context properties not available in
    // the FileDialog onAccepted signal handler on Qt 5.5.
    signal fileOpened(string path)

    width: 350
    height: 100
    onFileOpened: settings.openPath = path
    Component.onCompleted: {
        var resource = filter.get('av.file');
        lutFile.url = resource;
        if (filter.isNew) {
            interpolationCombo.currentIndex = 1;
            filter.set('av.interp', interpolationCombo.values[1]);
        } else {
            interpolationCombo.currentIndex = interpolationCombo.valueToIndex();
        }
        if (lutFile.exists()) {
            fileLabel.text = lutFile.fileName;
            fileLabelTip.text = lutFile.filePath;
        } else {
            fileLabel.text = qsTr("No File Loaded");
            fileLabel.color = 'red';
            fileLabelTip.text = qsTr('No 3D LUT file loaded.\nClick "Open" to load a file.');
        }
    }

    SystemPalette {
        id: activePalette

        colorGroup: SystemPalette.Active
    }

    Shotcut.File {
        id: lutFile
    }

    FileDialog {
        id: fileDialog

        modality: application.dialogModality
        selectMultiple: false
        selectFolder: false
        folder: settingsOpenPath
        nameFilters: ['3D-LUT Files (*.3dl *.cube *.dat *.m3d)', 'AfterEffects (*.3dl)', 'Iridas (*.cube)', 'DaVinci (*.dat)', 'Pandora (*.m3d)', 'All Files (*)']
        onAccepted: {
            lutFile.url = fileDialog.fileUrl;
            lut3dRoot.fileOpened(lutFile.path);
            fileLabel.text = lutFile.fileName;
            fileLabel.color = activePalette.text;
            fileLabelTip.text = lutFile.filePath;
            filter.set('av.file', lutFile.url);
        }
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Shotcut.Button {
            id: openButton

            text: qsTr('Open...')
            Layout.alignment: Qt.AlignRight
            onClicked: {
                fileDialog.selectExisting = true;
                fileDialog.title = qsTr("Open 3D LUT File");
                fileDialog.open();
            }
        }

        Label {
            id: fileLabel

            Layout.columnSpan: 2
            Layout.fillWidth: true

            Shotcut.HoverTip {
                id: fileLabelTip
            }

        }

        Label {
            text: qsTr('Interpolation')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: interpolationCombo

            property var values: ['nearest', 'trilinear', 'tetrahedral']

            function valueToIndex() {
                var w = filter.get('av.interp');
                for (var i = 0; i < values.length; ++i) if (values[i] === w) {
                    break;
                }
                if (i === values.length)
                    i = 1;

                return i;
            }

            implicitWidth: 180
            model: [qsTr('Nearest'), qsTr('Trilinear'), qsTr('Tetrahedral')]
            onActivated: filter.set('av.interp', values[currentIndex])
        }

        Shotcut.UndoButton {
            onClicked: {
                interpolationCombo.currentIndex = 1;
                filter.set('av.interp', interpolationCombo.values[1]);
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.columnSpan: 3
        }

    }

}
