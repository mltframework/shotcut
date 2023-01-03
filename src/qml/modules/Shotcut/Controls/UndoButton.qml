/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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
import QtQuick.Controls
import Shotcut.Controls as Shotcut

Shotcut.Button {
    icon.name: 'edit-undo'
    icon.source: 'qrc:///icons/oxygen/32x32/actions/edit-undo.png'
    implicitWidth: 20
    implicitHeight: 20

    Shotcut.HoverTip {
        text: qsTr('Reset to default')
    }
}
