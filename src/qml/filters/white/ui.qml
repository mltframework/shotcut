/*
 * Copyright (c) 2014-2020 Meltytech, LLC
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
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0 as Shotcut

Item {
    property var defaultParameters: []
    property var neutralParam: ""
    property var tempParam: ""
    property var defaultNeutral: "#7f7f7f"
    property var defaultTemp: 6500
    property var tempScale: 15000

    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.get("mlt_service").indexOf("movit.") == 0) {
            // Movit filter
            neutralParam = "neutral_color";
            tempParam = "color_temperature";
            tempScale = 1;
        } else {
            // Frei0r filter
            neutralParam = "0";
            tempParam = "1";
            tempScale = 15000;
            filter.set('threads', 0);
        }
        defaultParameters = [neutralParam, tempParam];
        presetItem.parameters = defaultParameters;
        if (filter.isNew) {
            // Set default parameter values
            filter.set(neutralParam, defaultNeutral);
            filter.set(tempParam, defaultTemp / tempScale);
            filter.savePreset(defaultParameters);
        }
        tempslider.value = filter.getDouble(tempParam) * tempScale;
        tempspinner.value = filter.getDouble(tempParam) * tempScale;
        colorPicker.value = filter.get(neutralParam);
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
                tempslider.value = filter.getDouble(tempParam) * tempScale;
                colorPicker.value = filter.get(neutralParam);
            }
        }

        // Row 1
        Label {
            text: qsTr('Neutral color')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ColorPicker {
            id: colorPicker

            property bool isReady: false

            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    filter.set(neutralParam, "" + value);
                    filter.set("disable", 0);
                }
            }
            onPickStarted: {
                filter.set("disable", 1);
            }
            onPickCancelled: filter.set('disable', 0)
        }

        Shotcut.UndoButton {
            onClicked: colorPicker.value = defaultNeutral
        }

        // Row 2
        Label {
            text: qsTr('Color temperature')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Slider {
                id: tempslider

                property bool isReady: false

                Layout.fillWidth: true
                implicitHeight: tempspinner.implicitHeight
                padding: 0
                from: 1000
                to: 15000
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        tempspinner.value = value;
                        filter.set(tempParam, value / tempScale);
                    }
                }

                background: Rectangle {
                    height: tempslider.availableHeight / 2
                    width: tempslider.availableWidth
                    x: tempslider.leftPadding
                    y: tempslider.topPadding + tempslider.availableHeight / 2 - height / 2
                    radius: 4

                    gradient: Gradient {
                        orientation: Gradient.Horizontal

                        GradientStop {
                            position: 0
                            color: "#FFC500"
                        }

                        GradientStop {
                            position: 0.392
                            color: "#FFFFFF"
                        }

                        GradientStop {
                            position: 1
                            color: "#DDFFFE"
                        }
                    }
                }

                handle: Rectangle {
                    x: tempslider.leftPadding + tempslider.visualPosition * (tempslider.availableWidth - width)
                    y: tempslider.topPadding + tempslider.availableHeight / 2 - height / 2
                    color: "lightgray"
                    border.color: "gray"
                    border.width: 2
                    width: height / 2
                    height: tempslider.height
                    radius: 4
                }
            }

            Shotcut.DoubleSpinBox {
                id: tempspinner

                Layout.minimumWidth: 150
                from: 1000
                to: 15000
                stepSize: 10
                decimals: 0
                suffix: qsTr('degrees')
                onValueModified: tempslider.value = value
            }
        }

        Shotcut.UndoButton {
            onClicked: tempslider.value = defaultTemp
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
