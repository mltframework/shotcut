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

Item {
    width: 100
    height: 50
    property string fromParameter: 'from'
    property string toParameter: 'to'
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            combo.currentIndex = 0
        } else {
            // Initialize parameter values
            combo.currentIndex = filter.get(fromParameter) * 1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr('Copy from') }
            ComboBox {
                id: combo
                model: [qsTr('Left to right'), qsTr('Right to left')]
                onCurrentIndexChanged: {
                    filter.set(fromParameter, currentIndex)
                    filter.set(toParameter, 1 - currentIndex)
                }
            }
        }
        Item {
            Layout.fillHeight: true;
        }
    }
}
