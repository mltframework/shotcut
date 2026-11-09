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
 * Example: apply a filter patch to many clips as one named undo step.
 *
 * "shotcut" is supplied by the Scripts panel. Read methods return snapshots;
 * edit methods use Shotcut's undoable commands and throw JavaScript errors on
 * failure. A target such as {clipId: id} identifies a clip independently of
 * its current row. Filter indexes, in contrast, belong to the queried target
 * and should be read again before a later edit.
 *
 * Keep an undo.macro callback synchronous: do not use async/await, return a
 * Promise, or schedule the group's edits in a Timer. The helper commits on
 * success and rolls the entire group back on an API failure or exception.
 * Playback, selection, and background jobs are actions, not rollbackable edits.
 */
Item {
    id: root

    property int selectedCount: 0
    property string summary: ""

    function refreshSelection() {
        selectedCount = shotcut.timeline.clipIds({
            selectedOnly: true
        }).length;
    }

    function applyBrightness() {
        // Capture the selected IDs before editing. The callback operates on
        // this set even if an edit changes the UI's current selection.
        const ids = shotcut.timeline.clipIds({
            selectedOnly: true
        });
        try {
            // The label appears in undo history with the host's Script: prefix.
            // The callback's return value becomes the result of macro().
            const count = shotcut.undo.macro(qsTr("Batch Brightness"), function () {
                for (const id of ids) {
                    const target = {
                        clipId: id
                    };
                    const filters = shotcut.filters.list(target);
                    // metadataId identifies Shotcut's filter UI, not just an
                    // MLT service. Update the first matching Brightness filter;
                    // leave any additional instances alone.
                    const existing = filters.find(entry => entry.metadataId === "brightness");
                    // Brightness uses a multiplier: 100% maps to level 1.
                    // A parameter patch changes only the named properties.
                    const parameters = {
                        level: brightness.value / 100
                    };
                    if (existing)
                        shotcut.filters.setParameters(target, existing.index, parameters);
                    else
                        shotcut.filters.add(target, "brightness", parameters);
                }
                return ids.length;
            });
            summary = qsTr("Updated %1 clips. Undo once to restore them all.").arg(count);
        } catch (error) {
            // Catch outside the macro so failure is visible to the user after
            // rollback. Do not report partial progress as a successful batch.
            summary = qsTr("No changes applied: %1").arg(error.message);
            console.warn("Batch Brightness: " + error.message);
        }
    }

    Component.onCompleted: refreshSelection()

    Connections {
        function onSelectionChanged() {
            root.refreshSelection();
        }
        target: shotcut.timeline
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: qsTr("Batch Brightness")
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Select timeline clips. This script adds or updates Brightness on each clip in one named undo group. If any edit fails, the whole group is rolled back.")
        }
        Label {
            text: qsTr("Selected clips: %1").arg(root.selectedCount)
        }
        RowLayout {
            Label {
                text: qsTr("Brightness (%):")
            }
            SpinBox {
                id: brightness
                from: 0
                to: 200
                value: 100
                editable: true
                Accessible.name: qsTr("Brightness percent")
            }
        }
        Button {
            text: qsTr("Apply to Selected Clips")
            // Other scripts can use running to avoid starting overlapping work.
            enabled: root.selectedCount > 0 && !shotcut.undo.running
            onClicked: root.applyBrightness()
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: root.summary
            visible: text.length > 0
        }
        Item {
            Layout.fillHeight: true
        }
    }
}
