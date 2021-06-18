/*
 * Copyright (c) 2018-2021 Meltytech, LLC
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
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import Shotcut.Controls 1.0 as Shotcut
import org.shotcut.qml 1.0

Item {
    id: root
    property double minimumValue: 0.0
    property double maximumValue: 1000.0
    property alias timeStr: timeField.text
    signal setDefaultClicked()

    implicitHeight: timeField.implicitHeight
    implicitWidth: timeField.implicitWidth + decrementButton.implicitWidth + incrementButton.implicitWidth + undoButton.implicitWidth

    function setValueSeconds(seconds) {
        timeStr = secondsToTime(seconds)
    }

    function getValueSeconds() {
        return timeToSeconds(timeStr)
    }

    function timeToSeconds(time) {
        var fields = time.split(":")
        if (fields[1] == undefined)
            fields[1] = 0
        if (fields[0] == undefined)
            fields[0] = 0
        if (fields[2] == undefined)
            fields[2] = 0
        var hSeconds = fields[0] * 60.0 * 60.0
        var mSeconds = fields[1] * 60.0
        var sSeconds = fields[2] * 1.0
        return hSeconds + mSeconds + sSeconds
    }

    function secondsToTime(seconds) {
        var h = Math.floor(seconds / 3600.0)
        var m = Math.floor((seconds - (h * 3600.0)) / 60.0)
        var s = seconds - (h * 3600.0) - (m * 60.0)
        return formatNum(h, 2, 0) + ":" + formatNum(m, 2, 0) + ":" + formatNum( s, 2, 3)
    }

    function clamp(x, min, max) {
        return Math.max(min, Math.min(max, x))
    }

    function formatNum(x, integerLen, fractionLen) {
        // Format a number as a string with specified padding zeros
        var a = x.toFixed(fractionLen) + ""
        var b = a.split(".")
        // Add leading zeros to the integer part (before decimal)
        while (b[0].length < integerLen) {
            b[0] = "0" + b[0]
        }
        if (fractionLen == 0)
            return b[0]
        // Add trailing zeros to the fraction part (after decimal)
        if (b[1] == undefined)
            b[1] = ""
        while (b[1].length < fractionLen) {
            b[1] = b[1] + "0"
        }
        return b[0] + "." + b[1]
    }

    RowLayout {
        spacing: 0

        TextField {
            id: timeField
            text: "00:00:00.000"
            horizontalAlignment: TextInput.AlignRight
            selectByMouse: true
            validator: RegExpValidator {regExp: /^\s*(\d*:){0,2}(\d*[.])?\d*\s*$/}
            onEditingFinished: {
                console.log(text, timeToSeconds(text))
                var seconds = clamp(timeToSeconds(text), minimumValue, maximumValue)
                text = secondsToTime(seconds)
            }
            Keys.onDownPressed: decrementAction.trigger()
            Keys.onUpPressed: incrementAction.trigger()
            onFocusChanged: if (focus) selectAll()
        }
        Shotcut.Button {
            id: decrementButton
            icon.name: 'list-remove'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
            Shotcut.HoverTip { text: qsTr('Decrement') }
            implicitWidth: 20
            implicitHeight: 20
            MouseArea {
                anchors.fill: parent
                onPressed: decrementAction.trigger()
                onPressAndHold: decrementTimer.start()
                onReleased: decrementTimer.stop()
            }
            Timer {
                id: decrementTimer
                repeat: true
                interval: 200
                triggeredOnStart: true
                onTriggered: decrementAction.trigger()
            }
        }
        Shotcut.Button {
            id: incrementButton
            icon.name: 'list-add'
            icon.source: 'qrc:///icons/oxygen/32x32/actions/list-add.png'
            Shotcut.HoverTip { text: qsTr('Increment') }
            implicitWidth: 20
            implicitHeight: 20
            MouseArea {
                anchors.fill: parent
                onPressed: incrementAction.trigger()
                onPressAndHold: incrementTimer.start()
                onReleased: incrementTimer.stop()
            }
            Timer {
                id: incrementTimer
                repeat: true
                interval: 200
                triggeredOnStart: true
                onTriggered: incrementAction.trigger()
            }
        }
        Shotcut.UndoButton {
            id: undoButton
            onClicked: root.setDefaultClicked()
        }
        Action {
            id: decrementAction
            onTriggered: {
                var frames = filter.framesFromTime(timeStr) - 1;
                if( frames < 0 )
                    frames = 0
                var newTime = filter.timeFromFrames(frames, Filter.TIME_CLOCK)
                var seconds = clamp(timeToSeconds(newTime), minimumValue, maximumValue)
                timeField.text = secondsToTime(seconds)
            }
        }
        Action {
            id: incrementAction
            onTriggered: {
                var frames = filter.framesFromTime(timeStr) + 1;
                var newTime = filter.timeFromFrames(frames, Filter.TIME_CLOCK)
                var seconds = clamp(timeToSeconds(newTime), minimumValue, maximumValue)
                timeField.text = secondsToTime(seconds)
            }
        }
    }
}
