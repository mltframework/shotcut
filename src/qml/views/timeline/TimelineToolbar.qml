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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
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
            Shotcut.HoverTip { text: qsTr('Display a menu of additional actions') }
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        ToolButton {
            action: cutAction
            Shotcut.HoverTip { text: qsTr('Cut - Copy the current clip to the Source\nplayer and ripple delete it') }
        }
        ToolButton {
            action: copyAction
            Shotcut.HoverTip { text: qsTr('Copy - Copy the current clip to the Source player (C)') }
        }
        ToolButton {
            action: insertAction
            Shotcut.HoverTip { text: qsTr('Paste - Insert clip into the current track\nshifting following clips to the right (V)') }
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        ToolButton {
            action: appendAction
            Shotcut.HoverTip { text: qsTr('Append to the current track (A)') }
        }
        ToolButton {
            action: deleteAction
            Shotcut.HoverTip { text: qsTr('Ripple Delete - Remove current clip\nshifting following clips to the left (X)') }
        }
        ToolButton {
            action: liftAction
            Shotcut.HoverTip { text: qsTr('Lift - Remove current clip without\naffecting position of other clips (Z)') }
        }
        ToolButton {
            action: overwriteAction
            Shotcut.HoverTip { text: qsTr('Overwrite clip onto the current track (B)') }
        }
        ToolButton {
            action: splitAction
            Shotcut.HoverTip { text: qsTr('Split At Playhead (S)') }
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
            Shotcut.HoverTip { text: qsTr("Zoom timeline out (-)") }
        }
        ZoomSlider {
            id: scaleSlider
        }
        ToolButton {
            action: zoomInAction
            Shotcut.HoverTip { text: qsTr("Zoom timeline in (+)") }
        }
        ToolButton {
            action: zoomFitAction
            Shotcut.HoverTip { text: qsTr('Zoom timeline to fit (0)') }
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
        icon.name: 'edit-cut'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-cut.png'
        enabled: timeline.selection.length
        onTriggered: timeline.removeSelection(true)
    }

    Action {
        id: copyAction
        icon.name: 'edit-copy'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-copy.png'
        enabled: timeline.selection.length
        onTriggered: timeline.copyClip(timeline.selection[0].y, timeline.selection[0].x)
    }

    Action {
        id: insertAction
        icon.name: 'edit-paste'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-paste.png'
        onTriggered: timeline.insert(currentTrack)
    }

    Action {
        id: appendAction
        icon.name: 'list-add'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/list-add.png'
        onTriggered: timeline.append(currentTrack)
    }

    Action {
        id: deleteAction
        icon.name: 'list-remove'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
        onTriggered: timeline.removeSelection()
   }

    Action {
        id: liftAction
        icon.name: 'lift'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/lift.png'
        onTriggered: timeline.liftSelection()
    }

    Action {
        id: overwriteAction
        icon.name: 'overwrite'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/overwrite.png'
        onTriggered: timeline.overwrite(currentTrack)
    }

    Action {
        id: splitAction
        icon.name: 'slice'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/slice.png'
        onTriggered: timeline.splitClip(currentTrack)
    }

    Action {
        id: zoomOutAction
        icon.name: 'zoom-out'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-out.png'
        onTriggered: root.zoomOut()
    }

    Action {
        id: zoomInAction
        icon.name: 'zoom-in'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-in.png'
        onTriggered: root.zoomIn()
    }

    Action {
        id: zoomFitAction
        icon.name: 'zoom-fit-best'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/zoom-fit-best.png'
        onTriggered: root.zoomToFit()
    }
}
