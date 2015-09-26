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
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import Shotcut.Controls 1.0

Rectangle {
    width: 400
    height: 100
    color: 'transparent'
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('resource', filter.path + 'filter-demo.html')
            // Set default parameter values
            colorSwatch.value = 'black'
            filter.set('radius', 0.5)
            slider.value = filter.getDouble('radius') * slider.maximumValue
        }
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: slider
            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            value: filter.getDouble('radius') * slider.maximumValue
            onValueChanged: filter.set('radius', value / maximumValue)
        }
        UndoButton {
            onClicked: slider.value = 50
        }

        Label {
            text: qsTr('Color')
            Layout.alignment: Qt.AlignRight
        }
        ColorPicker {
            id: colorSwatch
            Layout.columnSpan: 2
            value: filter.get('color')
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    filter.set('color', '' + value)
                    filter.set("disable", 0);
                }
            }
            onPickStarted: {
                filter.set("disable", 1);
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
