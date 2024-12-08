/*
 * Copyright (c) 2024 Meltytech, LLC
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
    property var defaultParameters: ['stop.1', 'stop.2']
    property bool _disableUpdate: true

    function setControls() {
        _disableUpdate = true;
        fgGradient.parseStops(filter.getGradient('stop'));
        _disableUpdate = false;
    }

    width: 350
    height: 180
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('stop.1', '#000000 0.0');
            filter.set('stop.2', '#ffffff 1.0');
            filter.savePreset(defaultParameters);
        }
        setControls();
    }

    GridLayout {
        columns: 5
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: defaultParameters
            Layout.columnSpan: 4
            onPresetSelected: setControls()
            onBeforePresetLoaded: {
                // Clear all gradient colors before loading the new values
                filter.setGradient('stop', []);
            }
        }

        Label {
            text: qsTr('Gradient Stops')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.GradientMapControl {
            id: fgGradient

            Layout.columnSpan: 5
            onGradientChanged: {
                if (_disableUpdate)
                    return;
                filter.setGradient('stop', stringizeStops());
            }
        }
    }
}
