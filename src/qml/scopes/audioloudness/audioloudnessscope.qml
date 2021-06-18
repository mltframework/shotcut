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
import QtQml 2.12
import Shotcut.Controls 1.0 as Shotcut

Rectangle {
    id: root

    property bool enableIntegrated: false
    property bool enableShortterm: false
    property bool enableMomentary: false
    property bool enableRange: false
    property bool enablePeak: false
    property bool enableTruePeak: false
    property double integrated: -100
    property double shortterm: -100
    property double momentary: -100
    property double range: -1
    property double peak: -100
    property double maxPeak: -100
    property double truePeak: -100
    property double maxTruePeak: -100
    property int orientation: Qt.Vertical

    color: activePalette.window
    state: orientation == Qt.Horizontal ? "landscape" : "portrait"
    SystemPalette { id: activePalette }

    TextMetrics {
        id: textMetrics
        font: momentaryLabel.font
        text: "-00.0"
    }

    GridLayout {
        id: grid
        anchors.fill: parent

        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: momentaryTitle
            text: 'M'
            color: activePalette.text
            Shotcut.HoverTip {text: qsTr('Momentary Loudness.')}
            visible: enableMomentary
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: momentaryLabel
            text: momentary < -99 ? '--' : momentary.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
            visible: enableMomentary
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: momentaryUnits
            text: 'LUFS'
            color: activePalette.text
            visible: enableMomentary
        }
        Shotcut.Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: momentaryGauge
            from: -50
            value: momentary
            to: 0
            decimals: 0
            visible: enableMomentary
        }

        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            text: 'S'
            color: activePalette.text
            Shotcut.HoverTip {text: qsTr('Short-term Loudness.')}
            visible: enableShortterm
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: shorttermLabel
            text: shortterm < -99 ? '--' : shortterm.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
            visible: enableShortterm
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: shorttermUnits
            text: 'LUFS'
            color: activePalette.text
            visible: enableShortterm
        }
        Shotcut.Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: shorttermGauge
            from: -50
            value: shortterm
            to: 0
            decimals: 0
            visible: enableShortterm
        }

        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            text: 'I'
            color: activePalette.text
            Shotcut.HoverTip {text: qsTr('Integrated Loudness.')}
            visible: enableIntegrated
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: integratedLabel
            text: integrated < -99 ? '--' : integrated.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
            visible: enableIntegrated
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: integratedUnits
            text: 'LUFS'
            color: activePalette.text
            visible: enableIntegrated
        }
        Shotcut.Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: integratedGauge
            from: -50
            value: integrated
            to: 0
            decimals: 0
            visible: enableIntegrated
        }
        
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            text: 'LRA'
            color: activePalette.text
            Shotcut.HoverTip {text: qsTr('Loudness Range.')}
            visible: enableRange
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: rangeLabel
            text: range < 0 ? '--' : range.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
            visible: enableRange
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: rangeUnits
            text: 'LU'
            color: activePalette.text
            visible: enableRange
        }
        Shotcut.Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: rangeGauge
            from: 0
            value: range
            to: 30
            decimals: 0
            visible: enableRange
        }
        
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            text: 'P'
            color: activePalette.text
            Shotcut.HoverTip {text: qsTr('Peak.')}
            visible: enablePeak
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: peakLabel
            text: peak < -99 ? '--' : peak.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
            visible: enablePeak
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: peakUnits
            text: 'dBFS'
            color: activePalette.text
            visible: enablePeak
        }
        Shotcut.Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: peakGauge
            from: -50
            value: peak
            to: 3
            decimals: 0
            visible: enablePeak
        }

        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            text: 'TP'
            color: activePalette.text
            Shotcut.HoverTip {text: qsTr('True Peak.')}
            visible: enableTruePeak
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.minimumWidth: textMetrics.width + 5
            Layout.preferredWidth: Layout.minimumWidth
            id: truePeakLabel
            text: truePeak < -99 ? '--' : truePeak.toFixed(1)
            color: activePalette.text
            horizontalAlignment: Qt.AlignHCenter
            visible: enableTruePeak
        }
        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            id: truePeakUnits
            text: 'dBTP'
            color: activePalette.text
            visible: enableTruePeak
        }
        Shotcut.Gauge {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            id: truePeakGauge
            from: -50
            value: truePeak
            to: 3
            decimals: 0
            visible: enableTruePeak
        }

        Item {
            id: filler
        }
    }

    states: [
        State {
            name: "landscape"
            PropertyChanges { target: grid; flow: GridLayout.LeftToRight; columns: 4 }
            PropertyChanges { target: momentaryGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false }
            PropertyChanges { target: shorttermGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false }
            PropertyChanges { target: integratedGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false }
            PropertyChanges { target: rangeGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false }
            PropertyChanges { target: peakGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false }
            PropertyChanges { target: truePeakGauge; orientation: Qt.Horizontal; Layout.fillWidth: true; Layout.fillHeight: false }
            PropertyChanges { target: filler; Layout.fillHeight: true; Layout.fillWidth: false }
        },
        State {
            name: "portrait"
            PropertyChanges { target: grid; flow: GridLayout.TopToBottom; rows: 4 }
            PropertyChanges { target: momentaryGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true }
            PropertyChanges { target: shorttermGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true }
            PropertyChanges { target: integratedGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true }
            PropertyChanges { target: rangeGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true }
            PropertyChanges { target: peakGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true }
            PropertyChanges { target: truePeakGauge; orientation: Qt.Vertical; Layout.fillWidth: false; Layout.fillHeight: true }
            PropertyChanges { target: filler; Layout.fillHeight: false; Layout.fillWidth: true }
        }
    ]
}