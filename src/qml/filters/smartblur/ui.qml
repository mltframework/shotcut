/*
 * Copyright (c) 2016-2021 Meltytech, LLC
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
    property double radiusDefault: 2.5
    property double strengthDefault: 0.5
    property double thresholdDefault: 3
    property var defaultParameters: ["av.luma_radius", "av.chroma_radius", "av.luma_strength", "av.chroma_strength", "av.luma_threshold", "av.chroma_threshold"]

    function setControls() {
        radiusSlider.value = filter.getDouble("av.luma_radius");
        strengthSlider.value = filter.getDouble("av.luma_strength");
        thresholdSlider.value = filter.getDouble("av.luma_threshold");
    }

    width: 200
    height: 125
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set("av.luma_radius", radiusDefault);
            filter.set("av.chroma_radius", radiusDefault);
            filter.set("av.luma_strength", strengthDefault);
            filter.set("av.chroma_strength", strengthDefault);
            filter.set("av.luma_threshold", thresholdDefault);
            filter.set("av.chroma_threshold", thresholdDefault);
            filter.savePreset(defaultParameters);
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
            id: presetItem

            Layout.columnSpan: 2
            parameters: defaultParameters
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Blur Radius')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The radius of the gaussian blur.')
            }
        }

        Shotcut.SliderSpinner {
            id: radiusSlider

            minimumValue: 0.1
            maximumValue: 5
            decimals: 1
            onValueChanged: {
                filter.set("av.luma_radius", value);
                filter.set("av.chroma_radius", value);
            }
        }

        Shotcut.UndoButton {
            onClicked: radiusSlider.value = radiusDefault
        }

        Label {
            text: qsTr('Blur Strength')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The strength of the gaussian blur.')
            }
        }

        Shotcut.SliderSpinner {
            id: strengthSlider

            minimumValue: 0
            maximumValue: 1
            decimals: 1
            onValueChanged: {
                filter.set("av.luma_strength", value);
                filter.set("av.chroma_strength", value);
            }
        }

        Shotcut.UndoButton {
            onClicked: strengthSlider.value = strengthDefault
        }

        Label {
            text: qsTr('Threshold')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('If the difference between the original pixel and the blurred pixel is less than threshold, the pixel will be replaced with the blurred pixel.')
            }
        }

        Shotcut.SliderSpinner {
            id: thresholdSlider

            minimumValue: 0
            maximumValue: 30
            decimals: 0
            onValueChanged: {
                filter.set("av.luma_threshold", value);
                filter.set("av.chroma_threshold", value);
            }
        }

        Shotcut.UndoButton {
            onClicked: thresholdSlider.value = thresholdDefault
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
