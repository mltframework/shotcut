/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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

import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    width: 350
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('start', 0.5)
            slider.value = filter.getDouble('start') * 1000
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr('Left') }
            SliderSpinner {
                id: slider
                minimumValue: 0
                maximumValue: 1000
                ratio: 1000
                decimals: 2
                label: qsTr('Right')
                value: filter.getDouble('start') * maximumValue
                onValueChanged: filter.set('start', value / maximumValue)
            }
            UndoButton {
                onClicked: slider.value = 500
            }
        }
        Item {
            Layout.fillHeight: true;
        }
    }
}
