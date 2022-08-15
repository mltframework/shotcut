/*
 * Copyright (c) 2018-2021 Meltytech, LLC

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
    function setControls() {
        xScatter.value = filter.get('x_scatter');
        yScatter.value = filter.get('y_scatter');
        scale.value = filter.getDouble('scale');
        mix.value = filter.getDouble('mix');
    }

    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('x_scatter', 2);
            filter.set('y_scatter', 2);
            filter.set('scale', 1.5);
            filter.set('mix', 0);
            filter.set('invert', 0);
            filter.savePreset(preset.parameters);
        }
        setControls();
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
            id: preset

            parameters: ['x_scatter', 'y_scatter', 'scale', 'mix']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Line Width')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: xScatter

            minimumValue: 1
            maximumValue: 10
            stepSize: 1
            suffix: ' px'
            value: filter.get('x_scatter')
            onValueChanged: filter.set('x_scatter', value)
        }

        Shotcut.UndoButton {
            onClicked: xScatter.value = 2
        }

        Label {
            text: qsTr('Line Height')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: yScatter

            minimumValue: 1
            maximumValue: 10
            stepSize: 1
            suffix: ' px'
            value: filter.get('y_scatter')
            onValueChanged: filter.set('y_scatter', value)
        }

        Shotcut.UndoButton {
            onClicked: yScatter.value = 2
        }

        Label {
            text: qsTr('Contrast')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: scale

            minimumValue: 0
            maximumValue: 10
            stepSize: 1
            ratio: 0.1
            suffix: ' %'
            value: filter.get('scale')
            onValueChanged: filter.set('scale', value)
        }

        Shotcut.UndoButton {
            onClicked: scale.value = 1.5
        }

        Label {
            text: qsTr('Color')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: mix

            minimumValue: 0
            maximumValue: 10
            stepSize: 1
            ratio: 0.1
            suffix: ' %'
            value: filter.get('mix')
            onValueChanged: filter.set('mix', value)
        }

        Shotcut.UndoButton {
            onClicked: mix.value = 0
        }

        Item {
            Layout.fillHeight: true
        }

    }

}
