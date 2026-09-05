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
    property bool inlineAudioControlsEnabled: false
    property bool stackedHeaderLayout: false
    property int _pendingVolumeClickModifiers: Qt.NoModifier
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

    function _gainScalePosition(dB) {
        return Math.max(0.0,
                        Math.min(1.0,
                                 (dB - _minimumGainDb) / (_maximumGainDb - _minimumGainDb)));
    }

    function _formatDb(value) {
        const v = Math.round(value * 10) / 10;
        return (Math.abs(v) < 0.05) ? '0.0 dB' : ((v > 0 ? '+' : '') + v.toFixed(1) + ' dB');
    }

    function _inlineMeterScalePosition(dB) {
        return Math.max(0.0,
                        Math.min(1.0,
                                 (dB - _minimumGainDb) / (0.0 - _minimumGainDb)));
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
        if (gain <= _minimumGainDb && !isMute) {
            timeline.toggleTrackMute(index);
        } else if (gain > _minimumGainDb && isMute) {
            timeline.toggleTrackMute(index);
        }
    }

    function _syncTrackGain() {
        const gain = Math.max(_minimumGainDb,
                              Math.min(_maximumGainDb,
                                       Math.round(trackGain * 10) / 10));
        _blockTrackGainUpdate = true;
        volumeSlider.value = gain;
        inlineVolumeSlider.value = gain;
        _blockTrackGainUpdate = false;
    }

    function _resetTrackGain() {
        _setTrackGain(0);
    }

    function _setTrackGainFromMouse(slider, mouseX) {
        const width = Math.max(1, slider.width);
        const position = Math.max(0.0, Math.min(1.0, mouseX / width));
        _setTrackGain(slider.from + (slider.to - slider.from) * position);
    }

    function _handleVolumeButtonSingleClick(modifiers) {
        if (modifiers & Qt.AltModifier) {
            if (volumePopup.opened)
                volumePopup.close();
            timeline.toggleOtherTracksMute(index);
            return;
        }

        if (inlineAudioControlsEnabled) {
            if (volumePopup.opened)
                volumePopup.close();
            timeline.toggleTrackMute(index);
            return;
        }

        _syncTrackGain();
        if (!volumePopup.opened)
            volumePopup.open();
    }

    function _handleVolumeButtonDoubleClick() {
        if (volumePopup.opened)
            volumePopup.close();
        timeline.toggleTrackMute(index);
    }

    function _positionVolumePopupOverVolumeButton() {
        if (!volumePopup.opened)
            return;

        const buttonCenter = volumeButton.mapToItem(trackHeadRoot,
                                                    volumeButton.width / 2,
                                                    volumeButton.height / 2);
        const popupButtonCenter = popupMuteButton.mapToItem(trackHeadRoot,
                                                            popupMuteButton.width / 2,
                                                            popupMuteButton.height / 2);
        volumePopup.x += Math.round(buttonCenter.x - popupButtonCenter.x);
        volumePopup.y += Math.round(buttonCenter.y - popupButtonCenter.y);
    }

    onTrackGainChanged: _syncTrackGain()

    Timer {
        id: volumeButtonSingleClickTimer

        interval: Qt.styleHints.mouseDoubleClickInterval
        repeat: false
        onTriggered: trackHeadRoot._handleVolumeButtonSingleClick(trackHeadRoot._pendingVolumeClickModifiers)
    }

    Timer {
        id: volumeSliderWheelTipTimer

        interval: 800
        repeat: false
    }

    Timer {
        id: inlineSliderWheelTipTimer

        interval: 800
        repeat: false
    }

    Component.onCompleted: _syncTrackGain()

    color: selected ? selectedTrackColor : (index % 2) ? activePalette.alternateBase : activePalette.base
    border.color: selected ? application.playheadColor : 'transparent'
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

    Column {
        id: trackHeadColumn

        spacing: trackHeadRoot.stackedHeaderLayout ? 1 : 2

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            leftMargin: 8
            rightMargin: trackHeadRoot.stackedHeaderLayout ? 4 : 0
            topMargin: trackHeadRoot.stackedHeaderLayout ? 2 : 0
            bottomMargin: trackHeadRoot.stackedHeaderLayout ? 4 : 0
        }

        Flow {
            id: trackHeadHeaderFlow

            width: parent.width
            flow: trackHeadRoot.stackedHeaderLayout ? Flow.TopToBottom : Flow.LeftToRight
            spacing: trackHeadRoot.stackedHeaderLayout ? 4 : 0

            Rectangle {
            color: 'transparent'
            width: trackHeadHeaderFlow.width - (trackHeadRoot.stackedHeaderLayout ? 0 : trackHeadButtons.implicitWidth)
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
            id: trackHeadButtons
            spacing: 8

            Item {
                Layout.alignment: Qt.AlignVCenter
                width: 14
                height: 14
                visible: trackHeadRoot.trackAudioLevelSupported && !trackHeadRoot.inlineAudioControlsEnabled

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
                    onClicked: mouse => {
                        trackHeadRoot._pendingVolumeClickModifiers = mouse.modifiers;
                        volumeButtonSingleClickTimer.restart();
                    }
                    onDoubleClicked: mouse => {
                        volumeButtonSingleClickTimer.stop();
                        trackHeadRoot._handleVolumeButtonDoubleClick();
                    }
                }

                Shotcut.HoverTip {
                    text: trackHeadRoot.inlineAudioControlsEnabled ? (qsTr('Mute/Unmute - Alt+Click to toggle mute of other tracks') + application.actionFirstShortcut('timelineToggleTrackMuteAction')) : (volumePopup.opened ? qsTr('Mute/Unmute - Alt+Click to toggle mute of other tracks') + application.actionFirstShortcut('timelineToggleTrackMuteAction') : qsTr('Adjust track volume - Alt+Click to toggle mute of other tracks') + application.actionFirstShortcut('timelineToggleTrackMuteAction'))
                }

                Popup {
                    id: volumePopup

                    parent: trackHeadRoot
                    x: Math.round(volumeButton.mapToItem(trackHeadRoot, 0, 0).x - padding)
                    y: Math.round(volumeButton.mapToItem(trackHeadRoot, 0, 0).y - padding)
                    width: 220
                    height: 36
                    padding: 6
                    modal: false
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                    onOpened: trackHeadRoot._positionVolumePopupOverVolumeButton()

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
                            width: volumeButton.width
                            height: volumeButton.height
                            padding: volumeButton.padding
                            focusPolicy: Qt.NoFocus
                            onClicked: {
                                timeline.toggleTrackMute(index);
                                volumePopup.close();
                            }

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
                            ToolTip.visible: false

                            MouseArea {
                                id: volumeSliderMouseArea

                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton
                                hoverEnabled: true
                                property bool _dragging: false
                                property real _pressX: 0
                                property real _pressY: 0

                                onPressed: mouse => {
                                        _dragging = false;
                                        _pressX = mouse.x;
                                        _pressY = mouse.y;
                                        mouse.accepted = true;
                                }

                                onDoubleClicked: mouse => {
                                        _dragging = false;
                                        trackHeadRoot._resetTrackGain();
                                        mouse.accepted = true;
                                }

                                onPositionChanged: mouse => {
                                    if (!(mouse.buttons & Qt.LeftButton))
                                        return;
                                    if (!_dragging) {
                                        if (Math.abs(mouse.x - _pressX) < 3 && Math.abs(mouse.y - _pressY) < 3)
                                            return;
                                        _dragging = true;
                                    }
                                    trackHeadRoot._setTrackGainFromMouse(volumeSlider, mouse.x);
                                }

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

            Item {
                width: 20
                height: 20

                ToolButton {
                    id: hideButton

                    anchors.centerIn: parent
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
            }

            Item {
                width: 20
                height: 20

                ToolButton {
                    id: filterButton

                    anchors.centerIn: parent
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

        Rectangle {
            visible: trackHeadRoot.inlineAudioControlsEnabled
                     && trackHeadRoot.trackAudioLevelSupported
            width: parent.width - 6
            height: trackHeadRoot.stackedHeaderLayout ? 28 : 30
            color: 'transparent'
            border.color: 'transparent'
            radius: 3

            Column {
                anchors.fill: parent
                spacing: 2

                Rectangle {
                    id: inlineMeter

                    width: parent.width
                    height: trackHeadRoot.stackedHeaderLayout ? 9 : 10
                    radius: 2
                    color: Qt.darker(activePalette.mid, 1.35)
                    border.color: activePalette.shadow
                    border.width: 1
                    clip: true

                    MouseArea {
                        id: inlineMeterMouseArea

                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        hoverEnabled: true

                        ToolTip {
                            parent: inlineMeterMouseArea
                            visible: inlineMeterMouseArea.containsMouse
                            delay: 0
                            timeout: -1
                            text: trackHeadRoot._indicatorAudioLevelText()
                        }
                    }

                    Item {
                        id: inlineMeterFill

                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(0,
                                        Math.round(parent.width * trackHeadRoot._inlineMeterScalePosition(Math.max(trackHeadRoot._minimumGainDb,
                                                                                                            Math.min(0.0,
                                                                                                              trackHeadRoot._indicatorAudioLevel())))))
                        height: parent.height - 2
                        clip: true

                        Rectangle {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: inlineMeter.width
                            height: parent.height
                            radius: 1
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop {
                                    position: 0.0
                                    color: 'darkgreen'
                                }

                                GradientStop {
                                    position: trackHeadRoot._inlineMeterScalePosition(-12.0)
                                    color: 'green'
                                }

                                GradientStop {
                                    position: trackHeadRoot._inlineMeterScalePosition(-6.0)
                                    color: 'yellow'
                                }

                                GradientStop {
                                    position: 1.0
                                    color: 'red'
                                }
                            }
                        }
                    }
                }

                Slider {
                    id: inlineVolumeSlider

                    width: parent.width
                    height: trackHeadRoot.stackedHeaderLayout ? 16 : 18
                    from: _minimumGainDb
                    to: _maximumGainDb
                    stepSize: 0.1
                    snapMode: Slider.SnapAlways
                    live: true
                    value: 0
                    onValueChanged: trackHeadRoot._setTrackGain(value)
                    ToolTip.visible: pressed || inlineSliderWheelTipTimer.running || inlineVolumeSliderMouseArea.containsMouse
                    ToolTip.text: trackHeadRoot._formatDb(value)

                    MouseArea {
                        id: inlineVolumeSliderMouseArea

                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true
                        property bool _dragging: false
                        property real _pressX: 0
                        property real _pressY: 0

                        onPressed: mouse => {
                            _dragging = false;
                            _pressX = mouse.x;
                            _pressY = mouse.y;
                            mouse.accepted = true;
                        }

                        onDoubleClicked: mouse => {
                            _dragging = false;
                            trackHeadRoot._resetTrackGain();
                            mouse.accepted = true;
                        }

                        onPositionChanged: mouse => {
                            if (!(mouse.buttons & Qt.LeftButton))
                                return;
                            if (!_dragging) {
                                if (Math.abs(mouse.x - _pressX) < 3 && Math.abs(mouse.y - _pressY) < 3)
                                    return;
                                _dragging = true;
                            }
                            trackHeadRoot._setTrackGainFromMouse(inlineVolumeSlider, mouse.x);
                        }

                        onWheel: wheel => {
                            const step = 0.1;
                            const delta = wheel.angleDelta.y > 0 ? step : -step;
                            inlineVolumeSlider.value = Math.max(inlineVolumeSlider.from,
                                                                Math.min(inlineVolumeSlider.to,
                                                                         Math.round((inlineVolumeSlider.value + delta) * 10)
                                                                         / 10));
                            inlineSliderWheelTipTimer.restart();
                            wheel.accepted = true;
                        }
                    }
                }
            }
        }
    }
}
