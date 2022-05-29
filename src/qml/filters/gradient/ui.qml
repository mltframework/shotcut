/*
 * Copyright (c) 2019-2021 Meltytech, LLC
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
    property string rectProperty: 'shotcut:rect'
    property string patternProperty: '0'
    property string startColorProperty: '1'
    property string startOpacityProperty: '2'
    property string endColorProperty: '3'
    property string endOpacityProperty: '4'
    property string startXProperty: '5'
    property string startYProperty: '6'
    property string endXProperty: '7'
    property string endYProperty: '8'
    property string offsetProperty: '9'
    property string blendProperty: '10'
    property rect filterRect
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue:  '_shotcut:endValue'
    property bool _disableUpdate: true

    width: 350
    height: 180

    Component.onCompleted: {
        filter.blockSignals = true
        filter.set(middleValue, Qt.rect(profile.width / 2, 0, profile.width * 0.01, profile.height))
        filter.set(startValue, filter.getRect(middleValue))
        filter.set(endValue, filter.getRect(middleValue))
        if (filter.isNew) {
            filter.set(patternProperty, 'gradient_linear')
            filter.set(startColorProperty, '#000000')
            filter.set(startOpacityProperty, 1.0)
            filter.set(endColorProperty, '#ffffff')
            filter.set(endOpacityProperty, 1.0)
            filter.set(offsetProperty, 0)
            filter.set(blendProperty, 'normal')

            filter.set(rectProperty, '0%/50%:100%x1%')
            filter.set(startXProperty, 0)
            filter.set(startYProperty, 0.5)
            filter.set(endXProperty, 1.0)
            filter.set(endYProperty, 0.5)
            filter.savePreset(preset.parameters, qsTr('Horizontal'))

            filter.set(rectProperty, '50%/0%:1%x100%')
            filter.set(startXProperty, 0.5)
            filter.set(startYProperty, 0)
            filter.set(endXProperty, 0.5)
            filter.set(endYProperty, 1.0)
            // Add default preset.
            filter.savePreset(preset.parameters)
        } else {
            filter.set(middleValue, filter.getRect(rectProperty, filter.animateIn + 1))
            if (filter.animateIn > 0)
                filter.set(startValue, filter.getRect(rectProperty, 0))
            if (filter.animateOut > 0)
                filter.set(endValue, filter.getRect(rectProperty, filter.duration - 1))
        }
        filter.blockSignals = false
        setControls()
        setKeyframedControls()
        if (filter.isNew)
            filter.set(rectProperty, filter.getRect(rectProperty))
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function resetFilterRect() {
        filter.resetProperty(rectProperty)
        filter.resetProperty(startXProperty)
        filter.resetProperty(startYProperty)
        filter.resetProperty(endXProperty)
        filter.resetProperty(endYProperty)
    }

    function updateFilterRect(rect, position) {
        if (position === null)
            position = -1
        filter.set(rectProperty, rect, position)
        if (filter.get(patternProperty) === 'gradient_linear') {
            filter.set(startXProperty, rect.x / profile.width, position)
            filter.set(startYProperty, rect.y / profile.height, position)
            filter.set(endXProperty, (rect.x + rect.width) / profile.width, position)
            filter.set(endYProperty, (rect.y + rect.height) / profile.height, position)
        } else {
            filter.set(startXProperty, (rect.x + rect.width / 2) / profile.width, position)
            filter.set(startYProperty, (rect.y + rect.height / 2) / profile.height, position)
            filter.set(endXProperty, (rect.x + rect.width) / profile.width, position)
            filter.set(endYProperty, (rect.y + rect.height) / profile.height, position)
        }
    }

    function setFilter(position) {
        if (position !== null) {
            filter.blockSignals = true
            if (position <= 0 && filter.animateIn > 0) {
                filter.set(startValue, filterRect)
            } else if (position >= filter.duration - 1 && filter.animateOut > 0) {
                filter.set(endValue, filterRect)
            } else {
                filter.set(middleValue, filterRect)
            }
            filter.blockSignals = false
        }

        resetFilterRect()
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            if (filter.animateIn > 0) {
                updateFilterRect(filter.getRect(startValue), 0)
                updateFilterRect(filter.getRect(middleValue), filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                updateFilterRect(filter.getRect(middleValue), filter.duration - filter.animateOut)
                updateFilterRect(filter.getRect(endValue), filter.duration - 1)
            }
        } else {
            updateFilterRect(filter.getRect(middleValue))
        }
    }

    function setControls() {
        _disableUpdate = true
        if (filter.get(patternProperty) === 'gradient_linear')
            linearRadioButton.checked = true
        else
            radialRadioButton.checked = true
        offsetSlider.value = filter.getDouble(offsetProperty) * 100
        var colors = []
        var alpha = Math.round(filter.getDouble(endOpacityProperty) * 255).toString(16)
        if (alpha.length < 2)
            alpha = '0' + alpha
        colors[0] = filter.get(endColorProperty)
        colors[0] = colors[0].substring(colors[0].length - 6)
        colors[0] = '#' + alpha + colors[0]
        alpha = Math.round(filter.getDouble(startOpacityProperty) * 255).toString(16)
        if (alpha.length < 2)
            alpha = '0' + alpha
        colors[1] = filter.get(startColorProperty)
        colors[1] = colors[1].substring(colors[1].length - 6)
        colors[1] = '#' + alpha + colors[1]
        gradient.colors = colors
        var value = filter.get(blendProperty)
        for (var i = 0; i < comboItems.count; i++) {
            if (value === comboItems.get(i).value) {
                blendCombo.currentIndex = i
                break
            }
        }
        _disableUpdate = false
    }

    function setKeyframedControls() {
        var position = getPosition()
        var newValue = filter.getRect(rectProperty, position)
        if (filterRect !== newValue) {
            filterRect = newValue
            rectX.value = filterRect.x.toFixed()
            rectY.value = filterRect.y.toFixed()
            rectW.value = filterRect.width.toFixed()
            rectH.value = filterRect.height.toFixed()
        }
        var enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
        rectX.enabled = enabled
        rectY.enabled = enabled
        rectW.enabled = enabled
        rectH.enabled = enabled
    }

    ButtonGroup { id: patternGroup }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            id: preset
            parameters: [patternProperty, rectProperty, startColorProperty, startOpacityProperty, endColorProperty, endOpacityProperty, blendProperty]
            Layout.columnSpan: 2
            onBeforePresetLoaded: {
                filter.resetProperty(rectProperty)
            }
            onPresetSelected: {
                setControls()
                setKeyframedControls()
                filter.blockSignals = true
                filter.set(middleValue, filter.getRect(rectProperty, filter.animateIn + 1))
                if (filter.animateIn > 0)
                    filter.set(startValue, filter.getRect(rectProperty, 0))
                if (filter.animateOut > 0)
                    filter.set(endValue, filter.getRect(rectProperty, filter.duration - 1))
                filter.blockSignals = false
            }
        }

        Label {
            text: qsTr('Type')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            RadioButton {
                id: linearRadioButton
                text: qsTr('Linear')
                ButtonGroup.group: patternGroup
                onClicked: {
                    filter.set(patternProperty, 'gradient_linear')
                    setFilter(null)
                }
            }
            RadioButton {
                id: radialRadioButton
                text: qsTr('Radial')
                ButtonGroup.group: patternGroup
                onClicked: {
                    filter.set(patternProperty, 'gradient_radial')
                    setFilter(null)
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                linearRadioButton.checked = true
                filter.set(patternProperty, 'gradient_linear')
            }
        }

        Label {
            text: qsTr('Offset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: offsetSlider
            minimumValue: 0
            maximumValue: 99.9
            decimals: 1
            suffix: ' %'
            onValueChanged: filter.set(offsetProperty, value / 100)
        }
        Shotcut.UndoButton {
            onClicked: offsetSlider.value = 0
        }

        Label {
            text: qsTr('Colors')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.GradientControl {
            id: gradient
            spinnerVisible: false
            function cssColor(color) {
                if (color.length > 7) {
                    return '#' + color.substring(color.length - 6)
                }
                return color
            }
            onGradientChanged: {
                 if (_disableUpdate) return
                 var color = Qt.darker(colors[0], 1.0)
                 filter.set(endOpacityProperty, color.a)
                 filter.set(endColorProperty, cssColor(colors[0]))
                 if (colors.length > 0) {
                     color = Qt.darker(colors[1], 1.0)
                     filter.set(startOpacityProperty, color.a)
                     filter.set(startColorProperty, cssColor(colors[1]))
                 }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                gradient.colors = ['#ff000000', '#ffffffff']
                gradient.gradientChanged()
            }
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
            font.bold: true
            font.italic: true
        }
        RowLayout {
            Shotcut.DoubleSpinBox {
                id: rectX
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.x !== value) {
                    filterRect.x = value
                    setFilter(getPosition())
                }
            }
            Label { text: ','; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCente }
            Shotcut.DoubleSpinBox {
                id: rectY
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.y !== value) {
                    filterRect.y = value
                    setFilter(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filterRect.x = rectX.value = profile.width / 2
                filterRect.y = rectY.value = 0
                setFilter(getPosition())
            }
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
            font.bold: true
            font.italic: true
        }
        RowLayout {
            Shotcut.DoubleSpinBox {
                id: rectW
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.width !== value) {
                    filterRect.width = value
                    setFilter(getPosition())
                }
            }
            Label { text: 'x'; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCente }
            Shotcut.DoubleSpinBox {
                id: rectH
                Layout.minimumWidth: 100
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: if (filterRect.height !== value) {
                    filterRect.height = value
                    setFilter(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filterRect.width = rectW.value = profile.width * 0.01
                filterRect.height = rectH.value = profile.height
                setFilter(getPosition())
            }
        }

        Label { text: qsTr('Blend mode') }
        Shotcut.ComboBox {
            id: blendCombo
            model: ListModel {
                id: comboItems
                ListElement { text: qsTr('Over'); value: 'normal' }
                ListElement { text: qsTr('None'); value: '' }
                ListElement { text: qsTr('Add'); value: 'add' }
                ListElement { text: qsTr('Saturate'); value: 'saturate' }
                ListElement { text: qsTr('Multiply'); value: 'multiply' }
                ListElement { text: qsTr('Screen'); value: 'screen' }
                ListElement { text: qsTr('Overlay'); value: 'overlay' }
                ListElement { text: qsTr('Darken'); value: 'darken' }
                ListElement { text: qsTr('Dodge'); value: 'colordodge' }
                ListElement { text: qsTr('Burn'); value: 'colorburn' }
                ListElement { text: qsTr('Hard Light'); value: 'hardlight' }
                ListElement { text: qsTr('Soft Light'); value: 'softlight' }
                ListElement { text: qsTr('Difference'); value: 'difference' }
                ListElement { text: qsTr('Exclusion'); value: 'exclusion' }
                ListElement { text: qsTr('HSL Hue'); value: 'hslhue' }
                ListElement { text: qsTr('HSL Saturation'); value: 'hslsaturatation' }
                ListElement { text: qsTr('HSL Color'); value: 'hslcolor' }
                ListElement { text: qsTr('HSL Luminosity'); value: 'hslluminocity' }
            }
            textRole: 'text'
            onActivated: {
                filter.set(blendProperty, comboItems.get(currentIndex).value)
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.set(blendProperty, comboItems.get(0).value)
                blendCombo.currentIndex = 0
            }
        }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        function onChanged() { setKeyframedControls() }
        function onInChanged() { setFilter(null) }
        function onOutChanged() { setFilter(null) }
        function onAnimateInChanged() { setFilter(null) }
        function onAnimateOutChanged() { setFilter(null) }
    }

    Connections {
        target: producer
        function onPositionChanged() { setKeyframedControls() }
    }
}
