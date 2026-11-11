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
 * Read-only starting point for a Coproducer script.
 *
 * The host loads this Item into the panel and supplies the "shotcut" context
 * property. No Shotcut-specific import or MCP connection is needed. API
 * properties support normal QML bindings; method results are plain snapshots
 * and need to be queried again when their source changes.
 *
 * Use Open Scripts Folder to edit the user copy, then Reload to recreate the
 * component. Missing examples are copied there from the installation, but
 * existing user copies are not overwritten. Scripts are trusted local code,
 * not sandboxed extensions. See Help > API Documentation for the full API.
 */
Item {
    id: root

    property int clipCount: 0
    property int selectedCount: 0

    function refresh() {
        // Unlike the property bindings below, these method calls do not install
        // live bindings to the contents of the returned arrays. Cache their
        // counts and refresh them from the timeline notifications.
        clipCount = shotcut.timeline.clips().length;
        selectedCount = shotcut.timeline.selection().length;
    }

    // Establish an initial snapshot; notifications only cover later changes.
    Component.onCompleted: refresh()

    ColumnLayout {
        anchors.fill: parent
        Label {
            text: qsTr("Project Dashboard")
            font.bold: true
        }
        Label {
            text: shotcut.project.name
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Label {
            text: qsTr("Tracks: %1").arg(shotcut.timeline.trackCount)
        }
        Label {
            text: qsTr("Clips: %1").arg(root.clipCount)
        }
        Label {
            text: qsTr("Selected clips: %1").arg(root.selectedCount)
        }
        Label {
            // API property notifications keep these expressions up to date
            // without an explicit refresh call. Times are project frames.
            text: qsTr("Timeline duration: %1 frames").arg(shotcut.timeline.duration)
        }
        Label {
            text: qsTr("Playhead: %1").arg(shotcut.player.position)
        }
        Label {
            text: qsTr("Project frame rate: %1 fps").arg(shotcut.project.profile.fps)
        }
        Label {
            text: qsTr("Read-only example. Edit this QML file and click Reload to try changes.")
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        // Content and selection have separate notifications. Listening to both
        // keeps the dashboard correct after edits, undo/redo, and user clicks.
        function onChanged() {
            root.refresh();
        }
        function onSelectionChanged() {
            root.refresh();
        }
        target: shotcut.timeline
    }
}
