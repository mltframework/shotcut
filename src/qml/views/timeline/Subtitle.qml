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
import QtQuick.Controls
import QtQuick.Dialogs
import Shotcut.Controls as Shotcut

Rectangle {
    id: root

    property real timeScale: 1
    property bool dragInProgress: false
    property var calculatedX: startFrame * timeScale

    color: activePalette.highlight

    signal dragStarted
    signal dragMoved(int offset)
    signal dragEnded
    signal moveRequested(int deltaFrames)

    width: (endFrame - startFrame) * timeScale
    height: 24

    Binding {
        root.x: calculatedX
        when: !dragInProgress
    }

    Text {
        id: subtitleText
        anchors.fill: parent
        text: simpleText
        color: activePalette.text
        elide: Text.ElideRight
    }

    MouseArea {
        id: mouseArea

        property int startDragX

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        cursorShape: pressed ? Qt.SizeHorCursor : Qt.PointingHandCursor
        onPressed: mouse => {
            if (mouse.button === Qt.LeftButton && !(mouse.modifiers & Qt.ControlModifier)) {
                if (!subtitlesSelectionModel.isItemSelected(index)) {
                    subtitlesSelectionModel.selectItem(index);
                }
                startDragX = root.x;
                root.dragStarted();
                root.x = startDragX;
            }
        }
        onPositionChanged: mouse => {
            if (dragInProgress) {
                var delta = root.x - startDragX;
                root.dragMoved(delta);
            }
        }
        onReleased: mouse => {
            if (mouse.button === Qt.LeftButton) {
                if (dragInProgress && startDragX != root.x) {
                    var deltaFrames = (root.x - startDragX) / timeScale;
                    console.log(root.x, startDragX, timeScale, deltaFrames);
                    root.moveRequested(deltaFrames);
                }
                root.dragEnded();
            }
        }
        onClicked: mouse => {
            if (mouse.button === Qt.LeftButton) {
                if (mouse.modifiers & Qt.ControlModifier) {
                    subtitlesSelectionModel.selectRange(index);
                } else {
                    subtitlesSelectionModel.selectItem(index);
                }
            }
        }

        drag {
            target: pressedButtons & Qt.LeftButton ? root : undefined
            axis: Drag.XAxis
            threshold: 0
            minimumX: 0
        }
    }
}
