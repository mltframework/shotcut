/*
 * Copyright (c) 2014-2019 Meltytech, LLC
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
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.0
import QtQuick.Window 2.1
import Shotcut.Controls 1.0 as Shotcut
import org.shotcut.qml 1.0 as Shotcut
import QtQml.Models 2.2

Item {
    id: webvfxRoot
    width: 350
    height: 200 + customUILoader.height
    property url settingsSavePath: 'file:///' + settings.savePath

    SystemPalette { id: activePalette; colorGroup: SystemPalette.Active }
    Shotcut.File { id: htmlFile }
    Shotcut.File {
        id: customUiFile
        url: {
            if (!htmlFile.url || !htmlFile.exists())
                return "";
            var uiFile = htmlFile.url;
            return uiFile.substr(0, uiFile.lastIndexOf(".")) + "_ui.qml";
        }
        onUrlChanged: {
            if (exists())
                customUILoader.source = 'file:///' + url;
            else
                customUILoader.source = "";
        }
    }

    // This signal is used to workaround context properties not available in
    // the FileDialog onAccepted signal handler on Qt 5.5.
    signal fileSaved(string path)
    onFileSaved: settings.savePath = path

    Component.onCompleted: {
        var resource = filter.get('resource')
        if (resource.substring(0,6) == "plain:") {
            resource = resource.substring(6)
            webvfxCheckBox.checked = false
        } else if (resource) {
            webvfxCheckBox.checked = true
        }
        else {
            webvfxCheckBox.checked = false
        }

        htmlFile.url = resource

        if (htmlFile.exists()) {
            fileLabel.text = htmlFile.fileName
            fileLabelTip.text = htmlFile.filePath
            fileLabel.visible = true
            openButton.visible = false
            templatesView.visible = false
            editButton.visible = true
            reloadButton.visible = true
            webvfxCheckBox.enabled = false
        } else {
            fileLabel.visible = false
            filter.set("disable", 1)
        }
    }

    function handleHtmlFile(selectExisting, selectFolder) {
        var protocol = webvfxCheckBox.checked? 'webvfx' : 'plain'
        if (!selectExisting || selectFolder) {
            templatesView.selection.forEach( function(row) {
                protocol = templatesModel.data(row, Shotcut.WebvfxTemplatesModel.ProtocolRole)
                htmlFile.url = templatesModel.copyTemplate(row, htmlFile.url)
                if (!htmlFile.url.length)
                    return
            })
        }

        webvfxRoot.fileSaved(htmlFile.path)
        fileLabel.text = htmlFile.fileName
        fileLabelTip.text = htmlFile.filePath
        fileLabel.visible = true
        openButton.visible = false
        templatesView.visible = false
        webvfxCheckBox.enabled = false
        editButton.visible = true
        reloadButton.visible = true

        var resource = htmlFile.url
        if (protocol === 'webvfx') {
            filter.set('duration', filter.duration / profile.fps)
        } else {
            resource = "plain:" + resource
        }
        filter.set('resource', resource)
        filter.set("disable", 0)

        if (!selectExisting || selectFolder) {
            editor.edit(htmlFile.url)
            reloadButton.enabled = false
        }
    }

    FileDialog {
        id: fileDialog
        modality: Qt.WindowModal
        selectMultiple: false
        selectFolder: false
        folder: settingsSavePath
        nameFilters: [ "HTML-Files (*.htm *.html)", "All Files (*)" ]
        selectedNameFilter: "HTML-Files (*.htm *.html)"
        onAccepted: {
            htmlFile.url = fileDialog.fileUrl
            if (!selectExisting && !selectFolder && !htmlFile.suffix())
                htmlFile.url = htmlFile.url + ".html"
            handleHtmlFile(selectExisting, selectFolder)
        }
        onRejected: {
            openButton.visible = true
            templatesView.visible = true
        }
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        TableView {
            id: templatesView
            Layout.columnSpan: 4
            Layout.fillWidth: true
            Shotcut.WebvfxTemplatesModel {
                id: templatesModel
            }
            model: templatesModel
            TableViewColumn {
                title: qsTr('Templates')
                role: 'name'
                resizable: false
            }
            property int selectedRow: -1
            onClicked : {
                selectedRow = row
                if (application.isProjectFolder()) {
                    htmlFile.url = templatesModel.copyTemplate(row)
                    handleHtmlFile(false, true)
                    editor.edit(htmlFile.url)
                    reloadButton.enabled = false
                } else if (templatesModel.needsFolder(row)) {
                    fileDialog.selectExisting = true
                    fileDialog.selectFolder = true
                    fileDialog.title = qsTr("Choose a Folder for HTML")
                    fileDialog.open()
                } else {
                    fileDialog.selectExisting = false
                    fileDialog.selectFolder = false
                    fileDialog.title = qsTr("Save HTML File")
                    fileDialog.open()
                }
            }
        }

        Label {
            id: fileRowLabel
            text: qsTr('<b>File:</b>')
            visible: fileLabel.visible
        }
        Label {
            id: fileLabel
            Layout.columnSpan: 3
            Shotcut.ToolTip { id: fileLabelTip }
        }

        Button {
            id: openButton
            text: qsTr('Open...')
            onClicked: {
                fileDialog.selectExisting = true
                fileDialog.selectFolder = false
                fileDialog.title = qsTr( "Open HTML File" )
                fileDialog.open()
            }
            Shotcut.ToolTip {
                 text: qsTr("Load an existing HTML file.")
            }
        }
        CheckBox {
            id: webvfxCheckBox
            Layout.columnSpan: 3
            Layout.fillWidth: true
            text: qsTr('Use WebVfx JavaScript extension')
            visible: openButton.visible
            Shotcut.ToolTip {
                id: webvfxCheckTip
                text: '<b>' + qsTr('For Advanced Users: ') + '</b>' + '<p>' +
                      qsTr('If you enable this, and you do not use the WebVfx JavaScript extension, your content will not render!')
            }
            onClicked: {
                if (checked) {
                    webvfxDialog.visible = true
                }
            }
            MessageDialog {
                id: webvfxDialog
                visible: false
                modality: Qt.ApplicationModal
                icon: StandardIcon.Question
                title: qsTr("Confirm Selection")
                text: webvfxCheckTip.text + "<p>" + qsTr("Do you still want to use this?")
                standardButtons: StandardButton.Yes | StandardButton.No
                onNo: {
                    webvfxCheckBox.checked = false
                }
            }
        }

        Button {
            id: editButton
            text: qsTr('Edit...')
            visible: false
            onClicked: {
                editor.edit(htmlFile.url)
                reloadButton.enabled = false
            }
            Shotcut.HtmlEditor {
                id: editor
                onSaved: {
                    filter.set("_reload", 1);
                }
                onClosed: {
                    reloadButton.enabled = true
                }
            }
        }
        Button {
            id: reloadButton
            text: qsTr('Reload')
            visible: false
            onClicked: {
                filter.set("_reload", 1);
            }
        }
        Item {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            visible: editButton.visible
        }

        Loader {
            id: customUILoader
            Layout.columnSpan: 4
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true;
            Layout.columnSpan: 4
        }
    }

    Connections {
        target: filter
        onInChanged: filter.set('duration', filter.duration / profile.fps)
        onOutChanged: filter.set('duration', filter.duration / profile.fps)
    }
}
