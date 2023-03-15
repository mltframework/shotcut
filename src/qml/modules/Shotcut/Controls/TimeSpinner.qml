/*
 * Copyright (c) 2014-2022 Meltytech, LLC
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

RowLayout {
    id: root

    property int minimumValue: 0
    property int maximumValue: 99
    property int value: 0
    property alias undoButtonVisible: undoButton.visible
    property alias saveButtonVisible: saveButton.visible

    signal setDefaultClicked
    signal saveDefaultClicked

    function clamp(x, min, max) {
        return Math.max(min, Math.min(max, x));
    }

    spacing: 0

    TextField {
        id: timeField

        text: filter.timeFromFrames(clamp(value, minimumValue, maximumValue))
        horizontalAlignment: TextInput.AlignRight
        selectByMouse: true
        onEditingFinished: value = filter.framesFromTime(text)
        Keys.onDownPressed: decrementAction.trigger()
        Keys.onUpPressed: incrementAction.trigger()
        onFocusChanged: {
            if (focus)
                selectAll();
        }

        validator: RegularExpressionValidator {
            regularExpression: /^\s*(\d*:){0,2}(\d*[.;:])?\d*\s*$/
        }
    }

    Shotcut.Button {
        id: decrementButton

        icon.name: 'list-remove'
        icon.source: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
        implicitWidth: 20
        implicitHeight: 20

        Shotcut.HoverTip {
            text: qsTr('Decrement')
        }

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
        implicitWidth: 20
        implicitHeight: 20

        Shotcut.HoverTip {
            text: qsTr('Increment')
        }

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

    Shotcut.SaveDefaultButton {
        id: saveButton

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
}
