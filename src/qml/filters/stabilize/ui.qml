/*
 * Copyright (c) 2013-2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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
import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    id: stabilizeRoot
    width: 350
    height: 150
    property string settingsSavePath: settings.savePath

    function setStatus( inProgress ) {
        if (inProgress) {
            status.text = qsTr('Analyzing...')
        }
        else if (filter.get("results").length > 0 && 
                 filter.get("filename").indexOf(filter.get("results")) !== -1) {
            status.text = qsTr('Analysis complete.')
        }
        else
        {
            status.text = qsTr('Click Analyze to use this filter.')
        }
    }

    // This signal is used to workaround context properties not available in
    // the FileDialog onAccepted signal handler on Qt 5.5.
    signal fileSaved(string filename)
    onFileSaved: {
        var lastPathSeparator = filename.lastIndexOf('/')
        if (lastPathSeparator !== -1) {
            settings.savePath = filename.substring(0, lastPathSeparator)
        }
    }

    Connections {
        target: filter
        onAnalyzeFinished: {
            filter.set("reload", 1);
            setStatus(false)
            button.enabled = true
        }
    }
    
    FileDialog {
        id: fileDialog
        title: qsTr( 'Select a file to store analysis results.' )
        modality: Qt.WindowModal
        selectExisting: false
        selectMultiple: false
        selectFolder: false
        folder: settingsSavePath
        nameFilters: [ "Stabilize Results (*.stab)" ]
        selectedNameFilter: "Stabilize Results (*.stab)"
        onAccepted: {
            var filename = fileDialog.fileUrl.toString()
            // Remove resource prefix ("file://")
            filename = filename.substring(7)
            if (filename.substring(2, 4) == ':/') {
                // In Windows, the prefix is a little different
                filename = filename.substring(1)
            }
            stabilizeRoot.fileSaved(filename)

            var extension = ".stab"
            // Force file extension to ".stab"
            var extIndex = filename.indexOf(extension, filename.length - extension.length)
            if (extIndex == -1) {
                filename += ".stab"
            }
            filter.set('filename', filename)
            filter.getHash()
            setStatus(true)
            filter.analyze();
        }
        onRejected: {
            button.enabled = true
        }
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        GroupBox {
            title: qsTr('Analyze Options')
            anchors.left: parent.left
            anchors.right: parent.right
            Layout.columnSpan: 3

            GridLayout {
                columns: 3
                anchors.fill: parent
                anchors.margins: 8

                Label {
                    text: qsTr('Shakiness')
                    Layout.alignment: Qt.AlignRight
                }
                SliderSpinner {
                    id: shakinessSlider
                    minimumValue: 1
                    maximumValue: 10
                    tickmarksEnabled: true
                    stepSize: 1
                    value: filter.getDouble('shakiness')
                    onValueChanged: filter.set('shakiness', value)
                }
                UndoButton {
                    onClicked: shakinessSlider.value = 4
                }

                Label {
                    text: qsTr('Accuracy')
                    Layout.alignment: Qt.AlignRight
                }
                SliderSpinner {
                    id: accuracySlider
                    minimumValue: 1
                    maximumValue: 15
                    tickmarksEnabled: true
                    stepSize: 1
                    value: filter.getDouble('accuracy')
                    onValueChanged: filter.set('accuracy', value)
                }
                UndoButton {
                    onClicked: accuracySlider.value = 4
                }

                Button {
                    id: button
                    text: qsTr('Analyze')
                    Layout.alignment: Qt.AlignRight
                    onClicked: {
                        button.enabled = false
                        fileDialog.folder = settings.savePath
                        fileDialog.open()
                    }
                }
                Label {
                    id: status
                    Layout.columnSpan: 2
                    Component.onCompleted: {
                        setStatus(false)
                    }
                }
            }
        }

        Label {
            text: qsTr('<b>Filter Options</b>')
            Layout.columnSpan: 3
        }

        Label {
            text: qsTr('Zoom')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: zoomSlider
            minimumValue: -50
            maximumValue: 50
            decimals: 1
            suffix: ' %'
            value: filter.getDouble('zoom')
            onValueChanged: {
                filter.set('zoom', value)
                filter.set("refresh", 1);
            }
        }
        UndoButton {
            onClicked: zoomSlider.value = 0
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}
