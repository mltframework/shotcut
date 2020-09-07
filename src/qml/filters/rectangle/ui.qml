/*
 * Copyright (c) 2020 Meltytech, LLC
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
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import Shotcut.Controls 1.0

Item {
    width: 400
    height: 100
    property bool blockUpdate: true
    property double startValueRadius: 0
    property double middleValueRadius: 0
    property double endValueRadius: 0
    property string startValueRect: '_shotcut:startValue'
    property string middleValueRect: '_shotcut:middleValue'
    property string endValueRect:  '_shotcut:endValue'
    property string rectProperty: 'rect'
    property rect filterRect

    Component.onCompleted: {
        filter.blockSignals = true
        filter.set('mlt_resolution_scale', 1)
        filter.set(startValueRect, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(middleValueRect, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(endValueRect, Qt.rect(0, 0, profile.width, profile.height))
        if (filter.isNew) {
            filter.set('resource', filter.path + 'rectangle.qml')
            // Set default parameter values
            filter.set('color.1', '#80ff0000')
            filter.set('radius', 0)
            filter.set('border.color', '#80ffffff')
            filter.set('border.width', 3)
            filter.set(rectProperty, '40%/40%:20%x20%')
            filter.savePreset(preset.parameters)
        } else {
            middleValueRadius = filter.getDouble('radius', filter.animateIn)
            filter.set(middleValueRect, filter.getRect(rectProperty, filter.animateIn + 1))
            if (filter.animateIn > 0) {
                startValueRadius = filter.getDouble('radius', 0)
                filter.set(startValueRect, filter.getRect(rectProperty, 0))
            }
            if (filter.animateOut > 0) {
                endValueRadius = filter.getDouble('radius', filter.duration - 1)
                filter.set(endValueRect, filter.getRect(rectProperty, filter.duration - 1))
            }
        }
        filter.blockSignals = false
        setRatioControls()
        setRectControls()
        fillColorPicker.colors = filter.getGradient('color')
        borderColorPicker.value = filter.get('border.color');
        borderWidthSpinner.value = filter.get('border.width')
        if (filter.isNew)
            filter.set(rectProperty, filter.getRect(rectProperty))
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setRatioControls() {
        var position = getPosition()
        blockUpdate = true
        slider.value = filter.getDouble('radius', position) * slider.maximumValue
        blockUpdate = false
        slider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
        radiusKeyframesButton.checked = filter.keyframeCount('radius') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
    }

    function setRectControls() {
        var position = getPosition()
        var newValue = filter.getRect(rectProperty, position)
        if (filterRect !== newValue) {
            filterRect = newValue
            rectX.text = filterRect.x.toFixed()
            rectY.text = filterRect.y.toFixed()
            rectW.text = filterRect.width.toFixed()
            rectH.text = filterRect.height.toFixed()
        }
        var enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
        rectX.enabled = enabled
        rectY.enabled = enabled
        rectW.enabled = enabled
        rectH.enabled = enabled
    }
    
    function updateFilterRadius(position) {
        if (blockUpdate) return
        var value = slider.value / 100.0

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0) {
                startValueRadius = value
            } else if (position >= filter.duration - 1 && filter.animateOut > 0) {
                endValueRadius = value
            } else {
                middleValueRadius = value
            }
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('radius')
            radiusKeyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set('radius', startValueRadius, 0)
                filter.set('radius', middleValueRadius, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set('radius', middleValueRadius, filter.duration - filter.animateOut)
                filter.set('radius', endValueRadius, filter.duration - 1)
            }
        } else if (!radiusKeyframesButton.checked) {
            filter.resetProperty('radius')
            filter.set('radius', middleValueRadius)
        } else if (position !== null) {
            filter.set('radius', value, position)
        }
    }

    function updateFilterRect(position) {
        if (blockUpdate) return
        var rect

        if (position !== null) {
            filter.blockSignals = true
            if (position <= 0 && filter.animateIn > 0) {
                filter.set(startValueRect, filterRect)
            } else if (position >= filter.duration - 1 && filter.animateOut > 0) {
                filter.set(endValueRect, filterRect)
            } else {
                filter.set(middleValueRect, filterRect)
            }
            filter.blockSignals = false
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(rectProperty)
            positionKeyframesButton.checked = false
            if (filter.animateIn > 0) {
                rect = filter.getRect(startValueRect)
                filter.set(rectProperty, rect, 1.0, 0)
                rect = filter.getRect(middleValueRect)
                filter.set(rectProperty, rect, 1.0, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                rect = filter.getRect(middleValueRect)
                filter.set(rectProperty, rect, 1.0, filter.duration - filter.animateOut)
                rect = filter.getRect(endValueRect)
                filter.set(rectProperty, rect, 1.0, filter.duration - 1)
            }
        } else if (!positionKeyframesButton.checked) {
            filter.resetProperty(rectProperty)
            rect = filter.getRect(middleValueRect)
            filter.set(rectProperty, rect)
        } else if (position !== null) {
            filter.set(rectProperty, filterRect, 1.0, position)
        }
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: [rectProperty, 'radius', 'color.1', 'color.2', 'color.3', 'color.4', 'color.5', 'color.6', 'color.7', 'color.8', 'color.9', 'color.10', 'border.color', 'border.width']
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filterRect = Qt.rect(0, 0, 0, 0)
                filter.resetProperty(rectProperty)
                filter.resetProperty('radius')
                filter.setGradient('color', [])
            }
            onPresetSelected: {
                setRatioControls()
                setRectControls()
                blockUpdate = true
                fillColorPicker.colors = filter.getGradient('color')
                borderColorPicker.value = filter.get('border.color')
                borderWidthSpinner.value = filter.get('border.width')
                blockUpdate = false
                positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
                middleValueRadius = filter.getDouble('radius', filter.animateIn)
                filter.set(middleValueRect, filter.getRect(rectProperty, filter.animateIn + 1))
                if (filter.animateIn > 0) {
                    startValueRadius = filter.getDouble('radius', 0)
                    filter.set(startValueRect, filter.getRect(rectProperty, 0))
                }
                if (filter.animateOut > 0) {
                    endValueRadius = filter.getDouble('radius', filter.duration - 1)
                    filter.set(endValueRect, filter.getRect(rectProperty, filter.duration - 1))
                }
            }
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            TextField {
                id: rectX
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: if (filterRect.x !== parseFloat(text)) {
                    filterRect.x = parseFloat(text)
                    updateFilterRect(getPosition(), true)
                }
            }
            Label { text: ',' }
            TextField {
                id: rectY
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: if (filterRect.y !== parseFloat(text)) {
                    filterRect.y = parseFloat(text)
                    updateFilterRect(getPosition(), true)
                }
            }
        }
        UndoButton {
            onClicked: {
                rectX.text = rectY.text = 0
                filterRect.x = filterRect.y = 0
                updateFilterRect(getPosition(), true)
            }
        }
        KeyframesButton {
            id: positionKeyframesButton
            Layout.rowSpan: 2
            checked: filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
            onToggled: {
                if (checked) {
                    filter.blockSignals = true
                    filter.clearSimpleAnimation(rectProperty)
                    filter.blockSignals = false
                    filter.set(rectProperty, filterRect, 1.0, getPosition())
                } else {
                    filter.resetProperty(rectProperty)
                    filter.set(rectProperty, filterRect)
                }
                checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
            }
        }
    
        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            TextField {
                id: rectW
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: if (filterRect.width !== parseFloat(text)) {
                    filterRect.width = parseFloat(text)
                    updateFilterRect(getPosition(), true)
                }
            }
            Label { text: 'x' }
            TextField {
                id: rectH
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: if (filterRect.height !== parseFloat(text)) {
                    filterRect.height = parseFloat(text)
                    updateFilterRect(getPosition(), true)
                }
            }
        }
        UndoButton {
            onClicked: {
                rectW.text = profile.width
                rectH.text = profile.height
                filterRect.width = profile.width
                filterRect.height = profile.height
                updateFilterRect(getPosition(), true)
            }
        }
    
        Label {
            text: qsTr('Corner radius')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: slider
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilterRadius(getPosition())
        }
        UndoButton {
            onClicked: slider.value = 0
        }
        KeyframesButton {
            id: radiusKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('radius') > 0
            onToggled: {
                var value = slider.value / 100.0
                if (checked) {
                    filter.clearSimpleAnimation('radius')
                    filter.set('radius', value, getPosition())
                } else {
                    filter.resetProperty('radius')
                    filter.set('radius', value)
                }
                checked = filter.keyframeCount('radius') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
            }
        }


        Label {
            text: qsTr('Fill color')
            Layout.alignment: Qt.AlignRight
        }
        GradientControl {
            id: fillColorPicker
            Layout.columnSpan: 2
            onGradientChanged: {
                 if (blockUpdate) return
                 filter.setGradient('color', colors)
            }
        }
        Item { width: 1 }
        
        Label {
            text: qsTr('Outline')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            spacing: 8
            Layout.columnSpan: 3
            ColorPicker {
                id: borderColorPicker
                eyedropper: false
                alpha: true
                onValueChanged: if (!blockUpdate) filter.set('border.color', value)
            }
            Label {
                text: qsTr('Thickness')
                Layout.alignment: Qt.AlignRight
            }
            SpinBox {
                id: borderWidthSpinner
                Layout.minimumWidth: 50
                Layout.columnSpan: 3
                minimumValue: 0
                decimals: 0
                onValueChanged: if (!blockUpdate) filter.set('border.width', value)
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    function updateFilter() {
        updateFilterRadius(null)
        updateFilterRect(null)
    }
    
    Connections {
        target: filter
        onChanged: setRectControls()
        onInChanged: updateFilter()
        onOutChanged: updateFilter()
        onAnimateInChanged: updateFilter()
        onAnimateOutChanged: updateFilter()
    }

    Connections {
        target: producer
        onPositionChanged: {
            setRatioControls()
            setRectControls()
        }
    }
}
