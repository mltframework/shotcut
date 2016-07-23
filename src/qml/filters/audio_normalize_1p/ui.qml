/*
 * Copyright (c) 2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.0
import QtQuick.Extras 1.4
import QtQml 2.2
import Shotcut.Controls 1.0

Item {
    width: 480
    height: 325
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('target_loudness', -23.0)
            filter.set('window', 10.0)
            filter.set('max_gain', 15.0)
            filter.set('min_gain', -15.0)
            filter.set('max_rate', 3.0)
            filter.savePreset(preset.parameters)
        }
        setControls()
        timer.start()
    }

    function setControls() {
        programSlider.value = filter.getDouble('target_loudness')
        windowSlider.value = filter.getDouble('window')
        maxgainSlider.value = filter.getDouble('max_gain')
        mingainSlider.value = filter.getDouble('min_gain')
        maxrateSlider.value = filter.getDouble('max_rate')
    }
    
    SystemPalette { id: activePalette }

    Timer {
        id: timer
        interval: 200
        running: false
        repeat: true
        property int _prevResetVal: 0
        property int _resetCountDown: 0
        onTriggered: {
            loudnessGauge.value = filter.getDouble('in_loudness')
            gainGauge.value = filter.getDouble('out_gain')
            if( filter.get('reset_count') != _prevResetVal ) {
                _prevResetVal = filter.get('reset_count')
                _resetCountDown = 1000 / timer.interval
                resetIndicator.active = true
            } else if( _resetCountDown != 0 ) {
                _resetCountDown--
            } else {
                resetIndicator.active = false
            }
            
        }
    }

    GridLayout {
        id: grid
        anchors.fill: parent
        anchors.margins: 8
        columns: 3
        columnSpacing: 8
        rowSpacing: 8
        
        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: ['target_loudness', 'window', 'max_gain', 'min_gain', 'max_rate']
            Layout.columnSpan: 2
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Target Loudness')
            Layout.alignment: Qt.AlignRight
            ToolTip {text: qsTr('The target loudness of the output in LUFS.')}
        }
        SliderSpinner {
            id: programSlider
            minimumValue: -50.0
            maximumValue: -10.0
            decimals: 1
            suffix: ' LUFS'
            spinnerWidth: 100
            value: filter.getDouble('target_loudness')
            onValueChanged: filter.set('target_loudness', value)
        }
        UndoButton {
            onClicked: programSlider.value = -23.0
        }

        Label {
            text: qsTr('Analysis Window')
            Layout.alignment: Qt.AlignRight
            ToolTip {text: qsTr('The amount of history to use to calculate the input loudness.')}
        }
        SliderSpinner {
            id: windowSlider
            minimumValue: 2
            maximumValue: 600
            decimals: 0
            suffix: ' s'
            spinnerWidth: 100
            value: filter.getDouble('window')
            onValueChanged: filter.set('window', value)
        }
        UndoButton {
            onClicked: windowSlider.value = 20.0
        }

        Label {
            text: qsTr('Maximum Gain')
            Layout.alignment: Qt.AlignRight
            ToolTip {text: qsTr('The maximum that the gain can be increased.')}
        }
        SliderSpinner {
            id: maxgainSlider
            minimumValue: 0.0
            maximumValue: 30.0
            decimals: 1
            suffix: ' dB'
            spinnerWidth: 100
            value: filter.getDouble('max_gain')
            onValueChanged: filter.set('max_gain', value)
        }
        UndoButton {
            onClicked: maxgainSlider.value = 15
        }

        Label {
            text: qsTr('Minimum Gain')
            Layout.alignment: Qt.AlignRight
            ToolTip {text: qsTr('The maximum that the gain can be decreased.')}
        }
        SliderSpinner {
            id: mingainSlider
            minimumValue: -30.0
            maximumValue: 0.0
            decimals: 1
            suffix: ' dB'
            spinnerWidth: 100
            value: filter.getDouble('min_gain')
            onValueChanged: filter.set('min_gain', value)
        }
        UndoButton {
            onClicked: mingainSlider.value = -15.0
        }

        Label {
            text: qsTr('Maximum Rate')
            Layout.alignment: Qt.AlignRight
            ToolTip {text: qsTr('The maximum rate that the gain can be changed.')}
        }
        SliderSpinner {
            id: maxrateSlider
            minimumValue: 0.5
            maximumValue: 9.0
            decimals: 1
            stepSize: 0.1
            suffix: ' dB/s'
            spinnerWidth: 100
            value: filter.getDouble('max_rate')
            onValueChanged: filter.set('max_rate', value)
        }
        UndoButton {
            onClicked: maxrateSlider.value = 3.0
        }
        
        Rectangle {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.minimumHeight: 12
            color: 'transparent'
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 2
                radius: 2
                color: activePalette.text
            }
        }
        
        Component {
            id: gaugeStyle
            GaugeStyle {
                valueBar: Rectangle {
                    implicitWidth: 16
                    color: 'transparent'
                    Rectangle {
                        anchors.right: parent.right
                        width: parent.width
                        height: 5
                        radius: 3
                        color: activePalette.highlight
                    }
                }
                tickmark: Item {
                    implicitWidth: 9
                    implicitHeight: 1
                    Rectangle {
                        color: activePalette.windowText
                        anchors.fill: parent
                        anchors.leftMargin: 2
                        anchors.rightMargin: 2
                    }
                }
                minorTickmark: Item {
                    implicitWidth: 6
                    implicitHeight: 1
                    Rectangle {
                        color: activePalette.windowText
                        anchors.fill: parent
                        anchors.leftMargin: 2
                        anchors.rightMargin: 2
                    }
                }
                tickmarkLabel: Text {
                    font.pixelSize: 12
                    text: Math.round(parseFloat(styleData.value))
                    color: activePalette.windowText
                    antialiasing: true
                 }
            }
        }
        
        Label {
            text: qsTr('Input Loudness')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            ToolTip {text: qsTr('Status indicator showing the loudness measured on the input.')}
        }
        Gauge {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.rightMargin: 4
            id: loudnessGauge
            minimumValue: -50
            value: -50
            maximumValue: 0
            orientation: Qt.Horizontal
            style: gaugeStyle
        }
        
        Label {
            text: qsTr('Output Gain')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            ToolTip {text: qsTr('Status indicator showing the gain being applied.')}
        }
        Gauge {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            id: gainGauge
            minimumValue: Math.floor(mingainSlider.value)
            value: 0
            maximumValue: Math.ceil(maxgainSlider.value)
            tickmarkStepSize: (maximumValue - minimumValue) / 6
            orientation: Qt.Horizontal
            style: gaugeStyle
        }

        Label {
            text: qsTr('Reset')
            Layout.alignment: Qt.AlignRight
            ToolTip {text: qsTr('Status indicator showing when the loudness measurement is reset.')}
        }
        StatusIndicator {
            Layout.columnSpan: 2
            Layout.maximumHeight: 20
            id: resetIndicator
            color: activePalette.highlight
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}
