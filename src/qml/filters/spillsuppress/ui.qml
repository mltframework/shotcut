/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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

Item {
    property string typeParam: '0'
    property double typeDefault: 0

    width: 200
    height: 50
    Component.onCompleted: {
        filter.set('threads', 0);
        if (filter.isNew)
            filter.set(typeParam, typeDefault);
        if (filter.getDouble(typeParam) === 0)
            greenRadioButton.checked = true;
        else
            blueRadioButton.checked = true;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            ButtonGroup {
                id: typeGroup
            }

            RadioButton {
                id: greenRadioButton

                text: qsTr('Green')
                ButtonGroup.group: typeGroup
                onClicked: filter.set(typeParam, 0)
            }

            RadioButton {
                id: blueRadioButton

                text: qsTr('Blue')
                ButtonGroup.group: typeGroup
                onClicked: filter.set(typeParam, 1)
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
