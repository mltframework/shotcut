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

import QtQuick 2.15
import Shotcut.Controls 1.0 as Shotcut

Item {
    property var control: parent
    property alias readOnly: contextMenu.readOnly

    function popup() {
        contextMenu.popup();
    }

    Shotcut.EditContextMenu {
        id: contextMenu
    }

    Connections {
        function onUndoTriggered() {
            control.undo();
        }

        function onRedoTriggered() {
            control.redo();
        }

        function onCutTriggered() {
            control.cut();
        }

        function onCopyTriggered() {
            control.copy();
        }

        function onPasteTriggered() {
            control.paste();
        }

        function onDeleteTriggered() {
            control.remove(control.selectionStart, control.selectionEnd);
        }

        function onClearTriggered() {
            control.selectAll();
            control.remove(control.selectionStart, control.selectionEnd);
        }

        function onSelectAllTriggered() {
            control.selectAll();
        }

        target: contextMenu
    }

}
