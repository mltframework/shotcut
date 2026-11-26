/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

/*
 * Example: export the full timeline and monitor asynchronous jobs.
 *
 * "shotcut" is supplied by the Scripts panel. exportPresets() lists the default,
 * stock, and custom presets; pass the returned ID unchanged to startExport().
 * Display names are not identifiers. Custom presets can be created in Shotcut's
 * Export dock. Stored consumer properties are used directly, without GUI
 * normalization or automatic hardware-encoder substitution; API exports keep
 * hardware decoding off. Choose a filename compatible with the selected preset.
 *
 * Export, cancellation, and log retrieval are not undoable project edits.
 * Do not wrap them in undo.macro expecting rollback to cancel a job or restore
 * an output file. Queuing succeeds before encoding finishes: inspect each job's
 * status and log for the eventual result. A two-pass preset queues two jobs.
 */
Item {
    id: root

    property var presets: []
    property var jobs: []
    property string message: ""
    property string logText: ""

    function refreshJobs() {
        // Method results are snapshots. Replace the array on each poll so
        // delegate labels, progress bars, and cancellation buttons update.
        jobs = shotcut.jobs.list();
    }

    function report(error) {
        message = error.message;
        console.warn("Jobs and Export: " + error.message);
    }

    function refreshPresets() {
        try {
            presets = shotcut.jobs.exportPresets();
            let index = presets.findIndex(preset => preset.id === "default");
            presetCombo.currentIndex = index;
        } catch (error) {
            report(error);
        }
    }

    function startExport() {
        try {
            // overwrite must be an explicit user choice. The API also protects
            // project/source files; permission to overwrite is not a bypass.
            const result = shotcut.jobs.startExport(outputPath.text, presets[presetCombo.currentIndex].id, {
                overwrite: overwrite.checked
            });
            message = qsTr("Queued %1 export jobs. Follow progress below or in Jobs.").arg(result.jobIds.length);
            refreshJobs();
        } catch (error) {
            report(error);
        }
    }

    function cancelJob(id) {
        try {
            // Request cancellation by job ID, not its changing display index.
            // Cancellation is asynchronous and leaves any partial output intact.
            shotcut.jobs.cancel(id);
            message = qsTr("Cancellation requested. Partial output files are not deleted.");
            refreshJobs();
        } catch (error) {
            report(error);
        }
    }

    function showLog(id) {
        try {
            // Bound the amount displayed in the panel. A truncated response
            // contains the tail, where the final encoder error usually appears.
            const result = shotcut.jobs.log(id, 8000);
            logText = (result.truncated ? qsTr("(Log tail)\n") : "") + result.text;
        } catch (error) {
            report(error);
        }
    }

    Component.onCompleted: {
        refreshPresets();
        refreshJobs();
    }

    Timer {
        // Polling is sufficient for this small example. Loading the component
        // starts monitoring; unloading it stops the Timer, not the queued jobs.
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.refreshJobs()
    }

    FileDialog {
        id: outputDialog
        title: qsTr("Export Timeline")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Video files (*.mp4 *.mov *.mkv)"), qsTr("Audio files (*.wav *.flac *.mp3)"), qsTr("All files (*)")]
        onAccepted: {
            // FileDialog returns a file URL; the export API expects a native
            // absolute filename. Decode URL escapes and remove the local-file
            // prefix and Windows drive slash. This example uses local files,
            // not remote URLs or network-share URL conversion.
            let path = decodeURIComponent(String(selectedFile).replace(/^file:/, ""));
            if (path.startsWith("///"))
                path = path.substring(2);
            if (/^\/[A-Za-z]:/.test(path))
                path = path.substring(1);
            outputPath.text = path;
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 8

            Label {
                text: qsTr("Jobs and Export")
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Export the full timeline using a stored preset. Preset properties are passed directly to the consumer; Export controls do not normalize them. Exports are background jobs, not undoable edits. Choose a filename compatible with the preset.")
            }
            ComboBox {
                id: presetCombo
                Layout.fillWidth: true
                model: root.presets.map(preset => preset.category + " / " + preset.name)
                Accessible.name: qsTr("Export preset")
            }
            Button {
                text: qsTr("Refresh Presets")
                onClicked: root.refreshPresets()
            }
            TextField {
                id: outputPath
                Layout.fillWidth: true
                placeholderText: qsTr("Absolute output filename")
                Accessible.name: qsTr("Output filename")
            }
            Button {
                text: qsTr("Choose Output File")
                onClicked: outputDialog.open()
            }
            CheckBox {
                id: overwrite
                text: qsTr("Allow replacing an existing output file")
            }
            Button {
                text: qsTr("Start Export")
                enabled: shotcut.timeline.exists && outputPath.text.length > 0 && presetCombo.currentIndex >= 0 && !shotcut.undo.running
                onClicked: root.startExport()
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: root.message
                visible: text.length > 0
            }
            Label {
                text: qsTr("Jobs: %1").arg(root.jobs.length)
                font.bold: true
            }
            Repeater {
                model: root.jobs
                delegate: ColumnLayout {
                    property var jobData: modelData
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: jobData.label + "\n" + jobData.status + " (" + jobData.progress + "%)" + (jobData.error ? "\n" + jobData.error : "")
                    }
                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: jobData.progress
                    }
                    RowLayout {
                        Button {
                            text: qsTr("Cancel")
                            enabled: jobData.canCancel
                            onClicked: root.cancelJob(jobData.id)
                        }
                        Button {
                            text: qsTr("Show Log")
                            onClicked: root.showLog(jobData.id)
                        }
                    }
                }
            }
            TextArea {
                Layout.fillWidth: true
                visible: root.logText.length > 0
                text: root.logText
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
            }
        }
    }
}
