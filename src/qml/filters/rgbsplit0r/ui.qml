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

import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0

Item {
    property string verSplit: '0'
    property string horSplit: '1'
    property double verSplitDefault: 0.5
    property double horSplitDefault: 0.5
    property real maxFilterPercent: 100.0
    property real maxUserPercent: 100.0
    property real defaultValue: 2.5 / maxFilterPercent

    width: 350
    height: 100

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(verSplit, verSplitDefault)
            filter.set(horSplit, horSplitDefault)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        verSplitSlider.value = filter.getDouble(verSplit) * maxFilterPercent
        horSplitSlider.value = filter.getDouble(horSplit) * maxFilterPercent
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 3

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: [verSplit, horSplit]
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Vertical')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: verSplitSlider
            minimumValue: 0
            maximumValue: maxUserPercent
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: filter.set(verSplit, value / maxFilterPercent)
        }
        UndoButton {
            onClicked: verSplitSlider.value = verSplitDefault * maxFilterPercent
        }

        Label {
            text: qsTr('Horizontal')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: horSplitSlider
            minimumValue: 0
            maximumValue: maxUserPercent
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: filter.set(horSplit, value / maxFilterPercent)
            }
        UndoButton {
            onClicked: horSplitSlider.value = horSplitDefault * maxFilterPercent
        }

        Item {
            Layout.fillHeight: true;
        }
    }
  }
