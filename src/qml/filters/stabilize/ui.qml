/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Rectangle {
    width: 400
    height: 200
    color: 'transparent'

    Connections {
        target: filter
        onStabilizeFinished: {
            filter.set("refresh", 1);
            if (isSuccess) status.text = qsTr('Analysis complete.')
            button.enabled = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        Label { text: qsTr('<b>Analyze Options</b>') }

        RowLayout {
            spacing: 8
            Label { text: qsTr('Shakiness') }
            Slider {
                id: shakinessSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('shakiness')
                minimumValue: 1
                maximumValue: 10
                tickmarksEnabled: true
                stepSize: 1
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        shakinessSpinner.value = value
                        filter.set('shakiness', value)
                    }
                }
            }
            SpinBox {
                id: shakinessSpinner
                Layout.minimumWidth: 60
                value: filter.get('shakiness')
                minimumValue: 1
                maximumValue: 10
                decimals: 0
                onValueChanged: shakinessSlider.value = value
            }
            UndoButton {
                onClicked: shakinessSlider.value = 4
            }
        }

        RowLayout {
            spacing: 8
            Label { text: qsTr('Accuracy') }
            Slider {
                id: accuracySlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('accuracy')
                minimumValue: 1
                maximumValue: 15
                tickmarksEnabled: true
                stepSize: 1
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        accuracySpinner.value = value
                        filter.set('accuracy', value)
                    }
                }
            }
            SpinBox {
                id: accuracySpinner
                Layout.minimumWidth: 60
                value: filter.get('accuracy')
                minimumValue: 1
                maximumValue: 15
                decimals: 0
                onValueChanged: accuracySlider.value = value
            }
            UndoButton {
                onClicked: accuracySlider.value = 4
            }
        }

        RowLayout {
            spacing: 8
            Button {
                id: button
                text: qsTr('Analyze')
                onClicked: {
                    button.enabled = false
                    status.text = ''
                    filter.stabilizeVideo();
                }
            }
            Label {
                id: status
                text: qsTr('Click Analyze to use this filter.')
                Component.onCompleted: {
                    if (filter.get("vectors").length > 0)
                        text = qsTr('Analysis complete.')
                }
            }
        }

        Label { text: qsTr('<b>Filter Options</b>') }

        RowLayout {
            spacing: 8
            Label { text: qsTr('Zoom') }
            Slider {
                id: zoomSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('zoom')
                minimumValue: -50
                maximumValue: 50
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        zoomSpinner.value = value
                        filter.set('zoom', value)
                        filter.set("refresh", 1);
                    }
                }
            }
            SpinBox {
                id: zoomSpinner
                Layout.minimumWidth: 60
                value: filter.get('zoom')
                minimumValue: -50
                maximumValue: 50
                decimals: 1
                suffix: ' %'
                onValueChanged: zoomSlider.value = value
            }
            UndoButton {
                onClicked: zoomSlider.value = 0
            }
        }

        RowLayout {
            spacing: 8
            Label { text: qsTr('Sharpening') }
            Slider {
                id: sharpenSlider
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                value: filter.get('sharpen')
                minimumValue: 0
                maximumValue: 10
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        sharpenSpinner.value = value
                        filter.set('sharpen', value)
                        filter.set("refresh", 1);
                    }
                }
            }
            SpinBox {
                id: sharpenSpinner
                Layout.minimumWidth: 60
                value: filter.get('sharpen')
                minimumValue: 0
                maximumValue: 10
                decimals: 2
                onValueChanged: sharpenSlider.value = value
            }
            UndoButton {
                onClicked: sharpenSlider.value = 0.8
            }
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}
