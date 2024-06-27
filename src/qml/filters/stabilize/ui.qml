/*
 * Copyright (c) 2013-2024 Meltytech, LLC
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

Item {
    id: stabilizeRoot

    property url settingsSavePath: 'file:///' + settings.savePath
    property string _analysisRequiredMessage: qsTr('Click Analyze to use this filter.')

    function hasAnalysisCompleted() {
        return (filter.get("results").length > 0 && filter.get("filename").indexOf(filter.get("results")) !== -1);
    }

    function setStatus(inProgress) {
        if (inProgress) {
            status.text = qsTr('Analyzing...');
            results.setText("--");
        } else if (hasAnalysisCompleted()) {
            status.text = qsTr('Analysis complete.');
            results.setText(filter.get("results"));
        } else {
            status.text = _analysisRequiredMessage;
            results.setText("--");
        }
    }

    function analyzeValueChanged() {
        status.text = _analysisRequiredMessage;
    }

    function startAnalyzeJob(filename) {
        var lastPathSeparator = filename.lastIndexOf('/');
        if (lastPathSeparator !== -1)
            settings.savePath = filename.substring(0, lastPathSeparator);
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

    Shotcut.FileDialog {
        // In Windows, the prefix is a little different
        id: fileDialog

        title: qsTr('Select a file to store analysis results.')
        fileMode: Shotcut.FileDialog.SaveFile
        nameFilters: ["Stabilize Results (*.stab)"]
        onAccepted: {
            var filename = selectedFile;
            // Remove resource prefix ("file://")
            if (filename.substring(0, 8) == 'file:///')
                filename = filename.substring(application.OS === 'Windows' ? 8 : 7);
            else if (filename.substring(0, 7) == 'file://')
                filename = filename.substring(6);

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

            function setText(path) {
                text = path.substring(path.lastIndexOf('/') + 1);
                resultsTip.text = path
            }

            Layout.columnSpan: 2

            Shotcut.HoverTip {
                id: resultsTip
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
