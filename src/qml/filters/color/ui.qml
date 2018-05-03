/*
 * Copyright (c) 2014-2018 Meltytech, LLC
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

import QtQuick 2.1
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    property var defaultParameters: ['lift_r', 'lift_g', 'lift_b', 'gamma_r', 'gamma_g', 'gamma_b', 'gain_r', 'gain_g', 'gain_b']
    property double gammaFactor: 2.0
    property double gainFactor: 4.0
    property bool blockUpdate: true
    width: 455
    height: 280
    
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set("lift_r", 0.0);
            filter.set("lift_g", 0.0);
            filter.set("lift_b", 0.0);
            filter.set("gamma_r", 1.0);
            filter.set("gamma_g", 1.0);
            filter.set("gamma_b", 1.0);
            filter.set("gain_r", 1.0);
            filter.set("gain_g", 1.0);
            filter.set("gain_b", 1.0);
            filter.savePreset(defaultParameters)
        }
        loadValues()
    }

    function loadValues() {
        var position = getPosition()
        blockUpdate = true
        liftRedSpinner.value = filter.getDouble("lift_r", position) * 100.0
        liftGreenSpinner.value = filter.getDouble("lift_g", position) * 100.0
        liftBlueSpinner.value = filter.getDouble("lift_b", position) * 100.0
        gammaRedSpinner.value = filter.getDouble("gamma_r", position) * 100.0 / gammaFactor
        gammaGreenSpinner.value = filter.getDouble("gamma_g", position) * 100.0 / gammaFactor
        gammaBlueSpinner.value = filter.getDouble("gamma_b", position) * 100.0 / gammaFactor
        gainRedSpinner.value = filter.getDouble("gain_r", position) * 100.0 / gainFactor
        gainGreenSpinner.value = filter.getDouble("gain_g", position) * 100.0 / gainFactor
        gainBlueSpinner.value = filter.getDouble("gain_b", position) * 100.0 / gainFactor
        blockUpdate = false
    }
    
    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    GridLayout {
        columns: 9
        anchors.fill: parent
        anchors.margins: 8

        // Row 1
        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            Layout.columnSpan: 8
            parameters: defaultParameters
            onBeforePresetLoaded: {
                filter.resetProperty('lift_r')
                filter.resetProperty('lift_g')
                filter.resetProperty('lift_b')
                filter.resetProperty('gamma_r')
                filter.resetProperty('gamma_g')
                filter.resetProperty('gamma_b')
                filter.resetProperty('gain_r')
                filter.resetProperty('gain_g')
                filter.resetProperty('gain_b')
            }
            onPresetSelected: {
                loadValues()
            }
        }

        // Row 2
        Label { text: qsTr('Shadows (Lift)') }
        UndoButton {
            Layout.alignment: Qt.AlignRight
            onClicked: {
                // Force a color change to make sure the color wheel is updated.
                liftRedSpinner.value = 1.0
                liftRedSpinner.value = 0.0
                liftGreenSpinner.value = 0.0
                liftBlueSpinner.value = 0.0
            }
        }
        KeyframesButton {
            id: liftKeyframesButton
            checked: filter.keyframeCount('lift_r') > 0
            Layout.alignment: Qt.AlignLeft
            onToggled: {
                if (checked) {
                    filter.set('lift_r', liftwheel.redF, getPosition())
                    filter.set('lift_g', liftwheel.greenF, getPosition())
                    filter.set('lift_b', liftwheel.blueF, getPosition())
                } else {
                    filter.resetProperty('lift_r')
                    filter.resetProperty('lift_g')
                    filter.resetProperty('lift_b')
                    filter.set('lift_r', liftwheel.redF)
                    filter.set('lift_g', liftwheel.greenF)
                    filter.set('lift_b', liftwheel.blueF)
                }
            }
        }
        Label { text: qsTr('Midtones (Gamma)') }
        UndoButton {
            Layout.alignment: Qt.AlignRight
            onClicked: {
                // Force a color change to make sure the color wheel is updated.
                gammaRedSpinner.value = 1.0 / gammaFactor
                gammaRedSpinner.value = 100.0 / gammaFactor
                gammaGreenSpinner.value = 100.0 / gammaFactor
                gammaBlueSpinner.value = 100.0 / gammaFactor
            }
        }
        KeyframesButton {
            id: gammaKeyframesButton
            checked: filter.keyframeCount('gamma_r') > 0
            Layout.alignment: Qt.AlignLeft
            onToggled: {
                if (checked) {
                    filter.set('gamma_r', gammawheel.redF * gammaFactor, getPosition())
                    filter.set('gamma_g', gammawheel.greenF * gammaFactor, getPosition())
                    filter.set('gamma_b', gammawheel.blueF * gammaFactor, getPosition())
                } else {
                    filter.resetProperty('gamma_r')
                    filter.resetProperty('gamma_g')
                    filter.resetProperty('gamma_b')
                    filter.set('gamma_r', gammawheel.redF * gammaFactor)
                    filter.set('gamma_g', gammawheel.greenF * gammaFactor)
                    filter.set('gamma_b', gammawheel.blueF * gammaFactor)
                }
            }
        }
        Label { text: qsTr('Highlights (Gain)') }
        UndoButton {
            Layout.alignment: Qt.AlignRight
            onClicked: {
                // Force a color change to make sure the color wheel is updated.
                gainRedSpinner.value = 1.0 / gainFactor
                gainRedSpinner.value = 100.0 / gainFactor
                gainGreenSpinner.value = 100.0 / gainFactor
                gainBlueSpinner.value = 100.0 / gainFactor
            }
        }
        KeyframesButton {
            id: gainKeyframesButton
            checked: filter.keyframeCount('gain_r') > 0
            Layout.alignment: Qt.AlignLeft
            onToggled: {
                if (checked) {
                    filter.set('gain_r', gainwheel.redF * gainFactor, getPosition())
                    filter.set('gain_g', gainwheel.greenF * gainFactor, getPosition())
                    filter.set('gain_b', gainwheel.blueF * gainFactor, getPosition())
                } else {
                    filter.resetProperty('gain_r')
                    filter.resetProperty('gain_g')
                    filter.resetProperty('gain_b')
                    filter.set('gain_r', gainwheel.redF * gainFactor)
                    filter.set('gain_g', gainwheel.greenF * gainFactor)
                    filter.set('gain_b', gainwheel.blueF * gainFactor)
                }
            }
        }

        // Row 3
        ColorWheelItem {
            id: liftwheel
            Layout.columnSpan: 3
            implicitWidth: parent.width / 3 - parent.columnSpacing
            implicitHeight: implicitWidth / 1.1
            Layout.alignment : Qt.AlignCenter | Qt.AlignTop
            Layout.minimumHeight: 75
            Layout.maximumHeight: 300
            onColorChanged: {
                if( liftRedSpinner.value != liftwheel.redF * 100.0 ) {
                    liftRedSpinner.value = liftwheel.redF * 100.0
                }
                if( liftGreenSpinner.value != liftwheel.greenF * 100.0 ) {
                    liftGreenSpinner.value = liftwheel.greenF * 100.0
                }
                if( liftBlueSpinner.value != liftwheel.blueF * 100.0 ) {
                    liftBlueSpinner.value = liftwheel.blueF * 100.0
                }
                if (!blockUpdate) {
                    if (!liftKeyframesButton.checked) {
                        filter.resetProperty('lift_r')
                        filter.resetProperty('lift_g')
                        filter.resetProperty('lift_b')
                        filter.set('lift_r', liftwheel.redF)
                        filter.set('lift_g', liftwheel.greenF)
                        filter.set('lift_b', liftwheel.blueF)
                    } else {
                        var position = getPosition()
                        filter.set('lift_r', liftwheel.redF, position)
                        filter.set('lift_g', liftwheel.greenF, position)
                        filter.set('lift_b', liftwheel.blueF, position)
                    }
                }
            }
        }
        ColorWheelItem {
            id: gammawheel
            Layout.columnSpan: 3
            implicitWidth: parent.width / 3 - parent.columnSpacing
            implicitHeight: implicitWidth / 1.1
            Layout.alignment : Qt.AlignCenter | Qt.AlignTop
            Layout.minimumHeight: 75
            Layout.maximumHeight: 300
            onColorChanged: {
                if( gammaRedSpinner.value != gammawheel.redF * 100.0 ) {
                    gammaRedSpinner.value = gammawheel.redF * 100.0
                }
                if( gammaGreenSpinner.value != gammawheel.greenF * 100.0 ) {
                    gammaGreenSpinner.value = gammawheel.greenF * 100.0
                }
                if( gammaBlueSpinner.value != gammawheel.blueF * 100.0 ) {
                    gammaBlueSpinner.value = gammawheel.blueF * 100.0
                }
                if (!blockUpdate) {
                    if (!gammaKeyframesButton.checked) {
                        filter.resetProperty('gamma_r')
                        filter.resetProperty('gamma_g')
                        filter.resetProperty('gamma_b')
                        filter.set('gamma_r', gammawheel.redF * gammaFactor)
                        filter.set('gamma_g', gammawheel.greenF * gammaFactor)
                        filter.set('gamma_b', gammawheel.blueF * gammaFactor)
                    } else {
                        var position = getPosition()
                        filter.set('gamma_r', gammawheel.redF * gammaFactor, position)
                        filter.set('gamma_g', gammawheel.greenF * gammaFactor, position)
                        filter.set('gamma_b', gammawheel.blueF * gammaFactor, position)
                    }
                }
            }
        }
        ColorWheelItem {
            id: gainwheel
            Layout.columnSpan: 3
            implicitWidth: parent.width / 3 - parent.columnSpacing
            implicitHeight: implicitWidth / 1.1
            Layout.alignment : Qt.AlignCenter | Qt.AlignTop
            Layout.minimumHeight: 75
            Layout.maximumHeight: 300
            onColorChanged: {
                if( gainRedSpinner.value != gainwheel.redF * 100.0 ) {
                    gainRedSpinner.value = gainwheel.redF * 100.0
                }
                if( gainGreenSpinner.value != gainwheel.greenF * 100.0 ) {
                    gainGreenSpinner.value = gainwheel.greenF * 100.0
                }
                if( gainBlueSpinner.value != gainwheel.blueF * 100.0 ) {
                    gainBlueSpinner.value = gainwheel.blueF * 100.0
                }
                if (!blockUpdate) {
                    if (!gainKeyframesButton.checked) {
                        filter.resetProperty('gain_r')
                        filter.resetProperty('gain_g')
                        filter.resetProperty('gain_b')
                        filter.set('gain_r', gainwheel.redF * gainFactor)
                        filter.set('gain_g', gainwheel.greenF * gainFactor)
                        filter.set('gain_b', gainwheel.blueF * gainFactor)
                    } else {
                        var position = getPosition()
                        filter.set('gain_r', gainwheel.redF * gainFactor, position)
                        filter.set('gain_g', gainwheel.greenF * gainFactor, position)
                        filter.set('gain_b', gainwheel.blueF * gainFactor, position)
                    }
                }
            }
        }

        // Row 4
        RowLayout {
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label { text: 'R' }
            SpinBox {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                id: liftRedSpinner
                minimumValue: 0
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( liftwheel.redF != value / 100.0 ) {
                        liftwheel.redF = value / 100.0
                    }
                }
            }
        }
        RowLayout {
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label { text: 'R' }
            SpinBox {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                id: gammaRedSpinner
                minimumValue: 0
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gammawheel.redF != value / 100.0 ) {
                        gammawheel.redF = value / 100.0
                    }
                }
            }
        }
        RowLayout {
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label { text: 'R' }
            SpinBox {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                id: gainRedSpinner
                minimumValue: 0
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gainwheel.redF != value / 100.0 ) {
                        gainwheel.redF = value / 100.0
                    }
                }
            }
        }

        // Row 5
        RowLayout {
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label { text: 'G' }
            SpinBox {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                id: liftGreenSpinner
                minimumValue: 0
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( liftwheel.greenF != value / 100.0 ) {
                        liftwheel.greenF = value / 100.0
                    }
                }
            }
        }
        RowLayout {
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label { text: 'G' }
            SpinBox {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                id: gammaGreenSpinner
                minimumValue: 0
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gammawheel.greenF != value / 100.0 ) {
                        gammawheel.greenF = value / 100.0
                    }
                }
            }
        }
        RowLayout {
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label { text: 'G' }
            SpinBox {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                id: gainGreenSpinner
                minimumValue: 0
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gainwheel.greenF != value / 100.0 ) {
                        gainwheel.greenF = value / 100.0
                    }
                }
            }
        }

        // Row 6
        RowLayout {
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label { text: 'B' }
            SpinBox {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                id: liftBlueSpinner
                minimumValue: 0
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( liftwheel.blueF != value / 100.0 ) {
                        liftwheel.blueF = value / 100.0
                    }
                }
            }
        }
        RowLayout {
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label { text: 'B' }
            SpinBox {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                id: gammaBlueSpinner
                minimumValue: 0
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gammawheel.blueF != value / 100.0 ) {
                        gammawheel.blueF = value / 100.0
                    }
                }
            }
        }
        RowLayout {
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Label { text: 'B' }
            SpinBox {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                id: gainBlueSpinner
                minimumValue: 0
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gainwheel.blueF != value / 100.0 ) {
                        gainwheel.blueF = value / 100.0
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: producer
        onPositionChanged: loadValues()
    }
}
