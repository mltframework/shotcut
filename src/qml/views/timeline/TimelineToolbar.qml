/*
 * Copyright (c) 2013-2020 Meltytech, LLC
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
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.2
import Shotcut.Controls 1.0 as Shotcut

ToolBar {
    property alias scrub: scrubButton.checked
    property color checkedColor: Qt.rgba(activePalette.highlight.r, activePalette.highlight.g, activePalette.highlight.b, 0.4)
    property alias scaleSlider: scaleSlider

    SystemPalette { id: activePalette }

    id: toolbar
    width: 200
    height: 32

    RowLayout {
        y: 2
        ToolButton {
            id: hiddenButton
            visible: false
        }
        ToolButton {
            action: menuAction
            Shotcut.HoverTip{ text: qsTr('Display a menu of additional actions') }
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        ToolButton {
            action: cutAction
        }
        ToolButton {
            action: copyAction
        }
        ToolButton {
            action: insertAction
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        ToolButton {
            action: appendAction
        }
        ToolButton {
            action: deleteAction
        }
        ToolButton {
            action: liftAction
        }
        ToolButton {
            action: overwriteAction
        }
        ToolButton {
            action: splitAction
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        Shotcut.ToolBarToggle {
            id: snapButton
            checked: settings.timelineSnap
            iconName: 'snap'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/snap.png'
            tooltip: qsTr('Toggle snapping')
            onClicked: settings.timelineSnap = !settings.timelineSnap
        }
        Shotcut.ToolBarToggle {
            id: scrubButton
            checked: settings.timelineDragScrub
            iconName: 'scrub_drag'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/scrub_drag.png'
            tooltip: qsTr('Scrub while dragging')
            onClicked: settings.timelineDragScrub = !settings.timelineDragScrub
        }
        Shotcut.ToolBarToggle {
            id: rippleButton
            checked: settings.timelineRipple
            iconName: 'target'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/target.png'
            tooltip: qsTr('Ripple trim and drop')
            onClicked: settings.timelineRipple = !settings.timelineRipple
        }
        Shotcut.ToolBarToggle {
            id: rippleAllButton
            checked: settings.timelineRippleAllTracks
            iconName: 'ripple-all'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/ripple-all.png'
            tooltip: qsTr('Ripple edits across all tracks')
            onClicked: settings.timelineRippleAllTracks = !settings.timelineRippleAllTracks
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        ToolButton {
            action: zoomOutAction
        }
        ZoomSlider {
            id: scaleSlider
        }
        ToolButton {
            action: zoomInAction
        }
        ToolButton {
            action: zoomFitAction
        }
    }

    Action {
        id: menuAction
        icon.name: 'show-menu'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/show-menu.png'
        onTriggered: menu.popup()
    }

    Action {
        id: cutAction
//        tooltip: qsTr('Cut - Copy the current clip to the Source\nplayer and ripple delete it')
        icon.name: 'edit-cut'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-cut.png'
        enabled: timeline.selection.length
        onTriggered: timeline.removeSelection(true)
    }

    Action {
        id: copyAction
//        tooltip: qsTr('Copy - Copy the current clip to the Source player (C)')
        icon.name: 'edit-copy'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-copy.png'
        enabled: timeline.selection.length
        onTriggered: timeline.copyClip(timeline.selection[0].y, timeline.selection[0].x)
    }

    Action {
        id: insertAction
//        tooltip: qsTr('Paste - Insert clip into the current track\nshifting following clips to the right (V)')
        icon.name: 'edit-paste'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-paste.png'
        onTriggered: timeline.insert(currentTrack)
    }

    Action {
        id: appendAction
//        tooltip: qsTr('Append to the current track (A)')
        icon.name: 'list-add'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/list-add.png'
        onTriggered: timeline.append(currentTrack)
    }

    Action {
        id: deleteAction
//        tooltip: qsTr('Ripple Delete - Remove current clip\nshifting following clips to the left (X)')
        icon.name: 'list-remove'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
        onTriggered: timeline.removeSelection()
   }

    Action {
        id: liftAction
//        tooltip: qsTr('Lift - Remove current clip without\naffecting position of other clips (Z)')
        icon.name: 'lift'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/lift.png'
        onTriggered: timeline.liftSelection()
    }

    Action {
        id: overwriteAction
//        tooltip: qsTr('Overwrite clip onto the current track (B)')
        icon.name: 'overwrite'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/overwrite.png'
        onTriggered: timeline.overwrite(currentTrack)
    }

    Action {
        id: splitAction
//        tooltip: qsTr('Split At Playhead (S)')
        icon.name: 'slice'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/slice.png'
        onTriggered: timeline.splitClip(currentTrack)
    }

    Action {
        id: zoomOutAction
//        tooltip: qsTr("Zoom timeline out (-)")
        icon.name: 'zoom-out'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-out.png'
        onTriggered: root.zoomOut()
    }

    Action {
        id: zoomInAction
//        tooltip: qsTr("Zoom timeline in (+)")
        icon.name: 'zoom-in'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-in.png'
        onTriggered: root.zoomIn()
    }

    Action {
        id: zoomFitAction
//        tooltip: qsTr('Zoom timeline to fit (0)')
        icon.name: 'zoom-fit-best'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-fit-best.png'
        onTriggered: root.zoomToFit()
    }
}
