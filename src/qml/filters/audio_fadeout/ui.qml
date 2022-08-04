/*
 * Copyright (c) 2014-2022 Meltytech, LLC
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
    width: 100
    height: 50
    objectName: 'fadeOut'
    property alias duration: timeSpinner.value

    Component.onCompleted: {
        if (filter.isNew) {
            duration = Math.ceil(settings.audioOutDuration * profile.fps)
        } else if (filter.animateOut === 0) {
            // Convert legacy filter.
            duration = filter.duration
            filter.set('in', producer.in )
            filter.set('out', producer.out )
        } else {
            duration = filter.animateOut
        }
    }

    Connections {
        target: filter
        function onAnimateOutChanged() {
            duration = filter.animateOut
        }
    }

    function updateFilter() {
        filter.resetProperty('level')
        filter.set('level', 0, Math.max(filter.duration - duration, 0))
        filter.set('level', -60, filter.duration - 1)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr('Duration') }
            Shotcut.TimeSpinner {
                id: timeSpinner
                minimumValue: 2
                maximumValue: 5000
                onValueChanged: {
                    filter.animateOut = duration
                    updateFilter()
                }
                onSetDefaultClicked: {
                    duration = Math.ceil(settings.audioOutDuration * profile.fps)
                }
                onSaveDefaultClicked: {
                    settings.audioOutDuration = duration / profile.fps
                }
            }
        }
        Item {
            Layout.fillHeight: true;
        }
    }
}
