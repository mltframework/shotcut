/*
 * Copyright (c) 2013-2026 Meltytech, LLC
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
import Shotcut.Controls as Shotcut

Rectangle {
    id: trackHeadRoot

    property string trackName: ''
    property real trackGain: 0
    property bool isMute
    property bool isHidden
    property bool isComposite
    property bool isLocked
    property bool isVideo
    property bool isFiltered
    property bool isTopVideo
    property bool isBottomVideo
    property bool isTopAudio
    property bool isBottomAudio
    property bool selected: false
    property bool current: false
    property real trackAudioLevel: -100
    property bool trackAudioLevelSupported: false
    property bool _blockTrackGainUpdate: true
    property real _maximumGainDb: 10
    property real _minimumGainDb: -70

    signal clicked

    function pulseLockButton() {
        lockButtonAnim.restart();
    }

    function _iecScale(dB) {
        if (dB < -70.0)
            return 0.0;
        if (dB < -60.0)
            return (dB + 70.0) * 0.0025;
        if (dB < -50.0)
            return (dB + 60.0) * 0.005 + 0.025;
        if (dB < -40.0)
            return (dB + 50.0) * 0.0075 + 0.075;
        if (dB < -30.0)
            return (dB + 40.0) * 0.015 + 0.15;
        if (dB < -20.0)
            return (dB + 30.0) * 0.02 + 0.3;
        if (dB < -0.001 || dB > 0.001)
            return (dB + 20.0) * 0.025 + 0.5;
        return 1.0;
    }

    function _iecScaleMax(dB, maxDb) {
        return _iecScale(dB) / _iecScale(maxDb);
    }

    function _mixColor(a, b, t) {
        return Qt.rgba(a.r + (b.r - a.r) * t,
                       a.g + (b.g - a.g) * t,
                       a.b + (b.b - a.b) * t,
                       a.a + (b.a - a.a) * t);
    }

    function _audioLevelColor(level) {
        if (level <= -70.0)
            return Qt.rgba(0.0, 0.0, 0.0, 1.0);

        const darkGreen = Qt.color('darkgreen');
        const green = Qt.color('green');
        const yellow = Qt.color('yellow');
        const red = Qt.color('red');
        const darkRed = Qt.color('darkred');
        const brightness = Math.max(0.0, Math.min(1.0, _iecScaleMax(level, 0.0)));
        const shapedBrightness = Math.pow(brightness, 1.35);
        const levelPos = Math.max(0.0, Math.min(1.0, _iecScaleMax(level, 0.0)));
        const pDarkGreen = _iecScaleMax(-90.0, 0.0);
        const pGreen = _iecScaleMax(-12.0, 0.0);
        const pYellow = _iecScaleMax(-6.0, 0.0);
        const pRed = _iecScaleMax(0.0, 0.0);
        let color = darkGreen;

        if (level > 0.0) {
            const over = Math.max(0.0, Math.min(1.0, level / _maximumGainDb));
            color = _mixColor(red, darkRed, over);
        } else if (levelPos <= pGreen) {
            color = _mixColor(darkGreen, green, (levelPos - pDarkGreen) / (pGreen - pDarkGreen));
        } else if (levelPos <= pYellow) {
            color = _mixColor(green, yellow, (levelPos - pGreen) / (pYellow - pGreen));
        } else {
            color = _mixColor(yellow, red, (levelPos - pYellow) / (pRed - pYellow));
        }

        return Qt.rgba(color.r * shapedBrightness,
                       color.g * shapedBrightness,
                       color.b * shapedBrightness,
                       1.0);
    }

    function _indicatorAudioLevel() {
        return isMute ? -100.0 : trackAudioLevel;
    }

    function _indicatorAudioLevelText() {
        if (isMute)
            return qsTr('Track muted');
        if (trackAudioLevel <= -70.0)
            return qsTr('-inf dB');
        return trackAudioLevel.toFixed(1) + qsTr(' dB');
    }

    function _setTrackGain(value) {
        if (_blockTrackGainUpdate)
            return;
        const gain = Math.round(Math.max(_minimumGainDb, Math.min(_maximumGainDb, value)) * 10) / 10;
        if (!timeline.setTrackGain(index, gain)) {
            _syncTrackGain();
            return;
        }
        if (gain <= volumeSlider.from && !isMute) {
            timeline.toggleTrackMute(index);
        } else if (gain > volumeSlider.from && isMute) {
            timeline.toggleTrackMute(index);
        }
    }

    function _syncTrackGain() {
        _blockTrackGainUpdate = true;
        volumeSlider.value = Math.max(volumeSlider.from,
                                      Math.min(volumeSlider.to,
                                               Math.round(trackGain * 10) / 10));
        _blockTrackGainUpdate = false;
    }

    function _toggleMuteWithPopupState(modifiers) {
        if (modifiers & Qt.AltModifier) {
            timeline.toggleOtherTracksMute(index);
            return;
        }
        if (volumePopup.opened) {
            timeline.toggleTrackMute(index);
        } else {
            _syncTrackGain();
            volumePopup.open();
        }
    }

    onTrackGainChanged: _syncTrackGain()

    Component.onCompleted: _syncTrackGain()

    color: selected ? selectedTrackColor : (index % 2) ? activePalette.alternateBase : activePalette.base
    border.color: selected ? 'red' : 'transparent'
    border.width: selected ? 1 : 0
    clip: true
    state: 'normal'
    states: [
        State {
            name: 'selected'
            when: trackHeadRoot.selected

            PropertyChanges {
                target: trackHeadRoot
                color: isVideo ? root.shotcutBlue : 'darkseagreen'
            }
        },
        State {
            name: 'current'
            when: trackHeadRoot.current

            PropertyChanges {
                target: trackHeadRoot
                color: Qt.rgba(selectedTrackColor.r * selectedTrackColor.a + activePalette.window.r * (1 - selectedTrackColor.a), selectedTrackColor.g * selectedTrackColor.a + activePalette.window.g * (1 - selectedTrackColor.a), selectedTrackColor.b * selectedTrackColor.a + activePalette.window.b * (1 - selectedTrackColor.a), 1)
            }
        },
        State {
            when: !trackHeadRoot.selected && !trackHeadRoot.current
            name: 'normal'

            PropertyChanges {
                target: trackHeadRoot
                color: (index % 2) ? activePalette.alternateBase : activePalette.base
            }
        }
    ]
    transitions: [
        Transition {
            to: '*'

            ColorAnimation {
                target: trackHeadRoot
                duration: 100
            }
        }
    ]

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            parent.clicked();
            nameEdit.focus = false;
            if (mouse.button === Qt.RightButton)
                root.timelineRightClicked();
        }
    }

    Flow {
        id: trackHeadColumn

        flow: (trackHeadRoot.height < 50) ? Flow.LeftToRight : Flow.TopToBottom
        spacing: (trackHeadRoot.height < 50) ? 0 : 6

        anchors {
            top: parent.top
            left: parent.left
            leftMargin: 8
            rightMargin: (trackHeadRoot.height < 50) ? 0 : 4
            topMargin: (trackHeadRoot.height < 50) ? 0 : 4
            bottomMargin: (trackHeadRoot.height < 50) ? 0 : 4
        }

        Rectangle {
            color: 'transparent'
            width: trackHeadRoot.width - trackHeadColumn.anchors.margins * 2 - (trackHeadRoot.height < 50 ? 120 : 0)
            radius: 2
            border.color: (!timeline.isFloating() && trackNameMouseArea.containsMouse) ? activePalette.shadow : 'transparent'
            height: nameEdit.height

            MouseArea {
                id: trackNameMouseArea

                height: parent.height
                width: nameEdit.width
                hoverEnabled: true
                onClicked: {
                    if (!timeline.isFloating()) {
                        nameEdit.focus = true;
                        nameEdit.selectAll();
                    }
                }
            }

            Control {
                Shotcut.HoverTip {
                    text: trackName
                }

                contentItem: Label {
                    text: trackName
                    color: activePalette.windowText
                    elide: Qt.ElideRight
                    leftPadding: 4
                    topPadding: 3
                    width: nameEdit.width
                }
            }

            TextField {
                id: nameEdit

                visible: focus
                width: parent.width
                selectByMouse: true
                text: trackName
                onEditingFinished: {
                    timeline.setTrackName(index, text);
                    focus = false;
                }
                Keys.onTabPressed: editingFinished()
            }
        }

        RowLayout {
            spacing: 8

            Item {
                Layout.alignment: Qt.AlignVCenter
                width: 14
                height: 14
                visible: trackHeadRoot.trackAudioLevelSupported

                Rectangle {
                    anchors.centerIn: parent
                    width: 10
                    height: 10
                    radius: 5
                    antialiasing: true
                    color: trackHeadRoot._audioLevelColor(trackHeadRoot._indicatorAudioLevel())
                    border.color: activePalette.shadow
                    border.width: 1
                }

                MouseArea {
                    id: audioLevelIndicatorMouseArea

                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    hoverEnabled: true

                    ToolTip {
                        parent: audioLevelIndicatorMouseArea
                        visible: audioLevelIndicatorMouseArea.containsMouse
                        delay: 0
                        timeout: -1
                        text: trackHeadRoot._indicatorAudioLevelText()
                    }
                }
            }

            ToolButton {
                id: lockButton

                icon.name: isLocked ? 'object-locked' : 'object-unlocked'
                icon.source: isLocked ? 'qrc:///icons/oxygen/32x32/status/object-locked.png' : 'qrc:///icons/oxygen/32x32/status/object-unlocked.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus
                onClicked: timeline.setTrackLock(index, !isLocked)
                transformOrigin: Item.Center

                Shotcut.HoverTip {
                    text: (isLocked ? qsTr('Unlock track') : qsTr('Lock track')) + application.actionFirstShortcut('timelineToggleTrackLockedAction')
                }

                SequentialAnimation {
                    id: lockButtonAnim

                    loops: 2

                    NumberAnimation {
                        target: lockButton
                        property: 'scale'
                        to: 2
                        duration: 200
                    }

                    NumberAnimation {
                        target: lockButton
                        property: 'scale'
                        to: 1
                        duration: 200
                    }
                }
            }

            ToolButton {
                id: volumeButton

                icon.name: isMute ? 'audio-volume-muted' : 'player-volume'
                icon.source: isMute ? 'qrc:///icons/oxygen/32x32/status/audio-volume-muted.png' : 'qrc:///icons/oxygen/32x32/actions/player-volume.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: mouse => trackHeadRoot._toggleMuteWithPopupState(mouse.modifiers)
                }

                Shotcut.HoverTip {
                    text: volumePopup.opened ? qsTr('Mute/Unmute - Alt+Click to toggle mute of other tracks') + application.actionFirstShortcut('timelineToggleTrackMuteAction') : qsTr('Adjust track volume - Alt+Click to toggle mute of other tracks') + application.actionFirstShortcut('timelineToggleTrackMuteAction')
                }

                Popup {
                    id: volumePopup

                    parent: trackHeadRoot
                    x: Math.max(0, volumeButton.x - width + volumeButton.width)
                    y: Math.min(trackHeadRoot.height - 4, volumeButton.y + volumeButton.height)
                    width: 220
                    height: 36
                    padding: 6
                    modal: false
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                    background: Rectangle {
                        radius: 3
                        color: activePalette.base
                        border.color: activePalette.mid
                    }

                    contentItem: RowLayout {
                        spacing: 6

                        ToolButton {
                            id: popupMuteButton

                            Layout.alignment: Qt.AlignVCenter
                            icon.name: isMute ? 'audio-volume-muted' : 'audio-volume-high'
                            icon.source: isMute ? 'qrc:///icons/oxygen/32x32/status/audio-volume-muted.png' : 'qrc:///icons/oxygen/32x32/status/audio-volume-high.png'
                            icon.width: 16
                            icon.height: 16
                            padding: 1
                            focusPolicy: Qt.NoFocus
                            onClicked: timeline.toggleTrackMute(index)

                            Shotcut.HoverTip {
                                text: isMute ? qsTr('Unmute') : qsTr('Mute')
                            }
                        }

                        Slider {
                            id: volumeSlider

                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            from: _minimumGainDb
                            to: _maximumGainDb
                            stepSize: 0.1
                            snapMode: Slider.SnapAlways
                            live: true
                            value: 0
                            implicitHeight: 24
                            implicitWidth: 120
                            onValueChanged: trackHeadRoot._setTrackGain(value)

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton
                                hoverEnabled: true
                                onWheel: wheel => {
                                    const step = 0.1;
                                    const delta = wheel.angleDelta.y > 0 ? step : -step;
                                    volumeSlider.value = Math.max(volumeSlider.from,
                                                                  Math.min(volumeSlider.to,
                                                                           Math.round((volumeSlider.value + delta) * 10) / 10));
                                    wheel.accepted = true;
                                }
                            }
                        }

                        Label {
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: 52
                            horizontalAlignment: Text.AlignRight
                            text: (Math.abs(trackGain) < 0.05) ? '0 dB' : ((trackGain > 0 ? '+' : '') + (Math.round(trackGain * 10) / 10).toFixed(1) + ' dB')
                            color: activePalette.windowText
                            font.pixelSize: 10
                        }
                    }
                }
            }

            ToolButton {
                id: hideButton

                visible: isVideo
                icon.name: isHidden ? 'layer-visible-off' : 'layer-visible-on'
                icon.source: isHidden ? 'qrc:///icons/oxygen/32x32/actions/layer-visible-off.png' : 'qrc:///icons/oxygen/32x32/actions/layer-visible-on.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus

                MouseArea {
                    anchors.fill: parent
                    onClicked: mouse => {
                        if (mouse.modifiers & Qt.AltModifier) {
                            timeline.toggleOtherTracksHidden(index);
                        } else {
                            timeline.toggleTrackHidden(index);
                        }
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Show/Hide - Alt+Click to toggle visibility of other tracks') + application.actionFirstShortcut('timelineToggleTrackHiddenAction')
                }
            }

            ToolButton {
                visible: isFiltered
                icon.name: 'view-filter'
                icon.source: 'qrc:///icons/oxygen/32x32/status/view-filter.png'
                icon.width: 16
                icon.height: 16
                padding: 1
                focusPolicy: Qt.NoFocus
                onClicked: {
                    trackHeadRoot.clicked();
                    nameEdit.focus = false;
                    timeline.filteredClicked();
                }

                Shotcut.HoverTip {
                    text: qsTr('Filters')
                }
            }
        }
    }
}
