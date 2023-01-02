/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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
import QtQuick.Dialogs
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    id: stabilizeRoot

    property url settingsSavePath: 'file:///' + settings.savePath
    property string _analysisRequiredMessage: qsTr('Click Analyze to use this filter.')

    // This signal is used to workaround context properties not available in
    // the FileDialog onAccepted signal handler on Qt 5.5.
    signal fileSaved(string filename)

    function hasAnalysisCompleted() {
        return (filter.get("results").length > 0 && filter.get("filename").indexOf(filter.get("results")) !== -1);
    }

    function setStatus(inProgress) {
        if (inProgress) {
            status.text = qsTr('Analyzing...');
            results.text = "--";
        } else if (hasAnalysisCompleted()) {
            status.text = qsTr('Analysis complete.');
            results.text = filter.get("results");
        } else {
            status.text = _analysisRequiredMessage;
            results.text = "--";
        }
    }

    function analyzeValueChanged() {
        status.text = _analysisRequiredMessage;
    }

    function startAnalyzeJob(filename) {
        stabilizeRoot.fileSaved(filename);
        filter.set('filename', filename);
        filter.getHash();
        setStatus(true);
        filter.analyze();
    }

    width: 350
    height: 150
    Component.onCompleted: {
        filter.set('analyze', 0);
        shakinessSlider.value = filter.getDouble('shakiness');
        accuracySlider.value = filter.getDouble('accuracy');
        setStatus(false);
    }
    onFileSaved: {
        var lastPathSeparator = filename.lastIndexOf('/');
        if (lastPathSeparator !== -1)
            settings.savePath = filename.substring(0, lastPathSeparator);
    }

    Connections {
        function onAnalyzeFinished() {
            filter.set("reload", 1);
            setStatus(false);
        }

        function onInChanged() {
            analyzeValueChanged();
        }

        function onOutChanged() {
            analyzeValueChanged();
        }

        target: filter
    }

    FileDialog {
        // In Windows, the prefix is a little different
        id: fileDialog

        title: qsTr('Select a file to store analysis results.')
        modality: application.dialogModality
        selectExisting: false
        selectMultiple: false
        selectFolder: false
        folder: settingsSavePath
        nameFilters: ["Stabilize Results (*.stab)"]
        selectedNameFilter: "Stabilize Results (*.stab)"
        onAccepted: {
            var filename = fileDialog.fileUrl.toString();
            // Remove resource prefix ("file://")
            filename = filename.substring(7);
            if (filename.substring(2, 4) == ':/')
                filename = filename.substring(1);

            // Force file extension to ".stab"
            var extension = ".stab";
            var extIndex = filename.indexOf(extension, filename.length - extension.length);
            if (extIndex == -1)
                filename += extension;
            startAnalyzeJob(filename);
        }
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('<b>Analyze Options</b>')
            Layout.columnSpan: 3
        }

        Label {
            text: qsTr('Shakiness')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: shakinessSlider

            minimumValue: 1
            maximumValue: 10
            stepSize: 1
            onValueChanged: {
                filter.set('shakiness', value);
                analyzeValueChanged();
            }
        }

        Shotcut.UndoButton {
            onClicked: shakinessSlider.value = 4
        }

        Label {
            text: qsTr('Accuracy')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: accuracySlider

            minimumValue: 1
            maximumValue: 15
            stepSize: 1
            onValueChanged: {
                filter.set('accuracy', value);
                analyzeValueChanged();
            }
        }

        Shotcut.UndoButton {
            onClicked: accuracySlider.value = 4
        }

        Shotcut.Button {
            id: button

            text: qsTr('Analyze')
            Layout.alignment: Qt.AlignRight
            onClicked: {
                var filename = application.getNextProjectFile('stab');
                if (filename) {
                    stabilizeRoot.fileSaved(filename);
                    startAnalyzeJob(filename);
                } else {
                    fileDialog.open();
                }
            }
        }

        Label {
            id: status

            Layout.columnSpan: 2
        }

        Label {
            text: qsTr('<b>Filter Options</b>')
            Layout.columnSpan: 3
        }

        Label {
            text: qsTr('Zoom')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: zoomSlider

            minimumValue: -50
            maximumValue: 50
            decimals: 1
            suffix: ' %'
            value: filter.getDouble('zoom')
            onValueChanged: {
                filter.set('zoom', value);
                filter.set("refresh", 1);
            }
        }

        Shotcut.UndoButton {
            onClicked: zoomSlider.value = 0
        }

        Label {
            text: qsTr('Smoothing')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: smoothingSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 1
            value: filter.get('smoothing')
            onValueChanged: {
                filter.set('smoothing', value);
            }
        }

        Shotcut.UndoButton {
            onClicked: smoothingSlider.value = 15
        }

        Rectangle {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.minimumHeight: 12
            color: 'transparent'

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 2
                radius: 2
                color: activePalette.text
            }
        }

        Label {
            text: qsTr('Stabilization file:')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The stabilization file generated by the analysis.')
            }
        }

        Label {
            id: results

            Layout.columnSpan: 2
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
