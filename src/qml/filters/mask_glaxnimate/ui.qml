/*
 * Copyright (c) 2022 Meltytech, LLC
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
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Shotcut.Controls 1.0 as Shotcut
import org.shotcut.qml 1.0 as Shotcut

Item {
    id: shapeRoot
    property url settingsOpenPath: 'file:///' + settings.openPath

    width: 350
    height: 100

    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('filter', 'shape')
            filter.set('filter.use_luminance', 0)
            filter.set('filter.use_mix', 0)
            filter.set('filter.audio_match', 0)
        } else {
            if (filter.get('filter.use_mix').length === 0)
                filter.set('filter.use_mix', 1)
            if (filter.get('filter.audio_match').length === 0)
                filter.set('filter.audio_match', 1)
        }
        setControls()
    }

    function setControls() {
        shapeFile.url = filter.get('filter.resource')
    }

    // This signal is used to workaround context properties not available in
    // the FileDialog onAccepted signal handler on Qt 5.5.
    signal fileOpened(string path)
    onFileOpened: {
        settings.openPath = path
        fileDialog.folder = 'file:///' + path
    }

    Shotcut.File {
        id: shapeFile
        onUrlChanged: {
            if (fileName.length > 0) {
                fileLabel.text = fileName
                fileLabelTip.text = filePath
            }
            watch()
        }
        onFileChanged: filter.set('filter.producer.refresh', 1)
    }
    FileDialog {
        id: fileDialog
        modality: application.dialogModality
        selectMultiple: false
        selectFolder: false
        folder: settingsOpenPath
        onAccepted: {
            shapeFile.url = fileDialog.fileUrl
            if (!fileDialog.selectExisting) {
                // Force file extension to ".rawr"
                var filename = shapeFile.url
                var extension = ".rawr"
                var extIndex = filename.indexOf(extension, filename.length - extension.length)
                if (extIndex == -1) {
                    filename += extension
                    url = filename
                }
                producer.newGlaxnimateFile(filename)
            }
            filter.set('filter.resource', shapeFile.url)
            fileLabelTip.text = shapeFile.filePath
            shapeRoot.fileOpened(shapeFile.path)
            if (!fileDialog.selectExisting) {
                producer.launchGlaxnimate(shapeFile.url)
            }
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
                    var filename = application.getNextProjectFile('rawr')
                    if (filename) {
                        producer.newGlaxnimateFile(filename)
                        shapeFile.url = filename
                        filter.set('filter.resource', shapeFile.url)
                        shapeRoot.fileOpened(shapeFile.path)
                        producer.launchGlaxnimate(shapeFile.url)
                    } else {
                        fileDialog.selectExisting = false
                        fileDialog.title = qsTr('New Animation File')
                        fileDialog.open()
                    }
                }
            }
            Shotcut.Button {
                text: qsTr('Open...')
                onClicked: {
                    fileDialog.selectExisting = true
                    fileDialog.title = qsTr('Open Animation File')
                    fileDialog.open()
                }
            }
        }
        Label {
            id: fileLabel
            Layout.columnSpan: parent.columns - 1
            text: qsTr('Click <b>New...</b> or <b>Open...</b> to use this filter')
            Shotcut.HoverTip { id: fileLabelTip }
        }

        Item { width: 1 }
        RowLayout {
            Shotcut.Button {
                text: qsTr('Edit...')
                onClicked: producer.launchGlaxnimate(filter.get('filter.resource'))
            }
            Shotcut.Button {
                text: qsTr('Reload')
                onClicked: {
                    filter.set('filter.producer.refresh', 1)
                }
            }
        }
        Item { width: 1 }

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
