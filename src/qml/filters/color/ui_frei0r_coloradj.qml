/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    property string paramRed: '0'
    property string paramGreen: '1'
    property string paramBlue: '2'
    property string paramAction: '3'
    property var defaultParameters: [paramRed, paramGreen, paramBlue, paramAction]

    function loadWheels() {
        wheel.color = Qt.rgba(filter.getDouble(paramRed), filter.getDouble(paramGreen), filter.getDouble(paramBlue), 1);
    }

    width: 450
    height: 250
    Component.onCompleted: {
        if (filter.isNew)
            filter.savePreset(defaultParameters);

        modeCombo.currentIndex = Math.round(filter.getDouble(paramAction) * 2);
        loadWheels();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        Shotcut.Preset {
            parameters: defaultParameters
            onPresetSelected: {
                modeCombo.currentIndex = Math.round(filter.getDouble(paramAction) * 2);
                loadWheels();
            }
        }

        RowLayout {
            Label {
                text: qsTr('Mode')
            }

            Shotcut.ComboBox {
                id: modeCombo

                Layout.minimumWidth: 200
                model: [qsTr('Shadows (Lift)'), qsTr('Midtones (Gamma)'), qsTr('Highlights (Gain)')]
                onActivated: filter.set(paramAction, currentIndex / 2)
            }

        }

        Shotcut.ColorWheelItem {
            id: wheel

            Layout.columnSpan: 2
            implicitWidth: (Math.min(parent.width, parent.height) - 60) * 1.1
            implicitHeight: Math.min(parent.width, parent.height) - 60
            Layout.alignment: Qt.AlignCenter | Qt.AlignTop
            Layout.minimumHeight: 75
            Layout.maximumHeight: 300
            onColorChanged: {
                filter.set(paramRed, wheel.red / 255);
                filter.set(paramGreen, wheel.green / 255);
                filter.set(paramBlue, wheel.blue / 255);
            }
        }

        Item {
            Layout.fillHeight: true
        }

    }

}
