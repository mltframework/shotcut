/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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

import QtQuick 2.1
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0 as Shotcut

RowLayout {
    property real value
    property alias minimumValue: slider.from
    property alias maximumValue: slider.to
    property real ratio: 1
    property alias label: label.text
    property int decimals: 0
    property alias stepSize: spinner.stepSize
    property alias spinnerWidth: spinner.width
    property alias suffix: spinner.suffix
    property alias prefix: spinner.prefix

    spacing: -3
    onValueChanged: spinner.value = value / ratio

    SystemPalette {
        id: activePalette
    }

    Slider {
        id: slider

        property bool isReady: false

        stepSize: spinner.stepSize
        Layout.fillWidth: true
        activeFocusOnTab: false
        wheelEnabled: true
        Component.onCompleted: {
            isReady = true;
            value = parent.value;
        }
        onValueChanged: {
            if (isReady) {
                spinner.value = value / ratio;
                parent.value = value;
            }
        }

        background: Rectangle {
            radius: 3
            color: activePalette.alternateBase
            border.color: 'gray'
            border.width: 1
            implicitHeight: spinner.height

            // Hide the right border.
            Rectangle {
                visible: !label.visible
                width: 3
                color: parent.color

                anchors {
                    top: parent.top
                    right: parent.right
                    bottom: parent.bottom
                    topMargin: 1
                    bottomMargin: 1
                }

            }

            // Indicate percentage full.
            Rectangle {
                radius: parent.radius
                width: parent.width * (value - minimumValue) / (maximumValue - minimumValue) - parent.border.width - (label.visible ? parent.border.width : 3)
                color: enabled ? activePalette.highlight : activePalette.midlight

                anchors {
                    top: parent.top
                    left: parent.left
                    bottom: parent.bottom
                    margins: 1
                }

            }

        }

        handle: Rectangle {
        }

    }

    // Optional label between slider and spinner
    Rectangle {
        width: 4 - parent.spacing * 2
        visible: label.visible
    }

    Label {
        id: label

        visible: text.length
    }

    Rectangle {
        width: 4 - parent.spacing * 2
        visible: label.visible
    }

    Shotcut.DoubleSpinBox {
        id: spinner

        verticalPadding: 2
        Layout.minimumWidth: background.implicitWidth
        from: slider.from / ratio
        to: slider.to / ratio
        decimals: parent.decimals
        stepSize: 1 / Math.pow(10, decimals)
        onValueChanged: slider.value = value * ratio

        background: Rectangle {
            color: activePalette.base
            border.color: 'gray'
            border.width: 1
            implicitHeight: 18
            implicitWidth: 115
            radius: 3

            // Hide the left border.
            Rectangle {
                visible: !label.visible
                width: 3
                color: parent.color

                anchors {
                    top: parent.top
                    left: parent.left
                    bottom: parent.bottom
                    topMargin: 1
                    bottomMargin: 1
                }

            }

        }

        up.indicator: Rectangle {
        }

        down.indicator: Rectangle {
        }

    }

}
