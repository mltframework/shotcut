/*
 * Copyright (c) 2020-2021 Meltytech, LLC
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
    width: 400
    height: 150
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
        filter.set(startValueRect, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(middleValueRect, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(endValueRect, Qt.rect(0, 0, profile.width, profile.height))
        if (filter.isNew) {
            // Set default parameter values
            filter.set('color', '#ff000000')
            filter.set('radius', 0)
            filter.set(rectProperty, '0%/0%:100%x100%')
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
        colorSwatch.value = filter.get('color')
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
        positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
    }
    
    function updateFilterRatio(position) {
        if (blockUpdate) return
        var value = slider.value / 100.0
        setRatioControls()
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
        var rect
        setRectControls()
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
        Shotcut.Preset {
            id: preset
            parameters: [rectProperty, 'radius', 'color']
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filterRect = Qt.rect(0, 0, 0, 0)
                filter.resetProperty(rectProperty)
                filter.resetProperty('radius')
            }
            onPresetSelected: {
                setRatioControls()
                setRectControls()
                colorSwatch.value = filter.get('color')
                filter.blockSignals = true
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
                filter.blockSignals = false
            }
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
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
                    updateFilterRect(getPosition(), true)
                }
            }
            Label { text: ','; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
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
                    updateFilterRect(getPosition(), true)
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                rectX.value = rectY.value = 0
                filterRect.x = filterRect.y = 0
                updateFilterRect(getPosition(), true)
            }
        }
        Shotcut.KeyframesButton {
            id: positionKeyframesButton
            Layout.rowSpan: 2
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
                    updateFilterRect(getPosition(), true)
                }
            }
            Label { text: 'x'; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
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
                    updateFilterRect(getPosition(), true)
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                rectW.value = profile.width
                rectH.value = profile.height
                filterRect.width = profile.width
                filterRect.height = profile.height
                updateFilterRect(getPosition(), true)
            }
        }
    
        Label {
            text: qsTr('Corner radius')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilterRatio(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider.value = 0
        }
        Shotcut.KeyframesButton {
            id: radiusKeyframesButton
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
            text: qsTr('Padding color')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Shotcut.ColorPicker {
                id: colorSwatch
                alpha: true
                property bool isReady: false
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        filter.set('color', value)
                        filter.set("disable", 0);
                    }
                }
                onPickStarted: {
                    filter.set("disable", 1);
                }
                onPickCancelled: filter.set('disable', 0)
            }
            Shotcut.Button {
                text: qsTr('Transparent')
                onClicked: colorSwatch.value = '#00000000'
            }
        }
        Shotcut.UndoButton {
            onClicked: colorSwatch.value = '#FF000000'
        }
        Item { width: 1 }

        Item {
            Layout.fillHeight: true
        }
    }

    function updateFilter() {
        updateFilterRatio(null)
        updateFilterRect(null)
    }
    
    Connections {
        target: filter
        onChanged: {
            setRectControls()
            setRatioControls()
        }
        onInChanged: updateFilter()
        onOutChanged: updateFilter()
        onAnimateInChanged: updateFilter()
        onAnimateOutChanged: updateFilter()
        onPropertyChanged: {
            setRectControls()
            setRatioControls()
        }
    }

    Connections {
        target: producer
        onPositionChanged: {
            setRatioControls()
            setRectControls()
        }
    }
}
