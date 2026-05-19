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
import QtQuick.Shapes
import QtMultimedia

Rectangle {
    color: "black"

    component ControlButton: Rectangle {
        id: _btn

        signal clicked()

        property bool active: false

        width: 44
        height: 44
        radius: 8
        color: _btnMouse.containsPress ? Qt.rgba(1, 1, 1, 0.3) : _btnMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.15) : active ? Qt.rgba(1, 1, 1, 0.12) : "transparent"

        MouseArea {
            id: _btnMouse

            anchors.fill: parent
            hoverEnabled: true
            onClicked: _btn.clicked()
        }
    }

    Component.onCompleted: hdrWindow.setVideoSink(videoOutput.videoSink)

    VideoOutput {
        id: videoOutput

        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        layer.enabled: true
        layer.format: ShaderEffectSource.RGBA16F
        layer.effect: ShaderEffect {
            property real gain: hdrWindow.hdrGain

            fragmentShader: "hdr_gain.frag.qsb"
        }
    }

    // Auto-hide overlay
    Item {
        id: overlay

        property bool _visible: true

        anchors.fill: parent

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
            cursorShape: overlay._visible ? Qt.ArrowCursor : Qt.BlankCursor
            onPositionChanged: {
                overlay._visible = true;
                hideTimer.restart();
            }
            onEntered: {
                overlay._visible = true;
                hideTimer.restart();
            }
        }

        Timer {
            id: hideTimer

            interval: 3000
            running: true
            onTriggered: overlay._visible = false
        }

        // Floating rounded control bar
        Rectangle {
            id: controlBar

            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: parent.height * 0.20
            }
            width: Math.max(buttonRow.implicitWidth + 24, 380)
            height: 88
            radius: 12
            visible: overlay._visible
            color: Qt.rgba(0, 0, 0, 0.72)

            // Absorb clicks so they don't pass through to the video
            MouseArea {
                anchors.fill: parent
            }

            // Transport buttons
            Row {
                id: buttonRow

                anchors {
                    top: parent.top
                    topMargin: 8
                    horizontalCenter: parent.horizontalCenter
                }
                spacing: 4

                // Rewind <<
                ControlButton {
                    onClicked: hdrWindow.triggerRewind()

                    Shape {
                        anchors.centerIn: parent
                        width: 22
                        height: 22

                        // Left chevron
                        ShapePath {
                            fillColor: "white"
                            strokeColor: "transparent"
                            startX: 11; startY: 2
                            PathLine { x: 3; y: 11 }
                            PathLine { x: 11; y: 20 }
                            PathLine { x: 14; y: 20 }
                            PathLine { x: 6; y: 11 }
                            PathLine { x: 14; y: 2 }
                        }

                        // Right chevron
                        ShapePath {
                            fillColor: "white"
                            strokeColor: "transparent"
                            startX: 18; startY: 2
                            PathLine { x: 10; y: 11 }
                            PathLine { x: 18; y: 20 }
                            PathLine { x: 21; y: 20 }
                            PathLine { x: 13; y: 11 }
                            PathLine { x: 21; y: 2 }
                        }
                    }
                }

                // Play / Pause
                ControlButton {
                    onClicked: hdrWindow.triggerPlayPause()

                    // Play triangle (visible when paused)
                    Shape {
                        anchors.centerIn: parent
                        visible: !hdrWindow.playing
                        width: 22
                        height: 22

                        ShapePath {
                            fillColor: "white"
                            strokeColor: "transparent"
                            startX: 5; startY: 2
                            PathLine { x: 19; y: 11 }
                            PathLine { x: 5; y: 20 }
                        }
                    }

                    // Pause bars (visible when playing)
                    Shape {
                        anchors.centerIn: parent
                        visible: hdrWindow.playing
                        width: 22
                        height: 22

                        ShapePath {
                            fillColor: "white"
                            strokeColor: "transparent"
                            startX: 4; startY: 3
                            PathLine { x: 8; y: 3 }
                            PathLine { x: 8; y: 19 }
                            PathLine { x: 4; y: 19 }
                        }

                        ShapePath {
                            fillColor: "white"
                            strokeColor: "transparent"
                            startX: 12; startY: 3
                            PathLine { x: 16; y: 3 }
                            PathLine { x: 16; y: 19 }
                            PathLine { x: 12; y: 19 }
                        }
                    }
                }

                // Fast Forward >>
                ControlButton {
                    onClicked: hdrWindow.triggerFastForward()

                    Shape {
                        anchors.centerIn: parent
                        width: 22
                        height: 22

                        // Left chevron
                        ShapePath {
                            fillColor: "white"
                            strokeColor: "transparent"
                            startX: 1; startY: 2
                            PathLine { x: 9; y: 11 }
                            PathLine { x: 1; y: 20 }
                            PathLine { x: 4; y: 20 }
                            PathLine { x: 12; y: 11 }
                            PathLine { x: 4; y: 2 }
                        }

                        // Right chevron
                        ShapePath {
                            fillColor: "white"
                            strokeColor: "transparent"
                            startX: 8; startY: 2
                            PathLine { x: 16; y: 11 }
                            PathLine { x: 8; y: 20 }
                            PathLine { x: 11; y: 20 }
                            PathLine { x: 19; y: 11 }
                            PathLine { x: 11; y: 2 }
                        }
                    }
                }

                // Fullscreen toggle
                ControlButton {
                    onClicked: hdrWindow.toggleFullScreen()

                    Shape {
                        id: fullscreenIcon

                        anchors.centerIn: parent
                        width: 22
                        height: 22

                        // Top-left corner — expand outward
                        ShapePath {
                            strokeColor: "white"
                            strokeWidth: 2
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap
                            joinStyle: ShapePath.RoundJoin
                            startX: hdrWindow.fullScreen ? 8 : 1; startY: hdrWindow.fullScreen ? 2 : 8
                            PathLine { x: hdrWindow.fullScreen ? 2 : 1; y: hdrWindow.fullScreen ? 2 : 1 }
                            PathLine { x: hdrWindow.fullScreen ? 2 : 8; y: hdrWindow.fullScreen ? 8 : 1 }
                        }

                        // Top-right corner
                        ShapePath {
                            strokeColor: "white"
                            strokeWidth: 2
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap
                            joinStyle: ShapePath.RoundJoin
                            startX: hdrWindow.fullScreen ? 14 : 21; startY: hdrWindow.fullScreen ? 2 : 8
                            PathLine { x: hdrWindow.fullScreen ? 20 : 21; y: hdrWindow.fullScreen ? 2 : 1 }
                            PathLine { x: hdrWindow.fullScreen ? 20 : 14; y: hdrWindow.fullScreen ? 8 : 1 }
                        }

                        // Bottom-left corner
                        ShapePath {
                            strokeColor: "white"
                            strokeWidth: 2
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap
                            joinStyle: ShapePath.RoundJoin
                            startX: hdrWindow.fullScreen ? 8 : 1; startY: hdrWindow.fullScreen ? 20 : 14
                            PathLine { x: hdrWindow.fullScreen ? 2 : 1; y: hdrWindow.fullScreen ? 20 : 21 }
                            PathLine { x: hdrWindow.fullScreen ? 2 : 8; y: hdrWindow.fullScreen ? 14 : 21 }
                        }

                        // Bottom-right corner
                        ShapePath {
                            strokeColor: "white"
                            strokeWidth: 2
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap
                            joinStyle: ShapePath.RoundJoin
                            startX: hdrWindow.fullScreen ? 14 : 21; startY: hdrWindow.fullScreen ? 20 : 14
                            PathLine { x: hdrWindow.fullScreen ? 20 : 21; y: hdrWindow.fullScreen ? 20 : 21 }
                            PathLine { x: hdrWindow.fullScreen ? 20 : 14; y: hdrWindow.fullScreen ? 14 : 21 }
                        }
                    }
                }

                // Settings gear
                ControlButton {
                    id: settingsButton

                    visible: hdrWindow.hdrTransferMode !== 0
                    active: settingsDialog.visible
                    onClicked: settingsDialog.visible = !settingsDialog.visible

                    onVisibleChanged: {
                        if (!visible)
                            settingsDialog.visible = false;
                    }

                    // ⚙ gear glyph (U+2699) — present in every desktop font
                    Text {
                        anchors.centerIn: parent
                        text: "\u2699"
                        color: "white"
                        font.pixelSize: 20
                    }
                }
            }

            // Scrub bar row
            Item {
                id: scrubRow

                anchors {
                    top: buttonRow.bottom
                    topMargin: 6
                    left: parent.left
                    right: parent.right
                    leftMargin: 12
                    rightMargin: 12
                }
                height: 20

                Text {
                    id: posLabel

                    anchors {
                        left: parent.left
                        verticalCenter: parent.verticalCenter
                    }
                    text: hdrWindow.positionText
                    color: Qt.rgba(1, 1, 1, 0.85)
                    font.pixelSize: 11
                }

                Text {
                    id: durLabel

                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                    }
                    text: hdrWindow.durationText
                    color: Qt.rgba(1, 1, 1, 0.85)
                    font.pixelSize: 11
                }

                Item {
                    id: scrubArea

                    anchors {
                        left: posLabel.right
                        right: durLabel.left
                        leftMargin: 8
                        rightMargin: 8
                        verticalCenter: parent.verticalCenter
                    }
                    height: parent.height

                    // Track background
                    Rectangle {
                        width: parent.width
                        height: 3
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 1.5
                        color: Qt.rgba(1, 1, 1, 0.25)

                        // Filled portion
                        Rectangle {
                            width: hdrWindow.videoDuration > 0 ? parent.width * hdrWindow.videoPosition / hdrWindow.videoDuration : 0
                            height: parent.height
                            radius: 1.5
                            color: "white"
                        }
                    }

                    // Scrub handle
                    Rectangle {
                        x: hdrWindow.videoDuration > 0 ? (scrubArea.width - width) * hdrWindow.videoPosition / hdrWindow.videoDuration : 0
                        anchors.verticalCenter: parent.verticalCenter
                        width: 10
                        height: 10
                        radius: 5
                        color: "white"
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true

                        function seek(mx) {
                            if (hdrWindow.videoDuration > 0) {
                                const frame = Math.round(mx / width * hdrWindow.videoDuration);
                                hdrWindow.seekToFrame(Math.max(0, Math.min(frame, hdrWindow.videoDuration - 1)));
                            }
                        }

                        onPressed: mouse => seek(mouseX)
                        onPositionChanged: mouse => {
                            if (pressed)
                                seek(mouseX);
                        }
                    }
                }
            }
        }

        // HDR Display Settings dialog — centered in the window
        Rectangle {
            id: settingsDialog

            anchors.centerIn: parent
            width: 300
            height: settingsColumn.implicitHeight + 28
            radius: 12
            color: Qt.rgba(0, 0, 0, 0.88)
            border.color: Qt.rgba(1, 1, 1, 0.12)
            border.width: 1
            visible: false
            z: 10

            // Absorb mouse events and restart the hide timer so the overlay
            // doesn't vanish while the user is interacting with the dialog.
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onPositionChanged: {
                    overlay._visible = true;
                    hideTimer.restart();
                }
            }

            Column {
                id: settingsColumn

                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    topMargin: 14
                    leftMargin: 14
                    rightMargin: 14
                }
                spacing: 10

                // ── Header ──────────────────────────────────────────────
                Item {
                    width: parent.width
                    height: 20

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("HDR Display Settings")
                        color: "white"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    ControlButton {
                        anchors {
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        width: 20
                        height: 20
                        radius: 10
                        onClicked: settingsDialog.visible = false

                        Text {
                            anchors.centerIn: parent
                            text: "\u00D7"
                            color: Qt.rgba(1, 1, 1, 0.7)
                            font.pixelSize: 14
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.08)
                }

                // ── Display brightness ──────────────────────────────────
                Column {
                    width: parent.width
                    spacing: 6

                    Text {
                        text: qsTr("Display brightness")
                        color: Qt.rgba(1, 1, 1, 0.6)
                        font.pixelSize: 11
                    }

                    Row {
                        spacing: 4

                        Repeater {
                            model: [{label: qsTr("Auto"), nits: 0}, {label: "400", nits: 400}, {label: "600", nits: 600}, {label: "1000", nits: 1000}, {label: "1600", nits: 1600}]

                            delegate: Rectangle {
                                readonly property bool _sel: hdrWindow.displayPeakNits === modelData.nits

                                width: _lbl.implicitWidth + 14
                                height: 24
                                radius: 4
                                color: _sel ? Qt.rgba(1, 1, 1, 0.28) : Qt.rgba(1, 1, 1, 0.1)
                                border.color: _sel ? Qt.rgba(1, 1, 1, 0.55) : Qt.rgba(1, 1, 1, 0.15)
                                border.width: 1

                                Text {
                                    id: _lbl

                                    anchors.centerIn: parent
                                    text: modelData.label
                                    color: "white"
                                    font.pixelSize: 11
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: hdrWindow.displayPeakNits = modelData.nits
                                }
                            }
                        }
                    }
                }

                // ── Content brightness (PQ only) ────────────────────────
                Column {
                    width: parent.width
                    spacing: 6
                    visible: hdrWindow.hdrTransferMode === 2

                    Text {
                        text: qsTr("Content brightness")
                        color: Qt.rgba(1, 1, 1, 0.6)
                        font.pixelSize: 11
                    }

                    Row {
                        spacing: 4

                        Repeater {
                            model: [{label: qsTr("Auto"), nits: 0}, {label: "400", nits: 400}, {label: "1000", nits: 1000}, {label: "4000", nits: 4000}, {label: "10000", nits: 10000}]

                            delegate: Rectangle {
                                readonly property bool _sel: hdrWindow.contentPeakNits === modelData.nits

                                width: _lbl2.implicitWidth + 14
                                height: 24
                                radius: 4
                                color: _sel ? Qt.rgba(1, 1, 1, 0.28) : Qt.rgba(1, 1, 1, 0.1)
                                border.color: _sel ? Qt.rgba(1, 1, 1, 0.55) : Qt.rgba(1, 1, 1, 0.15)
                                border.width: 1

                                Text {
                                    id: _lbl2

                                    anchors.centerIn: parent
                                    text: modelData.label
                                    color: "white"
                                    font.pixelSize: 11
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: hdrWindow.contentPeakNits = modelData.nits
                                }
                            }
                        }
                    }
                }

                // ── Tone mapping (PQ only) ──────────────────────────────
                Item {
                    width: parent.width
                    height: 24
                    visible: hdrWindow.hdrTransferMode === 2

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Tone mapping")
                        color: Qt.rgba(1, 1, 1, 0.6)
                        font.pixelSize: 11
                    }

                    Row {
                        anchors {
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        spacing: 4

                        Rectangle {
                            width: _onLbl.implicitWidth + 14
                            height: 24
                            radius: 4
                            color: hdrWindow.toneMapping ? Qt.rgba(1, 1, 1, 0.28) : Qt.rgba(1, 1, 1, 0.1)
                            border.color: hdrWindow.toneMapping ? Qt.rgba(1, 1, 1, 0.55) : Qt.rgba(1, 1, 1, 0.15)
                            border.width: 1

                            Text {
                                id: _onLbl

                                anchors.centerIn: parent
                                text: qsTr("On")
                                color: "white"
                                font.pixelSize: 11
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: hdrWindow.toneMapping = true
                            }
                        }

                        Rectangle {
                            width: _offLbl.implicitWidth + 14
                            height: 24
                            radius: 4
                            color: !hdrWindow.toneMapping ? Qt.rgba(1, 1, 1, 0.28) : Qt.rgba(1, 1, 1, 0.1)
                            border.color: !hdrWindow.toneMapping ? Qt.rgba(1, 1, 1, 0.55) : Qt.rgba(1, 1, 1, 0.15)
                            border.width: 1

                            Text {
                                id: _offLbl

                                anchors.centerIn: parent
                                text: qsTr("Off")
                                color: "white"
                                font.pixelSize: 11
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: hdrWindow.toneMapping = false
                            }
                        }
                    }
                }

                // Bottom padding
                Item {
                    width: 1
                    height: 2
                }
            }
        }
    }
}

