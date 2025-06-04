/*
 * Copyright (c) 2014-2025 Meltytech, LLC
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
    property alias duration: timeSpinner.value
    property bool _blockUpdate: false

    function updateFilter() {
        var name = (filter.get('alpha') != 1) ? 'alpha' : 'level';
        filter.resetProperty(name);
        filter.set(name, 0, 0);
        filter.set(name, 1, Math.max(Math.min(duration, filter.duration), 2) - 1);
    }

    width: 100
    height: 50
    objectName: 'fadeIn'
    Component.onCompleted: {
        filter.blockSignals = true;
        if (filter.isNew) {
            filter.set('alpha', 1);
            duration = Math.ceil(settings.videoInDuration * profile.fps);
        } else if (filter.animateIn === 0) {
            // Convert legacy filter.
            duration = filter.duration;
            filter.set('in', producer.in);
            filter.set('out', producer.out);
        } else {
            duration = filter.animateIn;
        }
        alphaCheckbox.checked = filter.get('alpha') != 1;
        filter.blockSignals = false;
    }

    Connections {
        function onAnimateInChanged() {
            _blockUpdate = true;
            duration = filter.animateIn;
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

                minimumValue: 1
                maximumValue: 5000
                onValueChanged: {
                    if (_blockUpdate)
                        return;
                    filter.startUndoParameterCommand(durationLabel.text);
                    filter.animateIn = duration;
                    updateFilter();
                    filter.endUndoCommand();
                }
                onSetDefaultClicked: {
                    duration = Math.ceil(settings.videoInDuration * profile.fps);
                }
                onSaveDefaultClicked: {
                    settings.videoInDuration = duration / profile.fps;
                }
            }
        }

        CheckBox {
            id: alphaCheckbox

            text: qsTr('Adjust opacity instead of fade with black')
            onClicked: {
                if (checked) {
                    filter.set('alpha', 0);
                    filter.set('level', 1);
                } else {
                    filter.set('alpha', 1);
                }
                updateFilter();
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
