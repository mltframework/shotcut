/*
 * Copyright (c) 2013-2014 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

import QtQuick 2.2
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

Rectangle {
    property alias ripple: rippleButton.checked
    property alias scrub: scrubButton.checked
    property alias snap: snapButton.checked

    SystemPalette { id: activePalette }

    width: 200
    height: 24
    color: activePalette.window

    ToolBar {
      RowLayout {
        ToolButton {
            action: menuAction
            implicitWidth: 28
            implicitHeight: 24
        }
        ToolButton {
            action: appendAction
            implicitWidth: 28
            implicitHeight: 24
        }
        ToolButton {
            action: deleteAction
            implicitWidth: 28
            implicitHeight: 24
        }
        ToolButton {
            action: liftAction
            implicitWidth: 28
            implicitHeight: 24
        }
        ToolButton {
            action: insertAction
            implicitWidth: 28
            implicitHeight: 24
        }
        ToolButton {
            action: overwriteAction
            implicitWidth: 28
            implicitHeight: 24
        }
        ToolButton {
            action: splitAction
            implicitWidth: 28
            implicitHeight: 24
        }
        ToolButton {
            id: snapButton
            implicitWidth: 28
            implicitHeight: 24
            checkable: true
            checked: true
            iconName: 'snap'
            iconSource: 'qrc:///icons/oxygen/16x16/actions/snap.png'
            tooltip: qsTr('Toggle snapping')
        }
        ToolButton {
            id: scrubButton
            implicitWidth: 28
            implicitHeight: 24
            checkable: true
            iconName: 'scrub_drag'
            iconSource: 'qrc:///icons/oxygen/16x16/actions/scrub_drag.png'
            tooltip: qsTr('Scrub while dragging')
        }
        ToolButton {
            id: rippleButton
            implicitWidth: 28
            implicitHeight: 24
            checkable: true
            iconName: 'target'
            iconSource: 'qrc:///icons/oxygen/16x16/actions/target.png'
            tooltip: qsTr('Ripple (insert) when source is dropped')
        }
      }
    }

    Action {
        id: menuAction
        tooltip: qsTr('Display a menu of additional actions')
        iconName: 'format-justify-fill'
        iconSource: 'qrc:///icons/oxygen/16x16/actions/format-justify-fill.png'
        onTriggered: menu.popup()
    }

    Action {
        id: appendAction
        tooltip: qsTr('Append to the current track (C)')
        iconName: 'list-add'
        iconSource: 'qrc:///icons/oxygen/16x16/actions/list-add.png'
        onTriggered: timeline.append(currentTrack)
    }

    Action {
        id: deleteAction
        tooltip: qsTr('Ripple Delete - Remove current clip\nshifting following clips to the left (X)')
        iconName: 'list-remove'
        iconSource: 'qrc:///icons/oxygen/16x16/actions/list-remove.png'
        onTriggered: timeline.remove(currentClipTrack, currentClip)
    }

    Action {
        id: liftAction
        tooltip: qsTr('Lift - Remove current clip without\naffecting position of other clips (Z)')
        iconName: 'lift'
        iconSource: 'qrc:///icons/oxygen/16x16/actions/lift.png'
        onTriggered: timeline.lift(currentClipTrack, currentClip)
    }

    Action {
        id: insertAction
        tooltip: qsTr('Insert clip into the current track\nshifting following clips to the right (V)')
        iconName: 'insert'
        iconSource: 'qrc:///icons/oxygen/16x16/actions/insert.png'
        onTriggered: timeline.insert(currentTrack)
    }

    Action {
        id: overwriteAction
        tooltip: qsTr('Overwrite clip onto the current track (B)')
        iconName: 'overwrite'
        iconSource: 'qrc:///icons/oxygen/16x16/actions/overwrite.png'
        onTriggered: timeline.overwrite(currentTrack)
    }

    Action {
        id: splitAction
        tooltip: qsTr('Split At Playhead (S)')
        iconName: 'split'
        iconSource: 'qrc:///icons/oxygen/16x16/actions/split.png'
        onTriggered: timeline.splitClip(currentTrack, currentClip)
    }
}
