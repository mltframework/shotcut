/*
 * Copyright (c) 2021 Meltytech, LLC
 * Written by Austin Brooks <ab.shotcut@outlook.com>
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Shotcut.Controls as Shotcut

Item {

    // Constants
    // Parameters
    // Conversion functions between filter and UI units
    // UI management functions
    property double thresholdMin: 3e-05
    property double thresholdMax: 0.5
    property double directionMax: 6.28319
    property string thr1Param: 'av.1thr'
    property double thr1Default: 0.0100294
    property string thr2Param: 'av.2thr'
    property double thr2Default: 0.0100294
    property string thr3Param: 'av.3thr'
    property double thr3Default: 0.0100294
    property string thr4Param: 'av.4thr'
    property double thr4Default: 0.0100294
    property string linkParam: 'ui.link'
    property bool linkDefault: true
    property string rangeParam: 'av.range'
    property int rangeDefault: 23
    property string directionParam: 'av.direction'
    property double directionDefault: directionMax
    property string blurParam: 'av.blur'
    property bool blurDefault: true
    property string couplingParam: 'av.coupling'
    property bool couplingDefault: false
    property var allParams: [thr1Param, thr2Param, thr3Param, thr4Param, linkParam, rangeParam, directionParam, blurParam, couplingParam]

    function pctToThr(value) {
        return (thresholdMax - thresholdMin) * (value / 100) + thresholdMin;
    }

    function thrToPct(value) {
        return Math.round(((value - thresholdMin) / (thresholdMax - thresholdMin) + Number.EPSILON) * 1000) / 10;
    }

    function sqrToRoot(value) {
        return Math.round((Math.sqrt(Math.abs(value)) + Number.EPSILON) * 10) / 10;
    }

    function rootToSqr(value) {
        return Math.round(value * value + Number.EPSILON);
    }

    function radToDeg(value) {
        return Math.round(Math.abs(value / directionMax * 360) + Number.EPSILON);
    }

    function degToRad(value) {
        return value / 360 * directionMax;
    }

    function setThreshold(param, pct) {
        if (idLink.checked) {
            var thr = pctToThr(pct);
            filter.set(thr1Param, thr);
            filter.set(thr2Param, thr);
            filter.set(thr3Param, thr);
            filter.set(thr4Param, thr);
            idThr1.value = pct;
            idThr2.value = pct;
            idThr3.value = pct;
            idThr4.value = pct;
        } else {
            filter.set(param, pctToThr(pct));
        }
    }

    function setControls() {
        idLink.checked = false;
        idThr1.value = thrToPct(filter.getDouble(thr1Param));
        idThr2.value = thrToPct(filter.getDouble(thr2Param));
        idThr3.value = thrToPct(filter.getDouble(thr3Param));
        idThr4.value = thrToPct(filter.getDouble(thr4Param));
        idLink.checked = parseInt(filter.get(linkParam));
        // The Randomize checkboxes must be set first or else sign inversion will happen.
        idRangeRand.checked = parseInt(filter.get(rangeParam)) >= 0 ? true : false;
        idRange.value = sqrToRoot(parseInt(filter.get(rangeParam)));
        idDirectionRand.checked = filter.getDouble(directionParam) >= 0 ? true : false;
        idDirection.value = radToDeg(filter.getDouble(directionParam));
        idBlur.checked = parseInt(filter.get(blurParam));
        idCoupling.checked = parseInt(filter.get(couplingParam));
    }

    Component.onCompleted: {
        filter.blockSignals = true;
        if (filter.isNew) {
            // Custom preset
            filter.set(thr1Param, 0.0100294);
            filter.set(thr2Param, 0.0100294);
            filter.set(thr3Param, 0.0100294);
            filter.set(thr4Param, 0.0100294);
            filter.set(linkParam, true);
            filter.set(rangeParam, 23);
            filter.set(directionParam, directionMax);
            filter.set(blurParam, true);
            filter.set(couplingParam, false);
            filter.savePreset(allParams, qsTr('Minimal strength'));
            // Custom preset
            filter.set(thr1Param, 0.02);
            filter.set(thr2Param, 0.02);
            filter.set(thr3Param, 0.02);
            filter.set(thr4Param, 0.02);
            filter.set(linkParam, true);
            filter.set(rangeParam, 16);
            filter.set(directionParam, directionMax);
            filter.set(blurParam, true);
            filter.set(couplingParam, false);
            filter.savePreset(allParams, qsTr('Average strength'));
            // Custom preset
            filter.set(thr1Param, 0.0150291);
            filter.set(thr2Param, 0.03994);
            filter.set(thr3Param, 0.0150291);
            filter.set(thr4Param, 0.0150291);
            filter.set(linkParam, false);
            filter.set(rangeParam, 90);
            filter.set(directionParam, directionMax);
            filter.set(blurParam, true);
            filter.set(couplingParam, false);
            filter.savePreset(allParams, qsTr('Blue sky'));
            // Custom preset
            filter.set(thr1Param, 0.0150291);
            filter.set(thr2Param, 0.0150291);
            filter.set(thr3Param, 0.03994);
            filter.set(thr4Param, 0.0150291);
            filter.set(linkParam, false);
            filter.set(rangeParam, 44);
            filter.set(directionParam, directionMax);
            filter.set(blurParam, true);
            filter.set(couplingParam, false);
            filter.savePreset(allParams, qsTr('Red sky'));
            // Custom preset
            filter.set(thr1Param, thr1Default);
            filter.set(thr2Param, thr2Default);
            filter.set(thr3Param, thr3Default);
            filter.set(thr4Param, thr4Default);
            filter.set(linkParam, linkDefault);
            filter.set(rangeParam, rangeDefault);
            filter.set(directionParam, directionDefault);
            filter.set(blurParam, blurDefault);
            filter.set(couplingParam, couplingDefault);
            filter.savePreset(allParams, qsTr('Full range to limited range'));
            // Default preset
            // Same as "Full to limited" preset; assumed most common use case
            filter.savePreset(allParams);
        }
        filter.blockSignals = false;
        setControls();
    }
    width: 500
    height: 360

    GridLayout {

        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Row split
        // Filler
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: idPreset

            Layout.columnSpan: 2
            parameters: allParams
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Contrast threshold')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Banding similarity within first component\nY (luma) in YCbCr mode\nRed in RGB mode')
            }
        }

        Shotcut.SliderSpinner {
            id: idThr1

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: setThreshold(thr1Param, value)
        }

        Shotcut.UndoButton {
            onClicked: idThr1.value = thrToPct(thr1Default)
        }

        Label {
            text: qsTr('Blue threshold')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Banding similarity within second component\nCb (blue) in YCbCr mode\nGreen in RGB mode')
            }
        }

        Shotcut.SliderSpinner {
            id: idThr2

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: setThreshold(thr2Param, value)
        }

        Shotcut.UndoButton {
            onClicked: idThr2.value = thrToPct(thr2Default)
        }

        Label {
            text: qsTr('Red threshold')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Banding similarity within third component\nCr (red) in YCbCr mode\nBlue in RGB mode')
            }
        }

        Shotcut.SliderSpinner {
            id: idThr3

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: setThreshold(thr3Param, value)
        }

        Shotcut.UndoButton {
            onClicked: idThr3.value = thrToPct(thr3Default)
        }

        Label {
            text: qsTr('Alpha threshold')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Banding similarity within fourth component')
            }
        }

        Shotcut.SliderSpinner {
            id: idThr4

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: setThreshold(thr4Param, value)
        }

        Shotcut.UndoButton {
            onClicked: idThr4.value = thrToPct(thr4Default)
        }

        Item {
            Layout.fillWidth: true
        }

        CheckBox {
            id: idLink

            text: qsTr('Link thresholds')
            onClicked: filter.set(linkParam, checked)
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr('Pixel range')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The size of bands being targeted')
            }
        }

        Shotcut.SliderSpinner {
            id: idRange

            minimumValue: 0
            maximumValue: 64
            decimals: 1
            onValueChanged: filter.set(rangeParam, idRangeRand.checked ? rootToSqr(value) : -rootToSqr(value))
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(rangeParam, rangeDefault);
                setControls();
            }
        }

        Item {
            Layout.fillWidth: true
        }

        CheckBox {
            id: idRangeRand

            text: qsTr('Randomize pixel range between zero and value')
            onClicked: filter.set(rangeParam, checked ? rootToSqr(idRange.value) : -rootToSqr(idRange.value))
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr('Direction')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Up = 270°\nDown = 90°\nLeft = 180°\nRight = 0° or 360°\nAll = 360° + Randomize')
            }
        }

        Shotcut.SliderSpinner {
            id: idDirection

            minimumValue: 0
            maximumValue: 360
            suffix: ' °'
            onValueChanged: filter.set(directionParam, idDirectionRand.checked ? degToRad(value) : -degToRad(value))
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(directionParam, directionDefault);
                setControls();
            }
        }

        Item {
            Layout.fillWidth: true
        }

        CheckBox {
            id: idDirectionRand

            text: qsTr('Randomize direction between zero degrees and value')
            onClicked: filter.set(directionParam, checked ? degToRad(idDirection.value) : -degToRad(idDirection.value))
        }

        Item {
            Layout.fillWidth: true
        }

        Item {
            Layout.fillWidth: true
        }

        CheckBox {
            id: idBlur

            text: qsTr('Measure similarity using average of neighbors')
            onClicked: filter.set(blurParam, checked)

            Shotcut.HoverTip {
                text: qsTr('Compare to thresholds using average versus exact neighbor values')
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(blurParam, blurDefault);
                idBlur.checked = blurDefault;
            }
        }

        Item {
            Layout.fillWidth: true
        }

        CheckBox {
            id: idCoupling

            text: qsTr('All components required to trigger deband')
            onClicked: filter.set(couplingParam, checked)

            Shotcut.HoverTip {
                text: qsTr('Deband only if all pixel components (including alpha) are within thresholds')
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(couplingParam, couplingDefault);
                idCoupling.checked = couplingDefault;
            }
        }

        Item {
            Layout.columnSpan: 3
            Layout.fillHeight: true
        }
    }
}
