/*
 * Copyright (c) 2014-2024 Meltytech, LLC
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
import org.shotcut.qml

Item {
    property alias duration: timeSpinner.value
    property bool _blockUpdate: false

    function updateFilter() {
        var filterDuration = producer.duration;
        filter.set('opacity', '%1$=1; %2=0'.arg(Math.max(filterDuration - duration, 0)).arg(filterDuration - 1));
    }

    width: 100
    height: 50
    objectName: 'fadeOut'
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('alpha', 1);
            duration = Math.ceil(settings.videoOutDuration * profile.fps);
        } else if (filter.animateOut === 0) {
            // Convert legacy filter.
            duration = filter.duration;
            filter.set('in', producer.in);
            filter.set('out', producer.out);
        } else {
            duration = filter.animateOut;
        }
        alphaCheckbox.checked = filter.get('alpha') != 1;
    }

    Connections {
        function onAnimateOutChanged() {
            _blockUpdate = true;
            duration = filter.animateOut;
            _blockUpdate = false;
        }

        target: filter
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label {
                id: durationLabel
                text: qsTr('Duration')
            }

            Shotcut.TimeSpinner {
                id: timeSpinner

                minimumValue: 2
                maximumValue: 5000
                onValueChanged: {
                    if (_blockUpdate)
                        return;
                    filter.startUndoParameterCommand(durationLabel.text);
                    filter.animateOut = duration;
                    updateFilter();
                    filter.endUndoCommand();
                }
                onSetDefaultClicked: {
                    duration = Math.ceil(settings.videoOutDuration * profile.fps);
                }
                onSaveDefaultClicked: {
                    settings.videoOutDuration = duration / profile.fps;
                }
            }
        }

        CheckBox {
            id: alphaCheckbox

            text: qsTr('Adjust opacity instead of fade with black')
            // When =-1, alpha follows opacity value.
            onClicked: filter.set('alpha', checked ? -1 : 1)
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
