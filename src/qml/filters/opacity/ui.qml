/*
 * Copyright (c) 2014-2015 Meltytech, LLC
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
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    width: 350
    height: 50
    Component.onCompleted: {
        filter.set('start', 1)
        if (filter.isNew) {
            // Set default parameter values
            filter.set('level', 1)
            filter.set('alpha', 100.0 / 100.0)
            filter.set('opacity', filter.getDouble('alpha'))
            slider.value = filter.getDouble('alpha') * 100.0
        }
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Opacity')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: slider
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            value: filter.getDouble('alpha') * 100.0
            onValueChanged: {
                filter.set('alpha', value / 100.0)
                filter.set('opacity', filter.getDouble('alpha'))
            }
        }
        UndoButton {
            onClicked: slider.value = 100
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
