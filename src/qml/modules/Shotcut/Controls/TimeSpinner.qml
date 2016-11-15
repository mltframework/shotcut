/*
 * Copyright (c) 2014-2016 Meltytech, LLC
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
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1

RowLayout {
    id: root
    property int minimumValue: 0
    property int maximumValue: 99
    property int value: 0
    signal setDefaultClicked()
    signal saveDefaultClicked()

    spacing: 0

    TextField {
        id: timeField
        text: filter.timeFromFrames(clamp(value, minimumValue, maximumValue))
        horizontalAlignment: TextInput.AlignRight
        validator: RegExpValidator {regExp: /^\s*(\d*:){0,2}(\d*[.;:])?\d*\s*$/}
        onEditingFinished: value = filter.framesFromTime(text)
        Keys.onDownPressed: decrementAction.trigger()
        Keys.onUpPressed: incrementAction.trigger()
        onFocusChanged: if (focus) selectAll()
    }
    Button {
        id: decrementButton
        iconName: 'list-remove'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
        tooltip: qsTr('Decrement')
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
    Button {
        id: incrementButton
        iconName: 'list-add'
        iconSource: 'qrc:///icons/oxygen/32x32/actions/list-add.png'
        tooltip: qsTr('Increment')
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
    UndoButton {
        onClicked: root.setDefaultClicked()
    }
    SaveDefaultButton {
        onClicked: root.saveDefaultClicked()
    }
    Action {
        id: decrementAction
        onTriggered: value = Math.max(value - 1, minimumValue)
    }
    Action {
        id: incrementAction
        onTriggered: value = Math.min(value + 1, maximumValue)
    }

    function clamp(x, min, max) {
        return Math.max(min, Math.min(max, x))
    }
}
