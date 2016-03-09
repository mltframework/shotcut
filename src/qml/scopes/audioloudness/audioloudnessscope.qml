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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.0
import QtQuick.Extras 1.4
import QtQml 2.2
import Shotcut.Controls 1.0

Rectangle {
    id: root
    
    property double _integrated: -100
    property double _shortterm: -100
    property double _momentary: -100
    property double _range: -1
    
    function setValues(program, shortterm, momentary, range) {
        _integrated = Math.round(program * 10) / 10
        _shortterm = Math.round(shortterm * 10) / 10
        _momentary = Math.round(momentary * 10) / 10
        _range = Math.round(range * 10) / 10
    }

    color: activePalette.window
    
    onWidthChanged: _setLayout()
    onHeightChanged: _setLayout()
    
    function _setLayout() {
        if (height > width) {
            root.state = "portrait"
        } else {
            root.state = "landscape"
        }
    }
    
    SystemPalette { id: activePalette }
    
    TextMetrics {
        id: textMetrics
        font: momentaryLabel.font
        text: "-00.0"
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

    GridLayout {
        id: grid
        anchors.fill: parent

        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: momentaryTitle
            text: "M"
            color: activePalette.text
            ToolTip {text: qsTr('Momentary Loudness.')}
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: momentaryLabel
            text: _momentary < -99 ? '--' : _momentary.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: momentaryUnits
            text: 'LUFS'
            color: activePalette.text
        }
        Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: momentaryGauge
            minimumValue: -50
            value: _momentary
            maximumValue: 0
            style: gaugeStyle
        }

        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            text: "S"
            color: activePalette.text
            ToolTip {text: qsTr('Short-term Loudness.')}
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: shorttermLabel
            text: _shortterm < -99 ? '--' : _shortterm.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: shorttermUnits
            text: 'LUFS'
            color: activePalette.text
        }
        Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: shorttermGauge
            minimumValue: -50
            value: _shortterm
            maximumValue: 0
            style: gaugeStyle
        }

        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            text: "I"
            color: activePalette.text
            ToolTip {text: qsTr('Integrated Loudness.')}
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: integratedLabel
            text: _integrated < -99 ? '--' : _integrated.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: integratedUnits
            text: 'LUFS'
            color: activePalette.text
        }
        Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: integratedGauge
            minimumValue: -50
            value: _integrated
            maximumValue: 0
            style: gaugeStyle
        }
        
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            text: "LRA"
            color: activePalette.text
            ToolTip {text: qsTr('Loudness Range.')}
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: rangeLabel
            text: _range < 0 ? '--' : _range.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: rangeUnits
            text: 'LU'
            color: activePalette.text
        }
        Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: rangeGauge
            minimumValue: 0
            value: _range
            maximumValue: 30
            style: gaugeStyle
        }
        
        Item {
            id: filler
        }
    }
        
    states: [
        State {
            name: "landscape"
            PropertyChanges { target: grid; flow: GridLayout.LeftToRight; columns: 4 }
            PropertyChanges { target: momentaryTitle; Layout.leftMargin: 0 }
            PropertyChanges { target: momentaryLabel; Layout.leftMargin: 0 }
            PropertyChanges { target: momentaryUnits; Layout.leftMargin: 0 }
            PropertyChanges { target: momentaryGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false; Layout.leftMargin: 4; Layout.rightMargin: 4 }
            PropertyChanges { target: shorttermGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false; Layout.leftMargin: 4; Layout.rightMargin: 4 }
            PropertyChanges { target: integratedGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false; Layout.leftMargin: 4; Layout.rightMargin: 4 }
            PropertyChanges { target: rangeGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false; Layout.leftMargin: 0; Layout.rightMargin: 0 }
            PropertyChanges { target: filler; Layout.fillHeight: true; Layout.fillWidth: false; Layout.maximumWidth: 0; Layout.maximumHeight: -1 }
        },
        State {
            name: "portrait"
            PropertyChanges { target: grid; flow: GridLayout.TopToBottom; rows: 4 }
            PropertyChanges { target: momentaryTitle; Layout.leftMargin: 10 }
            PropertyChanges { target: momentaryLabel; Layout.leftMargin: 10 }
            PropertyChanges { target: momentaryUnits; Layout.leftMargin: 10 }
            PropertyChanges { target: momentaryGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true; Layout.leftMargin: 10; Layout.rightMargin: 14 }
            PropertyChanges { target: shorttermGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true; Layout.leftMargin: 0; Layout.rightMargin: 14 }
            PropertyChanges { target: integratedGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true; Layout.leftMargin: 0; Layout.rightMargin: 14 }
            PropertyChanges { target: rangeGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true; Layout.leftMargin: -10; Layout.rightMargin: 14 }
            PropertyChanges { target: filler; Layout.fillHeight: false; Layout.fillWidth: true; Layout.maximumWidth: -1; Layout.maximumHeight: 0 }
        }
    ]
}
