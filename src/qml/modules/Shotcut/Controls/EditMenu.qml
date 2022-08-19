/*
 * Copyright (c) 2021 Meltytech, LLC
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

Menu {
    id: menu

    property var control: parent

    width: 220

    MenuItem {

        action: Action {
            text: qsTr('Undo') + (application.OS === 'OS X' ? '    ⌘Z' : ' (Ctrl+Z)')
            onTriggered: control.undo()
        }

    }

    MenuItem {

        action: Action {
            text: qsTr('Redo') + (application.OS === 'Windows' ? ' (Ctrl+Y)' : application.OS === 'OS X' ? '    ⇧⌘Z' : ' (Ctrl+Shift+Z)')
            onTriggered: control.redo()
        }

    }

    MenuSeparator {
    }

    MenuItem {

        action: Action {
            text: qsTr('Cut') + (application.OS === 'OS X' ? '    ⌘X' : ' (Ctrl+X)')
            onTriggered: control.cut()
        }

    }

    MenuItem {

        action: Action {
            text: qsTr('Copy') + (application.OS === 'OS X' ? '    ⌘C' : ' (Ctrl+C)')
            onTriggered: control.copy()
        }

    }

    MenuItem {

        action: Action {
            text: qsTr('Paste') + (application.OS === 'OS X' ? '    ⌘V' : ' (Ctrl+V)')
            onTriggered: control.paste()
        }

    }

    MenuItem {

        action: Action {
            text: qsTr('Delete')
            onTriggered: control.remove(control.selectionStart, control.selectionEnd)
        }

    }

    MenuItem {

        action: Action {
            text: qsTr('Clear')
            onTriggered: {
                control.selectAll();
                control.remove(control.selectionStart, control.selectionEnd);
            }
        }

    }

    MenuSeparator {
    }

    MenuItem {

        action: Action {
            text: qsTr('Select All') + (application.OS === 'OS X' ? '    ⌘A' : ' (Ctrl+A)')
            onTriggered: control.selectAll()
        }

    }

    MenuItem {
        text: qsTr('Cancel')
        onTriggered: menu.dismiss()
    }

}
