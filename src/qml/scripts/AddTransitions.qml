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
 * Example: use a high-level batch edit instead of manipulating clip rows.
 *
 * The Scripts panel supplies "shotcut". Pass clip IDs and explicit options to
 * timeline.addTransitions(); the API pairs clips in timeline order on each
 * track, validates available footage, and performs undoable timeline edits.
 * All successful transitions from this call belong to one undo transaction.
 *
 * A skipped pair is a reported outcome, not an exception: unsuitable pairs
 * can be skipped while other pairs succeed. Invalid requests or edit failures
 * throw errors. Show both skipped reasons and errors instead of assuming that
 * every selected pair produced a transition.
 */
Item {
    id: root

    property int selectedCount: 0
    property string summary: ""
    property bool applying: false

    function refreshSelection() {
        selectedCount = shotcut.timeline.selection().length;
    }

    function applyTransitions() {
        applying = true;
        try {
            const ids = [];
            // Re-read selection at invocation time. Clip IDs survive changes
            // to row indexes caused by the transitions created during the call.
            for (const clip of shotcut.timeline.selection())
                ids.push(clip.id);
            // Duration is in project frames. Overlap consumes existing footage;
            // preserve uses source handles to keep the combined clip duration.
            // Explicit options make the script independent of GUI ripple settings.
            const result = shotcut.timeline.addTransitions(ids, {
                duration: durationFrames.value,
                mode: timingMode.currentIndex === 0 ? "overlap" : "preserve",
                closeGaps: closeGaps.checked,
                ripple: ripple.checked,
                rippleAllTracks: ripple.checked && rippleAll.checked
            });
            // This single API edit already groups its internal commands. Use
            // undo.macro only if combining it with other edits under one label.
            let text = qsTr("Created %1 transitions. Skipped %2 pairs.").arg(result.created.length).arg(result.skipped.length);
            for (const pair of result.skipped)
                text += "\n" + pair.reason;
            summary = text;
        } catch (error) {
            // API exceptions provide a machine-readable code and user-facing
            // message. Keep the message visible and the code in the console.
            summary = error.message;
            console.warn("Add Transitions: " + error.code + ": " + error.message);
        } finally {
            // Restore UI state on both success and failure; edits can alter
            // selection, so the displayed count needs another query.
            applying = false;
            refreshSelection();
        }
    }

    Component.onCompleted: refreshSelection()

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 8
            Label {
                text: qsTr("Add Transitions")
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Select clips on the timeline. Consecutive selected clips on each track are paired; intervening clips or transitions are skipped.")
                wrapMode: Text.WordWrap
            }
            Label {
                text: qsTr("Selected clips: %1").arg(root.selectedCount)
            }
            RowLayout {
                Label {
                    text: qsTr("Duration (frames):")
                }
                SpinBox {
                    id: durationFrames
                    from: 1
                    to: 100000
                    // Approximately one second, rounded to a valid frame count.
                    value: Math.max(1, Math.round(shotcut.project.profile.fps))
                    editable: true
                    Accessible.name: qsTr("Transition duration in frames")
                }
            }
            Label {
                text: qsTr("%1 seconds").arg((durationFrames.value / shotcut.project.profile.fps).toFixed(2))
            }
            ComboBox {
                id: timingMode
                Layout.fillWidth: true
                model: [qsTr("Overlap existing footage"), qsTr("Preserve combined clip duration")]
                Accessible.name: qsTr("Transition timing")
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: timingMode.currentIndex === 0 ? qsTr("Overlap consumes footage from both clips. Ripple closes the vacated space; otherwise later clips stay in place.") : qsTr("Preserve uses spare source frames before the right clip's in point. The left clip supplies its tail. Pairs without enough source frames are skipped.")
            }
            CheckBox {
                id: closeGaps
                text: qsTr("Close gaps between selected clips")
                checked: true
            }
            CheckBox {
                id: ripple
                text: qsTr("Ripple")
            }
            CheckBox {
                id: rippleAll
                text: qsTr("Ripple all unlocked tracks")
                enabled: ripple.checked
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Gap pairs are skipped when Close Gaps is off. Ripple shifts later clips on the affected track; Ripple All Tracks also removes that time from other unlocked tracks. Markers stay in place.")
            }
            Button {
                text: qsTr("Add Transitions")
                enabled: root.selectedCount >= 2 && !root.applying
                onClicked: root.applyTransitions()
            }
            Label {
                Layout.fillWidth: true
                visible: root.summary.length > 0
                text: root.summary
                wrapMode: Text.WordWrap
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("All successful transitions from this run undo together.")
                wrapMode: Text.WordWrap
            }
        }
    }

    Connections {
        function onSelectionChanged() {
            root.refreshSelection();
        }
        target: shotcut.timeline
    }
}
