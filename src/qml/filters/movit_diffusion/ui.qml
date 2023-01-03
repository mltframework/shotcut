/*
 * Copyright (c) 2014-2022 Meltytech, LLC
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

Item {
    width: 350
    height: 50

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: radiusSlider

            minimumValue: 0
            maximumValue: 2000
            ratio: 100
            decimals: 2
            stepSize: 0.1
            value: filter.getDouble('radius') * 100
            onValueChanged: filter.set('radius', value / 100)
        }

        Shotcut.UndoButton {
            onClicked: radiusSlider.value = 300
        }

        Label {
            text: qsTr('Blurriness')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: mixSlider

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            value: filter.getDouble('mix') * 100
            onValueChanged: filter.set('mix', value / 100)
        }

        Shotcut.UndoButton {
            onClicked: mixSlider.value = 30
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
