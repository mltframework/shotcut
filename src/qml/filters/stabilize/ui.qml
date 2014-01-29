/*
 * Copyright (c) 2013 Meltytech, LLC
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

Rectangle {
    width: 400
    height: 200
    color: 'transparent'
    
    function setStatus( inProgress ) {
        if (inProgress) {
            status.text = qsTr('Analyzing...')
        }
        else if (filter.get("results").length > 0 && 
                 filter.get("results") == filter.get("filename") ) {
            status.text = qsTr('Analysis complete.')
        }
        else
        {
            status.text = qsTr('Click "Analyze" to use this filter.')
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
        modality: Qt.ApplicationModal2
        selectExisting: false
        selectMultiple: false
        selectFolder: false
        nameFilters: [ "Stabilize Results (*.stab)" ]
        selectedNameFilter: "Stabilize Results (*.stab)"
        onAccepted: {
            var filename = fileDialog.fileUrl.toString().substring(7)
            var extension = ".stab"
            // Force file extension to ".stab"
            if ( filename.indexOf( extension, filename - extension.length ) == -1 ) {
                filename += ".stab"
            }
            filter.set( 'filename', filename )
            setStatus(true)
            filter.analyze();
        }
        onRejected: {
            button.enabled = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        Label { text: qsTr('<b>Analyze Options</b>') }

        RowLayout {
            spacing: 8
            Label { text: qsTr('Shakiness') }
            Slider {
                id: shakinessSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('shakiness')
                minimumValue: 1
                maximumValue: 10
                tickmarksEnabled: true
                stepSize: 1
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        shakinessSpinner.value = value
                        filter.set('shakiness', value)
                    }
                }
            }
            SpinBox {
                id: shakinessSpinner
                Layout.minimumWidth: 60
                value: filter.get('shakiness')
                minimumValue: 1
                maximumValue: 10
                decimals: 0
                onValueChanged: shakinessSlider.value = value
            }
            UndoButton {
                onClicked: shakinessSlider.value = 4
            }
        }

        RowLayout {
            spacing: 8
            Label { text: qsTr('Accuracy') }
            Slider {
                id: accuracySlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('accuracy')
                minimumValue: 1
                maximumValue: 15
                tickmarksEnabled: true
                stepSize: 1
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        accuracySpinner.value = value
                        filter.set('accuracy', value)
                    }
                }
            }
            SpinBox {
                id: accuracySpinner
                Layout.minimumWidth: 60
                value: filter.get('accuracy')
                minimumValue: 1
                maximumValue: 15
                decimals: 0
                onValueChanged: accuracySlider.value = value
            }
            UndoButton {
                onClicked: accuracySlider.value = 4
            }
        }

        RowLayout {
            spacing: 8
            Button {
                id: button
                text: qsTr('Analyze')
                onClicked: {
                    button.enabled = false
                    fileDialog.open()
                }
            }
            Label {
                id: status
                Component.onCompleted: {
                    setStatus(false)
                }
            }
        }

        Label { text: qsTr('<b>Filter Options</b>') }

        RowLayout {
            spacing: 8
            Label { text: qsTr('Zoom') }
            Slider {
                id: zoomSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('zoom')
                minimumValue: -50
                maximumValue: 50
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        zoomSpinner.value = value
                        filter.set('zoom', value)
                        filter.set("refresh", 1);
                    }
                }
            }
            SpinBox {
                id: zoomSpinner
                Layout.minimumWidth: 60
                value: filter.get('zoom')
                minimumValue: -50
                maximumValue: 50
                decimals: 1
                suffix: ' %'
                onValueChanged: zoomSlider.value = value
            }
            UndoButton {
                onClicked: zoomSlider.value = 0
            }
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}
