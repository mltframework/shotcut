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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * Example: query clips, filter the snapshots in JavaScript, and navigate.
 *
 * The host supplies "shotcut". timeline.clips() returns clip information
 * across tracks, while setSelection() and seek() are non-undoable UI actions.
 * No undo macro is needed because this example does not edit project content.
 *
 * Store a clip ID rather than its row or start frame for a later user action.
 * Undo, deletion, and timeline edits can invalidate cached locations. Resolve
 * the ID again at click time and handle a missing clip explicitly.
 */
Item {
    id: root

    property var matches: []
    property string nameFilter: ""
    property int minimumSeconds: 0
    property string message: ""

    function refresh() {
        const query = nameFilter.trim().toLowerCase();
        // API durations and positions are integer project frames. Comparing
        // against this threshold avoids rounding the duration to whole seconds.
        const minimumFrames = minimumSeconds * shotcut.project.profile.fps;
        const filtered = [];
        for (const clip of shotcut.timeline.clips()) {
            if (clip.name.toLowerCase().includes(query) && clip.duration >= minimumFrames)
                filtered.push(clip);
        }
        // Replace the array to notify QML and rebuild the ListView model.
        // Mutating a returned object would not change Shotcut's timeline.
        matches = filtered;
    }

    function reportError(error) {
        message = error.message;
        console.warn("Clip Navigator: " + error.code + ": " + error.message);
    }

    function selectMatches() {
        try {
            // Re-query before acting instead of trusting the last displayed
            // snapshot. setSelection replaces the timeline's selection.
            refresh();
            const ids = [];
            for (const clip of matches)
                ids.push(clip.id);
            shotcut.timeline.setSelection(ids);
            message = qsTr("Selected %1 clips.").arg(ids.length);
        } catch (error) {
            reportError(error);
        }
    }

    function clearSelection() {
        try {
            shotcut.timeline.setSelection([]);
            message = qsTr("Selection cleared.");
        } catch (error) {
            reportError(error);
        }
    }

    function goToClip(id) {
        try {
            // findClip returns the current location or a missing result. A
            // cached start frame could now point at a different clip.
            const clip = shotcut.timeline.findClip(id);
            if (!clip) {
                refresh();
                message = qsTr("This clip is no longer in the timeline.");
                console.warn("Clip Navigator: " + message);
                return;
            }
            shotcut.timeline.seek(clip.start);
            shotcut.timeline.setSelection([clip.id]);
            message = qsTr("Selected %1 at frame %2.").arg(clip.name).arg(clip.start);
        } catch (error) {
            reportError(error);
        }
    }

    onNameFilterChanged: refresh()
    onMinimumSecondsChanged: refresh()
    Component.onCompleted: refresh()

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: qsTr("Clip Navigator")
            font.bold: true
        }
        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("Filter clip names")
            Accessible.name: qsTr("Clip name filter")
            onTextChanged: root.nameFilter = text
        }
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Minimum seconds:")
            }
            SpinBox {
                from: 0
                to: 86400
                editable: true
                Accessible.name: qsTr("Minimum clip duration in seconds")
                onValueChanged: root.minimumSeconds = value
            }
        }
        Label {
            text: qsTr("%1 matching clips across all tracks").arg(root.matches.length)
        }
        RowLayout {
            Layout.fillWidth: true
            Button {
                text: qsTr("Select Matches")
                enabled: root.matches.length > 0
                onClicked: root.selectMatches()
            }
            Button {
                text: qsTr("Clear Selection")
                enabled: shotcut.timeline.exists
                onClicked: root.clearSelection()
            }
        }
        ListView {
            id: clipList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.matches
            clip: true
            spacing: 4
            ScrollBar.vertical: ScrollBar {}

            delegate: Button {
                property var clip: modelData

                width: clipList.width
                text: qsTr("%1 - track %2, frame %3, %4 s").arg(clip.name).arg(clip.trackIndex).arg(clip.start).arg((clip.duration / shotcut.project.profile.fps).toFixed(2))
                Accessible.name: qsTr("Select and go to %1").arg(clip.name)
                ToolTip.text: text
                ToolTip.visible: hovered
                onClicked: root.goToClip(clip.id)
                contentItem: Label {
                    text: parent.text
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
        Label {
            visible: root.message.length > 0
            text: root.message
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Label {
            text: qsTr("Click a clip to select it and jump to its start. No project edits or undo entries.")
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
    }

    Connections {
        // Rebuild method-result snapshots after content changes, including
        // undo/redo. Selection alone does not affect this query.
        function onChanged() {
            root.refresh();
        }
        target: shotcut.timeline
    }
    Connections {
        // The seconds-to-frames conversion also depends on the project profile.
        function onChanged() {
            root.refresh();
        }
        target: shotcut.project
    }
}
