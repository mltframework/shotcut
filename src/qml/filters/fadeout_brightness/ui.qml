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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0

Item {
    width: 100
    height: 50
    objectName: 'fadeOut'
    property alias duration: timeSpinner.value

    Component.onCompleted: {
        if (filter.isNew) {
            duration = Math.ceil(settings.videoOutDuration * profile.fps)
            var out = filter.producerOut
            var inFrame = out - duration + 1
            filter.set('level', '0=1; %1=0'.arg(duration - 1))
            filter.set('alpha', 1)
            filter.set('in', inFrame)
            filter.set('out', out)
        }
        alphaCheckbox.checked = filter.get('alpha') != 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr('Duration') }
            TimeSpinner {
                id: timeSpinner
                minimumValue: 2
                maximumValue: 5000
                value: filter.getDouble('out') - filter.getDouble('in') + 1
                onValueChanged: {
                    var inFrame = filter.getDouble('out') - duration + 1
                    filter.set('in', inFrame)
                    if (filter.get('alpha') != 1)
                        filter.set('alpha', '0=1; %1=0'.arg(duration - 1))
                    else
                        filter.set('level', '0=1; %1=0'.arg(duration - 1))
                }
                onSetDefaultClicked: {
                    duration = Math.ceil(settings.videoOutDuration * profile.fps)
                }
                onSaveDefaultClicked: {
                    settings.videoOutDuration = duration / profile.fps
                }
            }
        }
        CheckBox {
            id: alphaCheckbox
            text: qsTr('Adjust opacity instead of fade with black')
            onClicked: {
                if (checked) {
                    filter.set('level', 1)
                    filter.set('alpha', '0=1; %1=0'.arg(duration - 1))
                } else {
                    filter.set('level', '0=1; %1=0'.arg(duration - 1))
                    filter.set('alpha', 1)
                }
            }
        }
        Item {
            Layout.fillHeight: true;
        }
    }
}
