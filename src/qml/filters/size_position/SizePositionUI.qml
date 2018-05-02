/*
 * Copyright (c) 2014-2018 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0

Item {
    property string fillProperty
    property string distortProperty
    property string legacyRectProperty: null
    property string rectProperty
    property string valignProperty
    property string halignProperty
    property rect filterRect
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue:  '_shotcut:endValue'

    width: 350
    height: 180

    Component.onCompleted: {
        filter.blockSignals = true
        filter.set(middleValue, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(startValue, Qt.rect(-profile.width, 0, profile.width, profile.height))
        filter.set(endValue, Qt.rect(profile.width, 0, profile.width, profile.height))
        if (filter.isNew) {
            filter.set(fillProperty, 0)
            filter.set(distortProperty, 0)

            filter.set(rectProperty,   '0%/50%:50%x50%')
            filter.set(valignProperty, 'bottom')
            filter.set(halignProperty, 'left')
            filter.savePreset(preset.parameters, qsTr('Bottom Left'))

            filter.set(rectProperty,   '50%/50%:50%x50%')
            filter.set(valignProperty, 'bottom')
            filter.set(halignProperty, 'right')
            filter.savePreset(preset.parameters, qsTr('Bottom Right'))

            filter.set(rectProperty,   '0%/0%:50%x50%')
            filter.set(valignProperty, 'top')
            filter.set(halignProperty, 'left')
            filter.savePreset(preset.parameters, qsTr('Top Left'))

            filter.set(rectProperty,   '50%/0%:50%x50%')
            filter.set(valignProperty, 'top')
            filter.set(halignProperty, 'right')
            filter.savePreset(preset.parameters, qsTr('Top Right'))

            filter.set(rectProperty,   '0%/0%:100%x100%')
            filter.set(valignProperty, 'top')
            filter.set(halignProperty, 'left')
            filter.savePreset(preset.parameters)
            filter.set(fillProperty, 1)
        } else {
            if (legacyRectProperty !== null) {
                var old = filter.get(legacyRectProperty)
                if (old && old.length > 0) {
                    filter.resetProperty(legacyRectProperty)
                    filter.set(rectProperty, old)
                }
            }
            filterRect = filter.getRect(rectProperty)
            filter.set(middleValue, filter.getRect(rectProperty, filter.animateIn))
            if (filter.animateIn > 0)
                filter.set(startValue, filter.getRect(rectProperty, 0))
            if (filter.animateOut > 0)
                filter.set(endValue, filter.getRect(rectProperty, filter.duration - 1))
        }
        filter.blockSignals = false
        setControls()
        setKeyframedControls()
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setFilter(position, force) {
        var x = parseFloat(rectX.text)
        var y = parseFloat(rectY.text)
        var w = parseFloat(rectW.text)
        var h = parseFloat(rectH.text)
        if (force ||
            x !== filterRect.x ||
            y !== filterRect.y ||
            w !== filterRect.width ||
            h !== filterRect.height)
        {
            filterRect.x = x
            filterRect.y = y
            filterRect.width = w
            filterRect.height = h
            if (position !== null) {
                if (position <= 0)
                    filter.set(startValue, filterRect)
                else if (position >= filter.duration - 1)
                    filter.set(endValue, filterRect)
                else
                    filter.set(middleValue, filterRect)
            }

            filter.resetProperty(rectProperty)
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                if (filter.animateIn > 0) {
                    filter.set(rectProperty, filter.getRect(startValue), 1.0, 0)
                    filter.set(rectProperty, filter.getRect(middleValue), 1.0, filter.animateIn - 1)
                }
                if (filter.animateOut > 0) {
                    filter.set(rectProperty, filter.getRect(middleValue), 1.0, filter.duration - filter.animateOut)
                    filter.set(rectProperty, filter.getRect(endValue), 1.0, filter.duration - 1)
                }
            } else {
                filter.set(rectProperty, filter.getRect(middleValue))
            }
        }
    }

    function setControls() {
        if (filter.get(distortProperty) === '1')
            distortRadioButton.checked = true
        else if (filter.get(fillProperty) === '1')
            fillRadioButton.checked = true
        else
            fitRadioButton.checked = true
        var align = filter.get(halignProperty)
        if (align === 'left')
            leftRadioButton.checked = true
        else if (align === 'center' || align === 'middle')
            centerRadioButton.checked = true
        else if (filter.get(halignProperty) === 'right')
            rightRadioButton.checked = true
        align = filter.get(valignProperty)
        if (align === 'top')
            topRadioButton.checked = true
        else if (align === 'center' || align === 'middle')
            middleRadioButton.checked = true
        else if (align === 'bottom')
            bottomRadioButton.checked = true
    }

    function setKeyframedControls() {
        var position = getPosition()
        var newValue = filter.getRect(rectProperty, position)
        if (filterRect !== newValue)
            filterRect = newValue
        var enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
        rectX.enabled = enabled
        rectY.enabled = enabled
        rectW.enabled = enabled
        rectH.enabled = enabled
    }

    ExclusiveGroup { id: sizeGroup }
    ExclusiveGroup { id: halignGroup }
    ExclusiveGroup { id: valignGroup }

    GridLayout {
        columns: 5
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: [fillProperty, distortProperty, rectProperty, halignProperty, valignProperty]
            Layout.columnSpan: 4
            onBeforePresetLoaded: {
                filter.resetProperty(rectProperty)
            }
            onPresetSelected: {
                setControls()
                filterRect = filter.getRect(rectProperty, getPosition())
                filter.set(middleValue, filter.getRect(rectProperty, filter.animateIn))
                if (filter.animateIn > 0)
                    filter.set(startValue, filter.getRect(rectProperty, 0))
                if (filter.animateOut > 0)
                    filter.set(endValue, filter.getRect(rectProperty, filter.duration - 1))
            }
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            TextField {
                id: rectX
                text: filterRect.x.toFixed()
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter(getPosition())
            }
            Label { text: ',' }
            TextField {
                id: rectY
                text: filterRect.y.toFixed()
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter(getPosition())
            }
        }
        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            TextField {
                id: rectW
                text: filterRect.width.toFixed()
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter(getPosition())
            }
            Label { text: 'x' }
            TextField {
                id: rectH
                text: filterRect.height.toFixed()
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter(getPosition())
            }
        }

        Label {
            text: qsTr('Size mode')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            RadioButton {
                id: fitRadioButton
                text: qsTr('Fit')
                exclusiveGroup: sizeGroup
                onClicked: {
                    filter.set(fillProperty, 0)
                    filter.set(distortProperty, 0)
                }
            }
            RadioButton {
                id: fillRadioButton
                text: qsTr('Fill')
                exclusiveGroup: sizeGroup
                onClicked: {
                    filter.set(fillProperty, 1)
                    filter.set(distortProperty, 0)
                }
            }
            RadioButton {
                id: distortRadioButton
                text: qsTr('Distort')
                exclusiveGroup: sizeGroup
                onClicked: {
                    filter.set(fillProperty, 1)
                    filter.set(distortProperty, 1)
                }
            }
        }

        Label {
            text: qsTr('Horizontal fit')
            Layout.alignment: Qt.AlignRight
        }
        RadioButton {
            id: leftRadioButton
            text: qsTr('Left')
            exclusiveGroup: halignGroup
            enabled: fitRadioButton.checked
            onClicked: filter.set(halignProperty, 'left')
        }
        RadioButton {
            id: centerRadioButton
            text: qsTr('Center')
            exclusiveGroup: halignGroup
            enabled: fitRadioButton.checked
            onClicked: filter.set(halignProperty, 'center')
        }
        RadioButton {
            id: rightRadioButton
            text: qsTr('Right')
            exclusiveGroup: halignGroup
            enabled: fitRadioButton.checked
            onClicked: filter.set(halignProperty, 'right')
        }
        Item { Layout.fillWidth: true }

        Label {
            text: qsTr('Vertical fit')
            Layout.alignment: Qt.AlignRight
        }
        RadioButton {
            id: topRadioButton
            text: qsTr('Top')
            exclusiveGroup: valignGroup
            enabled: fitRadioButton.checked
            onClicked: filter.set(valignProperty, 'top')
        }
        RadioButton {
            id: middleRadioButton
            text: qsTr('Middle')
            exclusiveGroup: valignGroup
            enabled: fitRadioButton.checked
            onClicked: filter.set(valignProperty, 'middle')
        }
        RadioButton {
            id: bottomRadioButton
            text: qsTr('Bottom')
            exclusiveGroup: valignGroup
            enabled: fitRadioButton.checked
            onClicked: filter.set(valignProperty, 'bottom')
        }
        Item { Layout.fillWidth: true }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        onChanged: setKeyframedControls()
        onInChanged: setFilter(null)
        onOutChanged: setFilter(null)
        onAnimateInChanged: setFilter(null, true)
        onAnimateOutChanged: setFilter(null, true)
    }

    Connections {
        target: producer
        onPositionChanged: {
            if (filter.animateIn > 0 || filter.animateOut > 0)
                setKeyframedControls()
        }
    }
}
