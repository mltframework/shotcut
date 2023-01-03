/*
 * Copyright (c) 2020-2022 Meltytech, LLC
 * Written by Austin Brooks <ab.shotcut@outlook.com>
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
import QtQuick.Window
import Shotcut.Controls as Shotcut

Item {
    property string methodParam: 'av.method'
    property string methodDefault: 'garrote'
    property var methodValues: ['soft', 'garrote', 'hard']
    property string nstepsParam: 'av.nsteps'
    property int nstepsDefault: 7
    property string thresholdParam: 'av.threshold'
    property double thresholdDefault: 10
    property string percentParam: 'av.percent'
    property double percentDefault: 85
    property var allParams: [methodParam, nstepsParam, thresholdParam, percentParam]

    function getComboIndex(value, values) {
        for (var i = 0; i < values.length; ++i) {
            if (values[i] === value)
                return i;
        }
        return -1;
    }

    function setControls() {
        idMethod.currentIndex = getComboIndex(filter.get(methodParam), methodValues);
        idNsteps.value = parseInt(filter.get(nstepsParam));
        idThreshold.value = filter.getDouble(thresholdParam);
        idPercent.value = filter.getDouble(percentParam);
    }

    function getMaxSteps() {
        var min = Math.min(profile.width, profile.height);
        var i = 1;
        while (Math.pow(2, i) <= min)
            ++i;
        return i - 1;
    }

    width: 350
    height: 200
    Component.onCompleted: {
        filter.blockSignals = true;
        if (filter.isNew) {
            // Custom preset
            filter.set(methodParam, 'garrote');
            filter.set(nstepsParam, 5);
            filter.set(thresholdParam, 7);
            filter.set(percentParam, 85);
            filter.savePreset(allParams, qsTr('Light'));
            // Custom preset
            filter.set(methodParam, 'garrote');
            filter.set(nstepsParam, 7);
            filter.set(thresholdParam, 10);
            filter.set(percentParam, 85);
            filter.savePreset(allParams, qsTr('Medium'));
            // Custom preset
            filter.set(methodParam, 'garrote');
            filter.set(nstepsParam, 8);
            filter.set(thresholdParam, 16);
            filter.set(percentParam, 85);
            filter.savePreset(allParams, qsTr('Heavy'));
            // Default preset
            filter.set(methodParam, methodDefault);
            filter.set(nstepsParam, nstepsDefault);
            filter.set(thresholdParam, thresholdDefault);
            filter.set(percentParam, percentDefault);
            filter.savePreset(allParams);
        }
        filter.blockSignals = false;
        setControls();
    }

    GridLayout {

        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Filler
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: idPreset

            Layout.columnSpan: 2
            parameters: allParams
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Method')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: idMethod

            implicitWidth: 180
            model: [qsTr('Soft'), qsTr('Garrote'), qsTr('Hard', 'Remove Noise Wavelet filter')]
            onActivated: filter.set(methodParam, methodValues[currentIndex])
        }

        Shotcut.UndoButton {
            onClicked: {
                idMethod.currentIndex = getComboIndex(methodDefault, methodValues);
                filter.set(methodParam, methodValues[idMethod.currentIndex]);
            }
        }

        Label {
            text: qsTr('Decompose')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: idNsteps

            minimumValue: 1
            maximumValue: 32
            onValueChanged: filter.set(nstepsParam, value)
        }

        Shotcut.UndoButton {
            onClicked: idNsteps.value = nstepsDefault
        }

        Label {
            text: qsTr('Threshold')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: idThreshold

            minimumValue: 0
            maximumValue: 64
            onValueChanged: filter.set(thresholdParam, value)
        }

        Shotcut.UndoButton {
            onClicked: idThreshold.value = thresholdDefault
        }

        Label {
            text: qsTr('Percent')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: idPercent

            minimumValue: 0
            maximumValue: 100
            suffix: ' %'
            onValueChanged: filter.set(percentParam, value)
        }

        Shotcut.UndoButton {
            onClicked: idPercent.value = percentDefault
        }

        Label {
            text: qsTr('Max decompositions for the current video mode') + ': ' + getMaxSteps()
            Layout.alignment: Qt.AlignHCenter
            Layout.columnSpan: 3
        }

        Label {
            text: qsTr('More information') + ': <a href="http://ffmpeg.org/ffmpeg-all.html#vaguedenoiser">FFmpeg vaguedenoiser</a>'
            Layout.alignment: Qt.AlignHCenter
            Layout.columnSpan: 3
            onLinkActivated: Qt.openUrlExternally(link)

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }

        Item {
            Layout.columnSpan: 3
            Layout.fillHeight: true
        }
    }
}
