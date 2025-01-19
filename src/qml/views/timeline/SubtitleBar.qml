/*
 * Copyright (c) 2024-2025 Meltytech, LLC
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

Item {
    id: subtitlebar

    property real timeScale: 1

    height: 24

    function startDrag() {
        var selectedItems = subtitlesSelectionModel.selectedItems;
        for (var i = 0; i < selectedItems.length; i++) {
            var subtitle = subtitlesRepeater.itemAt(selectedItems[i]);
            subtitle.dragInProgress = true;
            subtitle.x = subtitle.calculatedX;
            subtitle.z += subtitlesModel.itemCount(subtitlesSelectionModel.selectedTrack());
        }
    }

    function setDragDelta(delta) {
        var selectedItems = subtitlesSelectionModel.selectedItems;
        for (var i = 0; i < selectedItems.length; i++) {
            var subtitle = subtitlesRepeater.itemAt(selectedItems[i]);
            subtitle.x = subtitle.calculatedX + delta;
        }
    }

    function endDrag() {
        var selectedItems = subtitlesSelectionModel.selectedItems;
        for (var i = 0; i < selectedItems.length; i++) {
            var subtitle = subtitlesRepeater.itemAt(selectedItems[i]);
            subtitle.dragInProgress = false;
            subtitle.z -= subtitlesModel.itemCount(subtitlesSelectionModel.selectedTrack());
        }
    }

    function moveItems(deltaFrames) {
        var selectedRows = subtitlesSelectionModel.selectedRows();
        if (selectedRows.length <= 0) {
            return;
        }
        var groupStart = -1;
        for (var i = 0; i < selectedRows.length; i++) {
            var subtitle = subtitlesRepeater.itemAt(selectedRows[i].row);
            if (groupStart == -1 || subtitle.calculatedX < groupStart) {
                groupStart = subtitle.calculatedX;
            }
        }
        groupStart = groupStart / timeScale;
        var newStart = (groupStart + deltaFrames) * 1000.0 / profile.fps;
        if (subtitlesModel.validateMove(selectedRows, newStart)) {
            var trackIndex = subtitlesSelectionModel.selectedTrack();
            var firstItemIndex = selectedRows[0].row;
            var lastItemIndex = selectedRows[selectedRows.length - 1].row;
            subtitlesModel.moveItems(trackIndex, firstItemIndex, lastItemIndex, newStart);
        } else {
            application.showStatusMessage(qsTr('Unable to move. Subtitles already exist at this time.'));
        }
    }

    Connections {
        function onSelectedTrackModelIndexChanged() {
            subtitlesDelegateModel.rootIndex = subtitlesSelectionModel.selectedTrackModelIndex;
        }

        function onSelectedItemsChanged() {
            subtitlesSelectionRepeater.model = subtitlesSelectionModel.selectedItems;
        }

        target: subtitlesSelectionModel
    }

    DelegateModel {
        id: subtitlesDelegateModel

        model: subtitlesModel

        onRootIndexChanged: {
            if (rootIndex != subtitlesSelectionModel.selectedTrackModelIndex) {
                rootIndex = subtitlesSelectionModel.selectedTrackModelIndex;
            }
        }

        delegate: Subtitle {
            timeScale: subtitlebar.timeScale

            onDragStarted: subtitlebar.startDrag()
            onDragMoved: delta => subtitlebar.setDragDelta(delta)
            onDragEnded: subtitlebar.endDrag()
            onMoveRequested: deltaFrames => subtitlebar.moveItems(deltaFrames)
        }
    }

    Repeater {
        id: subtitlesRepeater
        model: subtitlesDelegateModel
    }

    Repeater {
        id: subtitlesSelectionRepeater

        Rectangle {
            property var subtitle: subtitlesRepeater.itemAt(modelData)

            x: subtitle ? subtitle.x : 0
            y: subtitle ? subtitle.y : 0
            z: subtitle ? subtitle.z + 1 : 0
            width: subtitle ? subtitle.width : 0
            height: subtitle ? subtitle.height : 0
            color: 'transparent'
            border.color: 'red'
        }
    }
}
