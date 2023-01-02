/*
 * Copyright (c) 2019-2021 Meltytech, LLC
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
    property string amount: 'av.frames'
    property int amountDefault: 2

    function updateFilter(value) {
        filter.set(amount, value);
        var weights = [];
        for (var i = 1; i <= value; i++)
            weights.push(i);
        filter.set('av.weights', weights.join(' '));
    }

    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            updateFilter(amountDefault);
            filter.savePreset(preset.parameters);
        }
        amountSlider.value = filter.get(amount);
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 4

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: [amount]
            Layout.columnSpan: parent.columns - 1
            onPresetSelected: amountSlider.value = filter.get(amount)
        }

        Label {
            text: qsTr('Amount')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: amountSlider

            minimumValue: 2
            maximumValue: Math.round(profile.fps)
            stepSize: 1
            suffix: qsTr(' frames')
            spinnerWidth: 110
            onValueChanged: updateFilter(value)
        }

        Shotcut.UndoButton {
            onClicked: amountSlider.value = amountDefault
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
