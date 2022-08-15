/*
 * Copyright (c) 2021 Meltytech, LLC
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
    property string colorParam: 'filter.0'
    property string colorDefault: '#00ef00'
    property string distanceParam: 'filter.1'
    property double distanceDefault: 28.8
    property var defaultParameters: [colorParam, distanceParam]

    width: 350
    height: 50
    Component.onCompleted: {
        presetItem.parameters = defaultParameters;
        filter.set('filter', 'frei0r.bluescreen0r');
        filter.set('filter.2', 1);
        filter.set('filter.threads', 0);
        if (filter.isNew) {
            // Set default parameter values
            filter.set(colorParam, colorDefault);
            filter.set(distanceParam, distanceDefault / 100);
            filter.savePreset(defaultParameters);
        }
        colorPicker.value = filter.get(colorParam);
        distanceSlider.value = filter.getDouble(distanceParam) * 100;
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: presetItem

            Layout.columnSpan: 2
            onPresetSelected: {
                colorPicker.value = filter.get(colorParam);
                distanceSlider.value = filter.getDouble(distanceParam) * 100;
            }
        }

        // Row 1
        Label {
            text: qsTr('Key color')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ColorPicker {
            id: colorPicker

            onValueChanged: {
                filter.set(colorParam, value);
                filter.set('disable', 0);
            }
            onPickStarted: filter.set('disable', 1)
            onPickCancelled: filter.set('disable', 0)
        }

        Shotcut.UndoButton {
            onClicked: colorPicker.value = colorDefault
        }

        // Row 2
        Label {
            text: qsTr('Distance')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: distanceSlider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(distanceParam) * 100
            onValueChanged: filter.set(distanceParam, value / 100)
        }

        Shotcut.UndoButton {
            onClicked: distanceSlider.value = distanceDefault
        }

        Shotcut.TipBox {
            Layout.columnSpan: parent.columns
            Layout.fillWidth: true
            Layout.margins: 10
            text: qsTr('Tip: Mask other video filters by adding filters after this one followed by <b>Mask: Apply</b>')
        }

        Item {
            Layout.fillHeight: true
        }

    }

}
