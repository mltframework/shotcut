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
    property string rectProperty: 'rect'
    property rect filterRect
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue:  '_shotcut:endValue'

    width: 350
    height: 180

    Component.onCompleted: {
        filter.blockSignals = true
        var rect = defaultRect()
        filter.set(middleValue, rect)
        filter.set(startValue, rect)
        filter.set(endValue, rect)
        if (filter.isNew) {
            // Add default preset.
            filter.set(rectProperty, '' + rect.x + '/' + rect.y + ':' + rect.width + 'x' + rect.height)
            filter.set("blur", 4)
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

    function defaultRect() {
        var result
        if (producer.displayAspectRatio > profile.aspectRatio) {
            result = Qt.rect(0, 0, profile.width, Math.round(profile.width / producer.displayAspectRatio))
        } else {
            result = Qt.rect(0, 0, Math.round(profile.height * producer.displayAspectRatio), profile.height)
        }
        result.x = Math.round((profile.width - result.width) / 2)
        result.y = Math.round((profile.height - result.height) / 2)
        return result
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setFilter(position) {
        if (position !== null) {
            filter.blockSignals = true
            if (position <= 0 && filter.animateIn > 0)
                filter.set(startValue, filterRect)
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                filter.set(endValue, filterRect)
            else
                filter.set(middleValue, filterRect)
            filter.blockSignals = false
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(rectProperty)
            positionKeyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set(rectProperty, filter.getRect(startValue), 0)
                filter.set(rectProperty, filter.getRect(middleValue), filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set(rectProperty, filter.getRect(middleValue), filter.duration - filter.animateOut)
                filter.set(rectProperty, filter.getRect(endValue), filter.duration - 1)
            }
        } else if (!positionKeyframesButton.checked) {
            filter.resetProperty(rectProperty)
            filter.set(rectProperty, filter.getRect(middleValue))
        } else if (position !== null) {
            filter.set(rectProperty, filterRect, position)
        }
    }

    function setControls() {
        amountSlider.value = filter.get('blur')
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
        positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
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
            parameters: [rectProperty]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty(rectProperty)
            }
            onPresetSelected: {
                setControls()
                setKeyframedControls()
                positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
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
                    setFilter(getPosition())
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
                    setFilter(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                var rect = defaultRect()
                filterRect.x = rectX.value = rect.x
                filterRect.y = rectY.value = rect.y
                setFilter(getPosition())
            }
        }
        Shotcut.KeyframesButton {
            id: positionKeyframesButton
            Layout.rowSpan: 2
            onToggled: {
                if (checked) {
                    filter.clearSimpleAnimation(rectProperty)
                    filter.set(rectProperty, filterRect, getPosition())
                } else {
                    filter.resetProperty(rectProperty)
                    filter.set(rectProperty, filterRect)
                }
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
                    setFilter(getPosition())
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
                    setFilter(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                var rect = defaultRect()
                filterRect.width = rectW.value = rect.width
                filterRect.height = rectH.value = rect.height
                setFilter(getPosition())
            }
        }

        Label {
            text: qsTr('Blur')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: amountSlider
            minimumValue: 0
            maximumValue: 10.0
            stepSize: .1
            decimals: 2
            suffix: ' %'
            onValueChanged: filter.set("blur", value)
        }
        Shotcut.UndoButton {
            onClicked: amountSlider.value = 4
        }
        Item { width: 1 }


        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        onChanged: setKeyframedControls()
        onInChanged: setFilter(null)
        onOutChanged: setFilter(null)
        onAnimateInChanged: setFilter(null)
        onAnimateOutChanged: setFilter(null)
    }

    Connections {
        target: producer
        onPositionChanged: setKeyframedControls()
    }
}
