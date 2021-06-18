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


import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import Shotcut.Controls 1.0 as Shotcut


Item {
    // Constants

    property double thresholdMin: 0.00003
    property double thresholdMax: 0.5
    property double directionMax: 6.28319


    // Parameters

    property string thr1Param: 'av.1thr'
    property double thr1Default: 0.02

    property string thr2Param: 'av.2thr'
    property double thr2Default: 0.02

    property string thr3Param: 'av.3thr'
    property double thr3Default: 0.02

    property string thr4Param: 'av.4thr'
    property double thr4Default: 0.02

    property string linkParam: 'ui.link'
    property bool linkDefault: true

    property string rangeParam: 'av.range'
    property int rangeDefault: 16

    property string directionParam: 'av.direction'
    property double directionDefault: directionMax

    property string blurParam: 'av.blur'
    property bool blurDefault: true

    property string couplingParam: 'av.coupling'
    property bool couplingDefault: false

    property var allParams: [thr1Param, thr2Param, thr3Param, thr4Param, linkParam, rangeParam, directionParam, blurParam, couplingParam]


    // Conversion functions between filter and UI units

    function pctToThr (value) {
        return (thresholdMax - thresholdMin) * (value / 100) + thresholdMin
    }


    function thrToPct (value) {
        return Math.round(((value - thresholdMin) / (thresholdMax - thresholdMin) + Number.EPSILON) * 1000) / 10
    }


    function sqrToRoot (value) {
        return Math.round((Math.sqrt(Math.abs(value)) + Number.EPSILON) * 10) / 10
    }


    function rootToSqr (value) {
        return Math.round(value * value + Number.EPSILON)
    }


    function radToDeg (value) {
        return Math.round(Math.abs(value / directionMax * 360) + Number.EPSILON)
    }


    function degToRad (value) {
        return value / 360 * directionMax
    }


    // UI management functions

    function setThreshold (param, pct) {
        if (idLink.checked) {
            var thr = pctToThr(pct)

            filter.set(thr1Param, thr)
            filter.set(thr2Param, thr)
            filter.set(thr3Param, thr)
            filter.set(thr4Param, thr)

            idThr1.value = pct
            idThr2.value = pct
            idThr3.value = pct
            idThr4.value = pct
        }
        else {
            filter.set(param, pctToThr(pct))
        }
    }


    function setControls () {
        idLink.checked = false
        idThr1.value = thrToPct(filter.getDouble(thr1Param))
        idThr2.value = thrToPct(filter.getDouble(thr2Param))
        idThr3.value = thrToPct(filter.getDouble(thr3Param))
        idThr4.value = thrToPct(filter.getDouble(thr4Param))
        idLink.checked = parseInt(filter.get(linkParam))

        // The Randomize checkboxes must be set first or else sign inversion will happen.
        idRangeRand.checked = parseInt(filter.get(rangeParam)) >= 0 ? true : false
        idRange.value = sqrToRoot(parseInt(filter.get(rangeParam)))
        idDirectionRand.checked = filter.getDouble(directionParam) >= 0 ? true : false
        idDirection.value = radToDeg(filter.getDouble(directionParam))

        idBlur.checked = parseInt(filter.get(blurParam))
        idCoupling.checked = parseInt(filter.get(couplingParam))
    }


    Component.onCompleted: {
        filter.blockSignals = true
        if (filter.isNew) {
            // Custom preset
            filter.set(thr1Param, thr1Default)
            filter.set(thr2Param, thr2Default)
            filter.set(thr3Param, thr3Default)
            filter.set(thr4Param, thr4Default)
            filter.set(linkParam, linkDefault)
            filter.set(rangeParam, rangeDefault)
            filter.set(directionParam, directionDefault)
            filter.set(blurParam, blurDefault)
            filter.set(couplingParam, couplingDefault)
            filter.savePreset(allParams, qsTr('FFmpeg default values'))

            // Custom preset
            filter.set(thr1Param, 0.0150291)
            filter.set(thr2Param, 0.03994)
            filter.set(thr3Param, 0.0150291)
            filter.set(thr4Param, 0.0150291)
            filter.set(linkParam, false)
            filter.set(rangeParam, 90)
            filter.set(directionParam, directionMax)
            filter.set(blurParam, true)
            filter.set(couplingParam, false)
            filter.savePreset(allParams, qsTr('Blue gradient in open sky'))

            // Custom preset
            filter.set(thr1Param, 0.0150291)
            filter.set(thr2Param, 0.0150291)
            filter.set(thr3Param, 0.03994)
            filter.set(thr4Param, 0.0150291)
            filter.set(linkParam, false)
            filter.set(rangeParam, 44)
            filter.set(directionParam, directionMax)
            filter.set(blurParam, true)
            filter.set(couplingParam, false)
            filter.savePreset(allParams, qsTr('Red gradient in open sky'))

            // Custom preset
            filter.set(thr1Param, 0.0100294)
            filter.set(thr2Param, 0.0100294)
            filter.set(thr3Param, 0.0100294)
            filter.set(thr4Param, 0.0100294)
            filter.set(linkParam, true)
            filter.set(rangeParam, 23)
            filter.set(directionParam, directionMax)
            filter.set(blurParam, true)
            filter.set(couplingParam, false)
            filter.savePreset(allParams, qsTr('Full range to Limited range'))

            // Default preset
            // Same as Full to Limited preset
            filter.savePreset(allParams)
        }
        filter.blockSignals = false

        setControls()
    }


    width: 350
    height: 450


    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        // Row split

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

        // Row split

        Label {
            text: qsTr('Y/R Threshold')
            Layout.alignment: Qt.AlignRight
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

        // Row split

        Label {
            text: qsTr('Cb/G Threshold')
            Layout.alignment: Qt.AlignRight
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

        // Row split

        Label {
            text: qsTr('Cr/B Threshold')
            Layout.alignment: Qt.AlignRight
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

        // Row split

        Label {
            text: qsTr('Alpha Threshold')
            Layout.alignment: Qt.AlignRight
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

        // Row split

        Label {}
        CheckBox {
            id: idLink
            text: qsTr('Link thresholds')
            onClicked: filter.set(linkParam, checked)
        }
        Label {}

        // Filler

        Label {
            Layout.columnSpan: 3
        }

        // Row split

        Label {
            text: qsTr('Pixel Range')
            Layout.alignment: Qt.AlignRight
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
                filter.set(rangeParam, rangeDefault)
                setControls()
            }
        }

        // Row split

        Label {}
        CheckBox {
            id: idRangeRand
            text: qsTr('Randomize between zero and value')
            onClicked: filter.set(rangeParam, checked ? rootToSqr(idRange.value) : -rootToSqr(idRange.value))
        }
        Label {}

        // Row split

        Label {
            text: qsTr('Direction')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: idDirection
            minimumValue: 0
            maximumValue: 360
            suffix: ' °'
            Shotcut.HoverTip { text: qsTr('Up = 270°\nDown = 90°\nLeft = 180°\nRight = 0°/360°') }
            onValueChanged: filter.set(directionParam, idDirectionRand.checked ? degToRad(value) : -degToRad(value))
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.set(directionParam, directionDefault)
                setControls()
            }
        }

        // Row split

        Label {}
        CheckBox {
            id: idDirectionRand
            text: qsTr('Randomize between 0° and value')
            onClicked: filter.set(directionParam, checked ? degToRad(idDirection.value) : -degToRad(idDirection.value))
        }
        Label {}

        // Filler

        Label {
            Layout.columnSpan: 3
        }

        // Row split

        Label {}
        CheckBox {
            id: idBlur
            text: qsTr('Use average of neighbors')
            Shotcut.HoverTip { text: qsTr('Compare to thresholds using average versus exact neighbor values') }
            onClicked: filter.set(blurParam, checked)
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.set(blurParam, blurDefault)
                idBlur.checked = blurDefault
            }
        }

        // Row split

        Label {}
        CheckBox {
            id: idCoupling
            text: qsTr('All components required')
            Shotcut.HoverTip { text: qsTr('Deband only if all pixel components (including alpha) are within thresholds') }
            onClicked: filter.set(couplingParam, checked)
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.set(couplingParam, couplingDefault)
                idCoupling.checked = couplingDefault
            }
        }

        // Filler

        Label {
            Layout.columnSpan: 3
        }

        // Row split

        Label {
            text: qsTr('More information') + ': <a href="http://ffmpeg.org/ffmpeg-all.html#deband">FFmpeg deband</a>'
            Shotcut.HoverTip { text: 'http://ffmpeg.org/ffmpeg-all.html#deband' }
            Layout.alignment: Qt.AlignHCenter
            Layout.columnSpan: 3

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }

            onLinkActivated: Qt.openUrlExternally(link)
        }

        // Filler

        Item {
            Layout.columnSpan: 3
            Layout.fillHeight: true
        }
    }
}
