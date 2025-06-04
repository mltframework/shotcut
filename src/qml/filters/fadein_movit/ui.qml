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
import org.shotcut.qml

Item {
    property alias duration: timeSpinner.value
    property bool _blockUpdate: false

    width: 100
    height: 50
    objectName: 'fadeIn'
    Component.onCompleted: {
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

                minimumValue: 2
                maximumValue: 5000
                onValueChanged: {
                    if (_blockUpdate)
                        return;
                    filter.startUndoParameterCommand(durationLabel.text);
                    filter.animateIn = duration;
                    filter.resetProperty('opacity');
                    filter.set('opacity', 0, 0, KeyframesModel.SmoothNaturalInterpolation);
                    filter.set('opacity', 1, Math.max(Math.min(duration, filter.duration), 2) - 1);
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
            // When =-1, alpha follows opacity value.
            onClicked: filter.set('alpha', checked ? -1 : 1)
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
