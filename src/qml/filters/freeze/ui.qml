/*
 * Copyright (c) 2019 Meltytech, LLC
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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0
import org.shotcut.qml 1.0

Item {
    width: 100
    height: 100

    Component.onCompleted: {
        if (filter.isNew) {
            updateFilter(producer.position)
        }
        filter.blockSignals = true
        timeSpinner.value = filter.getDouble('frame') - (isMultitrack()? producer.in : 0)
        filter.blockSignals = false
    }

    function isMultitrack() {
        return producer.get('ignore_points') !== '1'
    }

    function updateFilter(value) {
        filter.set('frame', filter.timeFromFrames(value + (isMultitrack()? producer.in : -producer.in), Filter.TIME_CLOCK))
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr('Frame') }
            TimeSpinner {
                id: timeSpinner
                undoButtonVisible: false
                saveButtonVisible: false
                minimumValue: 0
                maximumValue: 2147483647
                onValueChanged: updateFilter(value)
            }
            Button {
                iconName: 'view-history'
                iconSource: 'qrc:///icons/oxygen/32x32/actions/view-history.png'
                tooltip: qsTr('Use the current frame')
                implicitWidth: 20
                implicitHeight: 20
                onClicked: timeSpinner.value = producer.position + (isMultitrack()? 0 : producer.in)
            }
        }

        Item { Layout.fillHeight: true }
    }
}
