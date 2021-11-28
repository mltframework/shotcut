/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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
    height: settings.smallIcons ? 25 : 33

    RowLayout {
        y: 2

        Shotcut.ToolButton {
            id: menuButton
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: menuAction
            Shotcut.HoverTip { text: qsTr('Display a menu of additional actions') }
            focusPolicy: Qt.NoFocus
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: cutAction
            Shotcut.HoverTip { text: qsTr('Cut - Copy the current clip to the Source\nplayer and ripple delete it') }
            focusPolicy: Qt.NoFocus
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: copyAction
            Shotcut.HoverTip { text: qsTr('Copy - Copy the current clip to the Source player (C)') }
            focusPolicy: Qt.NoFocus
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: insertAction
            Shotcut.HoverTip { text: qsTr('Paste - Insert clip into the current track\nshifting following clips to the right (V)') }
            focusPolicy: Qt.NoFocus
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: appendAction
            Shotcut.HoverTip { text: qsTr('Append to the current track (A)') }
            focusPolicy: Qt.NoFocus
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: deleteAction
            Shotcut.HoverTip { text: qsTr('Ripple Delete - Remove current clip\nshifting following clips to the left (X)') }
            focusPolicy: Qt.NoFocus
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: liftAction
            Shotcut.HoverTip { text: qsTr('Lift - Remove current clip without\naffecting position of other clips (Z)') }
            focusPolicy: Qt.NoFocus
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: overwriteAction
            Shotcut.HoverTip { text: qsTr('Overwrite clip onto the current track (B)') }
            focusPolicy: Qt.NoFocus
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: splitAction
            Shotcut.HoverTip { text: qsTr('Split At Playhead (S)') }
            focusPolicy: Qt.NoFocus
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: markerAction
            Shotcut.HoverTip { text: qsTr('Marker (M)') }
            focusPolicy: Qt.NoFocus
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: prevMarkerAction
            Shotcut.HoverTip { text: qsTr('Previous Marker (<)') }
            focusPolicy: Qt.NoFocus
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: nextMarkerAction
            Shotcut.HoverTip { text: qsTr('Next Marker (>)') }
            focusPolicy: Qt.NoFocus
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        Shotcut.ToolButton {
            id: snapButton
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            checked: settings.timelineSnap
            icon.name: 'snap'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/snap.png'
            focusPolicy: Qt.NoFocus
            Shotcut.HoverTip { text: qsTr('Toggle snapping') }
            onClicked: settings.timelineSnap = !settings.timelineSnap
        }
        Shotcut.ToolButton {
            id: scrubButton
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            checked: settings.timelineDragScrub
            icon.name: 'scrub_drag'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/scrub_drag.png'
            focusPolicy: Qt.NoFocus
            Shotcut.HoverTip { text: qsTr('Scrub while dragging') }
            onClicked: settings.timelineDragScrub = !settings.timelineDragScrub
        }
        Shotcut.ToolButton {
            id: rippleButton
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            checked: settings.timelineRipple
            icon.name: 'target'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/target.png'
            focusPolicy: Qt.NoFocus
            Shotcut.HoverTip { text: qsTr('Ripple trim and drop') }
            onClicked: settings.timelineRipple = !settings.timelineRipple
        }
        Shotcut.ToolButton {
            id: rippleAllButton
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            checked: settings.timelineRippleAllTracks
            icon.name: 'ripple-all'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/ripple-all.png'
            focusPolicy: Qt.NoFocus
            Shotcut.HoverTip { text: qsTr('Ripple edits across all tracks') }
            onClicked: settings.timelineRippleAllTracks = !settings.timelineRippleAllTracks
        }
        Shotcut.ToolButton {
            id: rippleMarkers
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            checked: settings.timelineRippleMarkers
            icon.name: 'ripple-marker'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/ripple-marker.png'
            focusPolicy: Qt.NoFocus
            Shotcut.HoverTip { text: qsTr('Ripple markers with edits') }
            onClicked: settings.timelineRippleMarkers = !settings.timelineRippleMarkers
        }
        Button { // separator
            enabled: false
            implicitWidth: 2
            implicitHeight: toolbar.height / 2
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: zoomOutAction
            Shotcut.HoverTip { text: qsTr("Zoom timeline out (-)") }
            focusPolicy: Qt.NoFocus
        }
        ZoomSlider {
            id: scaleSlider
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: zoomInAction
            Shotcut.HoverTip { text: qsTr("Zoom timeline in (+)") }
            focusPolicy: Qt.NoFocus
        }
        Shotcut.ToolButton {
            implicitHeight: toolbar.height - 3
            implicitWidth: implicitHeight
            action: zoomFitAction
            Shotcut.HoverTip { text: qsTr('Zoom timeline to fit (0)') }
            focusPolicy: Qt.NoFocus
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
        onTriggered: timeline.copy(timeline.selection[0].y, timeline.selection[0].x)
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
        id: markerAction
        icon.name: 'marker'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/marker.png'
        onTriggered: timeline.createOrEditMarker()
    }

    Action {
        id: prevMarkerAction
        icon.name: 'format-indent-less'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-indent-less.png'
        onTriggered: timeline.seekPrevMarker()
    }

    Action {
        id: nextMarkerAction
        icon.name: 'format-indent-more'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/format-indent-more.png'
        onTriggered: timeline.seekNextMarker()
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
