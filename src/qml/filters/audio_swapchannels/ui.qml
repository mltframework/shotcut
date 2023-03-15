/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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
    property string fromParameter: 'from'
    property string toParameter: 'to'

    width: 100
    height: 50
    Component.onCompleted: {
        if (application.audioChannels() === 1) {
            fromCombo.enabled = false;
            toCombo.enabled = false;
        } else if (application.audioChannels() === 6) {
            fromCombo.model = [qsTr('Front left'), qsTr('Front right'), qsTr('Center'), qsTr('Low frequency'), qsTr('Left surround'), qsTr('Right surround')];
        }
        filter.set('swap', 1);
        if (filter.isNew) {
            // Set default parameter values
            fromCombo.currentIndex = 0;
            filter.set(fromParameter, 0);
            toCombo.currentIndex = (application.audioChannels() === 1) ? 0 : 1;
            filter.set(fromParameter, toCombo.currentIndex);
        } else {
            // Initialize parameter values
            fromCombo.currentIndex = filter.get(fromParameter);
            toCombo.currentIndex = filter.get(toParameter);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label {
                text: qsTr('Swap')
            }

            Shotcut.ComboBox {
                id: fromCombo

                model: [qsTr('Left'), qsTr('Right')]
                onActivated: filter.set(fromParameter, currentIndex)
            }

            Label {
                text: qsTr('with')
            }

            Shotcut.ComboBox {
                id: toCombo

                model: fromCombo.model
                onActivated: filter.set(toParameter, currentIndex)
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
