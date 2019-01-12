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
        // Force a color change to make sure the color wheel is updated.
        liftRedSpinner.value = 1.0
        gammaRedSpinner.value = 1.0
        gainRedSpinner.value = 1.0
        liftRedSpinner.value = filter.getDouble("lift_r", position) * 100.0
        liftGreenSpinner.value = filter.getDouble("lift_g", position) * 100.0
        liftBlueSpinner.value = filter.getDouble("lift_b", position) * 100.0
        gammaRedSpinner.value = wheelToSpinner(scaleValueToWheel(filter.getDouble("gamma_r", position), gammaFactor))
        gammaGreenSpinner.value = wheelToSpinner(scaleValueToWheel(filter.getDouble("gamma_g", position), gammaFactor))
        gammaBlueSpinner.value = wheelToSpinner(scaleValueToWheel(filter.getDouble("gamma_b", position), gammaFactor))
        gainRedSpinner.value = wheelToSpinner(scaleValueToWheel(filter.getDouble("gain_r", position), gainFactor))
        gainGreenSpinner.value = wheelToSpinner(scaleValueToWheel(filter.getDouble("gain_g", position), gainFactor))
        gainBlueSpinner.value = wheelToSpinner(scaleValueToWheel(filter.getDouble("gain_b", position), gainFactor))
        blockUpdate = false
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function scaleWheelToValue(w, f) {
        // The color wheel value (w) range is [0.0 - 1.0]. For the gamma and gain
        // gain parameters, we want to scale that to [0.0 - f] where (f) is the
        // maximum scaling factor. The middle of the wheel (0.5) needs to map to
        // a filter value of 1.0 (no change). Since this could be a non-linear
        // scaling, we use the quadratic equation:
        //    f(x) = ax**2 + bx + c
        // With the following point mappings:
        //    f(0) = 0
        //    f(0.5) = 1
        //    f(1) = f (scaling factor)
        // This reduces to:
        //    a = 2f - 4
        //    b = f - a
        //    c = 0
        var a = (2*f) - 4
        var b = f - a
        return a * (w*w) + b*w
    }

    function scaleValueToWheel(v, f) {
       // Do the opposite of scaleWheelToValue
       var a = (2*f) - 4
       var b = f - a
       if (a == 0) {
           // For a = 0, the function is linear
           return v / f
       } else {
           // The solution to the quadratic formula reduces to:
           return ((b*-1.0) + Math.sqrt((b*b) + (4*a*v))) / (2*a)
       }
   }

    function wheelToSpinner(w) {
        // Convert a wheel value [0.0 - 1.0] to a spinner value [-100.0 - 100.0]
        return (w * 2.0 - 1.0) * 100.0
    }

    function spinnerToWheel(s) {
        // Convert a spinner value [-100.0 - 100.0] to wheel value [0.0 - 1.0]
        return (s / 100.0 + 1.0) / 2.0
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
                    filter.set('lift_r', liftwheel.redF * 2.0 - 1.0, getPosition())
                    filter.set('lift_g', liftwheel.greenF * 2.0 - 1.0, getPosition())
                    filter.set('lift_b', liftwheel.blueF * 2.0 - 1.0, getPosition())
                } else {
                    filter.resetProperty('lift_r')
                    filter.resetProperty('lift_g')
                    filter.resetProperty('lift_b')
                    filter.set('lift_r', liftwheel.redF * 2.0 - 1.0)
                    filter.set('lift_g', liftwheel.greenF * 2.0 - 1.0)
                    filter.set('lift_b', liftwheel.blueF * 2.0 - 1.0)
                }
            }
        }
        Label { text: qsTr('Midtones (Gamma)') }
        UndoButton {
            Layout.alignment: Qt.AlignRight
            onClicked: {
                // Force a color change to make sure the color wheel is updated.
                gammaRedSpinner.value = 100.0
                gammaRedSpinner.value = 0.0
                gammaGreenSpinner.value = 0.0
                gammaBlueSpinner.value = 0.0
            }
        }
        KeyframesButton {
            id: gammaKeyframesButton
            checked: filter.keyframeCount('gamma_r') > 0
            Layout.alignment: Qt.AlignLeft
            onToggled: {
                if (checked) {
                    filter.set('gamma_r', scaleWheelToValue(gammawheel.redF, gammaFactor), getPosition())
                    filter.set('gamma_g', scaleWheelToValue(gammawheel.greenF, gammaFactor), getPosition())
                    filter.set('gamma_b', scaleWheelToValue(gammawheel.blueF, gammaFactor), getPosition())
                } else {
                    filter.resetProperty('gamma_r')
                    filter.resetProperty('gamma_g')
                    filter.resetProperty('gamma_b')
                    filter.set('gamma_r', scaleWheelToValue(gammawheel.redF, gammaFactor))
                    filter.set('gamma_g', scaleWheelToValue(gammawheel.greenF, gammaFactor))
                    filter.set('gamma_b', scaleWheelToValue(gammawheel.blueF, gammaFactor))
                }
            }
        }
        Label { text: qsTr('Highlights (Gain)') }
        UndoButton {
            Layout.alignment: Qt.AlignRight
            onClicked: {
                // Force a color change to make sure the color wheel is updated.
                gainRedSpinner.value = 100.0
                gainRedSpinner.value = 0.0
                gainGreenSpinner.value = 0.0
                gainBlueSpinner.value = 0.0
            }
        }
        KeyframesButton {
            id: gainKeyframesButton
            checked: filter.keyframeCount('gain_r') > 0
            Layout.alignment: Qt.AlignLeft
            onToggled: {
                if (checked) {
                    filter.set('gain_r', scaleWheelToValue(gainwheel.redF, gainFactor), getPosition())
                    filter.set('gain_g', scaleWheelToValue(gainwheel.greenF, gainFactor), getPosition())
                    filter.set('gain_b', scaleWheelToValue(gainwheel.blueF, gainFactor), getPosition())
                } else {
                    filter.resetProperty('gain_r')
                    filter.resetProperty('gain_g')
                    filter.resetProperty('gain_b')
                    filter.set('gain_r', scaleWheelToValue(gainwheel.redF, gainFactor))
                    filter.set('gain_g', scaleWheelToValue(gainwheel.greenF, gainFactor))
                    filter.set('gain_b', scaleWheelToValue(gainwheel.blueF, gainFactor))
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
            step: 0.0005
            onColorChanged: {
                if( liftRedSpinner.value != wheelToSpinner(liftwheel.redF) ) {
                    liftRedSpinner.value = wheelToSpinner(liftwheel.redF)
                }
                if( liftGreenSpinner.value != wheelToSpinner(liftwheel.greenF) ) {
                    liftGreenSpinner.value = wheelToSpinner(liftwheel.greenF)
                }
                if( liftBlueSpinner.value != wheelToSpinner(liftwheel.blueF) ) {
                    liftBlueSpinner.value = wheelToSpinner(liftwheel.blueF)
                }
                if (!blockUpdate) {
                    if (!liftKeyframesButton.checked) {
                        filter.resetProperty('lift_r')
                        filter.resetProperty('lift_g')
                        filter.resetProperty('lift_b')
                        filter.set('lift_r', liftwheel.redF * 2.0 - 1.0)
                        filter.set('lift_g', liftwheel.greenF * 2.0 - 1.0)
                        filter.set('lift_b', liftwheel.blueF * 2.0 - 1.0)
                    } else {
                        var position = getPosition()
                        filter.set('lift_r', liftwheel.redF * 2.0 - 1.0, position)
                        filter.set('lift_g', liftwheel.greenF * 2.0 - 1.0, position)
                        filter.set('lift_b', liftwheel.blueF * 2.0 - 1.0, position)
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
            step: 0.0005
            onColorChanged: {
                if( gammaRedSpinner.value != wheelToSpinner(gammawheel.redF) ) {
                    gammaRedSpinner.value = wheelToSpinner(gammawheel.redF)
                }
                if( gammaGreenSpinner.value != wheelToSpinner(gammawheel.greenF) ) {
                    gammaGreenSpinner.value = wheelToSpinner(gammawheel.greenF)
                }
                if( gammaBlueSpinner.value != wheelToSpinner(gammawheel.blueF) ) {
                    gammaBlueSpinner.value = wheelToSpinner(gammawheel.blueF)
                }
                if (!blockUpdate) {
                    if (!gammaKeyframesButton.checked) {
                        filter.resetProperty('gamma_r')
                        filter.resetProperty('gamma_g')
                        filter.resetProperty('gamma_b')
                        filter.set('gamma_r', scaleWheelToValue(gammawheel.redF, gammaFactor))
                        filter.set('gamma_g', scaleWheelToValue(gammawheel.greenF, gammaFactor))
                        filter.set('gamma_b', scaleWheelToValue(gammawheel.blueF, gammaFactor))
                    } else {
                        var position = getPosition()
                        filter.set('gamma_r', scaleWheelToValue(gammawheel.redF, gammaFactor), position)
                        filter.set('gamma_g', scaleWheelToValue(gammawheel.greenF, gammaFactor), position)
                        filter.set('gamma_b', scaleWheelToValue(gammawheel.blueF, gammaFactor), position)
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
            step: 0.0005
            onColorChanged: {
                if( gainRedSpinner.value != wheelToSpinner(gainwheel.redF) ) {
                    gainRedSpinner.value = wheelToSpinner(gainwheel.redF)
                }
                if( gainGreenSpinner.value != wheelToSpinner(gainwheel.greenF) ) {
                    gainGreenSpinner.value = wheelToSpinner(gainwheel.greenF)
                }
                if( gainBlueSpinner.value != wheelToSpinner(gainwheel.blueF) ) {
                    gainBlueSpinner.value = wheelToSpinner(gainwheel.blueF)
                }
                if (!blockUpdate) {
                    if (!gainKeyframesButton.checked) {
                        filter.resetProperty('gain_r')
                        filter.resetProperty('gain_g')
                        filter.resetProperty('gain_b')
                        filter.set('gain_r', scaleWheelToValue(gainwheel.redF, gainFactor))
                        filter.set('gain_g', scaleWheelToValue(gainwheel.greenF, gainFactor))
                        filter.set('gain_b', scaleWheelToValue(gainwheel.blueF, gainFactor))
                    } else {
                        var position = getPosition()
                        filter.set('gain_r', scaleWheelToValue(gainwheel.redF, gainFactor), position)
                        filter.set('gain_g', scaleWheelToValue(gainwheel.greenF, gainFactor), position)
                        filter.set('gain_b', scaleWheelToValue(gainwheel.blueF, gainFactor), position)
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
                minimumValue: -100
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( liftwheel.redF != spinnerToWheel(value) ) {
                        liftwheel.redF = spinnerToWheel(value)
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
                minimumValue: -100
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gammawheel.redF != spinnerToWheel(value) ) {
                        gammawheel.redF = spinnerToWheel(value)
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
                minimumValue: -100
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gainwheel.redF != spinnerToWheel(value) ) {
                        gainwheel.redF = spinnerToWheel(value)
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
                minimumValue: -100
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( liftwheel.greenF != spinnerToWheel(value) ) {
                        liftwheel.greenF = spinnerToWheel(value)
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
                minimumValue: -100
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gammawheel.greenF != spinnerToWheel(value) ) {
                        gammawheel.greenF = spinnerToWheel(value)
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
                minimumValue: -100
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gainwheel.greenF != spinnerToWheel(value) ) {
                        gainwheel.greenF = spinnerToWheel(value)
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
                minimumValue: -100
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( liftwheel.blueF != spinnerToWheel(value) ) {
                        liftwheel.blueF = spinnerToWheel(value)
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
                minimumValue: -100
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gammawheel.blueF != spinnerToWheel(value) ) {
                        gammawheel.blueF = spinnerToWheel(value)
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
                minimumValue: -100
                maximumValue: 100
                decimals: 1
                stepSize: 0.1
                suffix: ' %'
                onValueChanged: {
                    if( gainwheel.blueF != spinnerToWheel(value) ) {
                        gainwheel.blueF = spinnerToWheel(value)
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
