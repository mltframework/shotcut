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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0
import QtQuick.Controls.Styles 1.1

Item {
    property var defaultParameters: []
    property var neutralParam: ""
    property var tempParam: ""
    property var defaultNeutral: "#7f7f7f"
    property var defaultTemp: 6500.0
    property var tempScale: 15000.0
    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.get("mlt_service").indexOf("movit.") == 0 ) {
            // Movit filter
            neutralParam = "neutral_color"
            tempParam = "color_temperature"
            tempScale = 1;
        } else {
            // Frei0r filter
            neutralParam = "0"
            tempParam = "1"
            tempScale = 15000.0
            filter.set('threads', 0)
        }
        defaultParameters = [neutralParam, tempParam]
        presetItem.parameters = defaultParameters

        if (filter.isNew) {
            // Set default parameter values
            filter.set(neutralParam, defaultNeutral)
            filter.set(tempParam, defaultTemp / tempScale)
            filter.savePreset(defaultParameters)
        }
        tempslider.value = filter.getDouble(tempParam) * tempScale
        tempspinner.value = filter.getDouble(tempParam) * tempScale
        colorPicker.value = filter.get(neutralParam)
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8
        
        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: presetItem
            Layout.columnSpan: 2
            onPresetSelected: {
                tempslider.value = filter.getDouble(tempParam) * tempScale
                colorPicker.value = filter.get(neutralParam)
            }
        }
        
        // Row 1
        Label {
            text: qsTr('Neutral color')
            Layout.alignment: Qt.AlignRight
        }
        ColorPicker {
            id: colorPicker
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    filter.set(neutralParam, ""+value)
                    filter.set("disable", 0);
                }
            }
            onPickStarted: {
                filter.set("disable", 1);
            }
            onPickCancelled: filter.set('disable', 0)
        }
        UndoButton {
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
                Layout.fillWidth: true
                Layout.maximumHeight: tempspinner.height
                style: SliderStyle {
                    groove: Rectangle {
                        rotation: -90
                        height: parent.width
                        x: (parent.width - width) / 2
                        gradient: Gradient {
                            GradientStop { position: 0.000; color: "#FFC500" }
                            GradientStop { position: 0.392; color: "#FFFFFF" }
                            GradientStop { position: 1.000; color: "#DDFFFE" }
                        }
                        radius: 4
                        onWidthChanged: {
                            // Force width (which is really height due to rotation).
                            width = tempslider.height / 2
                        }
                    }
                    handle: Rectangle {
                        color: "lightgray"
                        border.color: "gray"
                        border.width: 2
                        width: height / 2
                        height: tempslider.height
                        radius: 4
                    }
                }
                minimumValue: 1000.0
                maximumValue: 15000.0
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        tempspinner.value = value
                        filter.set(tempParam, value / tempScale)
                    }
                }
            }
            SpinBox {
                id: tempspinner
                Layout.minimumWidth: 100
                minimumValue: 1000.0
                maximumValue: 15000.0
                decimals: 0
                stepSize: 10
                suffix: qsTr(' deg', 'degrees')
                onValueChanged: tempslider.value = value
            }
        }
        UndoButton {
            onClicked: tempslider.value = defaultTemp
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
