/*
 * Copyright (c) 2015-2021 Meltytech, LLC
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
    id: background

    function setControls() {
        sliderLow.value = filter.getDouble('0');
        sliderMid.value = filter.getDouble('1');
        sliderHigh.value = filter.getDouble('2');
    }

    width: 350
    height: 200
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('0', 0);
            filter.set('1', 0);
            filter.set('2', 0);
            filter.set('wetness', 1);
            filter.savePreset(preset.parameters);
        }
        setControls();
    }

    SystemPalette {
        id: activePalette
    }

    Rectangle {
        id: topLine

        height: 1
        width: lowColumn.width * 3 + 20
        color: activePalette.text
        opacity: 0.5

        anchors {
            left: gridLayout.left
            leftMargin: gridLayout.leftMargin
            top: gridLayout.top
            topMargin: presetLayout.height + gridLayout.anchors.margins
        }

    }

    Label {
        text: '+12 dB'
        font.pointSize: bassLabel.font.pointSize - 1

        anchors {
            right: topLine.left
            rightMargin: 8
            verticalCenter: topLine.verticalCenter
        }

    }

    Rectangle {
        height: 1
        width: lowColumn.width * 3 + gridLayout.anchors.margins * 2
        color: activePalette.text
        opacity: 0.5

        anchors {
            left: gridLayout.left
            leftMargin: gridLayout.leftMargin
            top: topLine.top
            topMargin: sliderLow.height * 0.25 + 1
        }

    }

    Rectangle {
        id: zeroLine

        height: 2
        width: lowColumn.width * 3 + gridLayout.anchors.margins * 2
        color: activePalette.text
        opacity: 0.5

        anchors {
            left: gridLayout.left
            leftMargin: gridLayout.leftMargin
            top: topLine.top
            topMargin: sliderLow.height * 0.5 - 4
        }

    }

    Label {
        text: '0 dB'
        font.pointSize: bassLabel.font.pointSize - 1

        anchors {
            right: zeroLine.left
            rightMargin: 8
            verticalCenter: zeroLine.verticalCenter
        }

    }

    Rectangle {
        height: 1
        width: lowColumn.width * 3 + gridLayout.anchors.margins * 2
        color: activePalette.text
        opacity: 0.5

        anchors {
            left: gridLayout.left
            leftMargin: gridLayout.leftMargin
            top: topLine.top
            topMargin: sliderLow.height * 0.75 - 8
        }

    }

    Rectangle {
        id: bottomLine

        height: 1
        width: lowColumn.width * 3 + gridLayout.anchors.margins * 2
        color: activePalette.text
        opacity: 0.5

        anchors {
            left: gridLayout.left
            leftMargin: gridLayout.leftMargin
            bottom: gridLayout.bottom
            bottomMargin: bassLabel.height + gridLayout.anchors.margins
        }

    }

    Label {
        text: '-12 dB'
        font.pointSize: bassLabel.font.pointSize - 1

        anchors {
            right: bottomLine.left
            rightMargin: 8
            verticalCenter: bottomLine.verticalCenter
        }

    }

    GridLayout {
        id: gridLayout

        property double leftMargin: (width - lowColumn.width * 3 - columnSpacing * 2) / 2

        anchors.fill: parent
        anchors.margins: 8
        columns: 5

        RowLayout {
            id: presetLayout

            spacing: 8
            Layout.columnSpan: parent.columns

            Label {
                text: qsTr('Preset')
                Layout.alignment: Qt.AlignRight
            }

            Shotcut.Preset {
                id: preset

                parameters: ['0', '1', '2']
                onPresetSelected: setControls()
            }

        }

        Item {
            Layout.fillWidth: true
        }

        ColumnLayout {
            id: lowColumn

            Layout.minimumWidth: 80
            Layout.alignment: Qt.AlignHCenter

            Slider {
                id: sliderLow

                Layout.alignment: Qt.AlignHCenter
                Layout.fillHeight: true
                orientation: Qt.Vertical
                from: -12
                to: 12
                value: filter.getDouble('0')
                onValueChanged: filter.set('0', value)

                Shotcut.HoverTip {
                    text: '%1 dB'.arg(Math.round(parent.value * 10) / 10)
                }

            }

            Label {
                id: bassLabel

                text: qsTr('Bass')
                Layout.alignment: Qt.AlignHCenter

                Shotcut.HoverTip {
                    text: '100 Hz'
                }

            }

        }

        ColumnLayout {
            Layout.minimumWidth: 80
            Layout.alignment: Qt.AlignHCenter

            Slider {
                id: sliderMid

                Layout.alignment: Qt.AlignHCenter
                Layout.fillHeight: true
                orientation: Qt.Vertical
                from: -12
                to: 12
                value: filter.getDouble('1')
                onValueChanged: filter.set('1', value)

                Shotcut.HoverTip {
                    text: '%1 dB'.arg(Math.round(parent.value * 10) / 10)
                }

            }

            Label {
                text: qsTr('Middle', 'Bass & Treble audio filter')
                Layout.alignment: Qt.AlignHCenter

                Shotcut.HoverTip {
                    text: '1000 Hz'
                }

            }

        }

        ColumnLayout {
            Layout.minimumWidth: 80
            Layout.alignment: Qt.AlignHCenter

            Slider {
                id: sliderHigh

                Layout.alignment: Qt.AlignHCenter
                Layout.fillHeight: true
                orientation: Qt.Vertical
                from: -12
                to: 12
                value: filter.getDouble('2')
                onValueChanged: filter.set('2', value)

                Shotcut.HoverTip {
                    text: '%1 dB'.arg(Math.round(parent.value * 10) / 10)
                }

            }

            Label {
                text: qsTr('Treble')
                Layout.alignment: Qt.AlignHCenter

                Shotcut.HoverTip {
                    text: '10000 Hz'
                }

            }

        }

        Item {
            Layout.fillWidth: true
        }

    }

}
