/*
 * Copyright (c) 2014-2020 Meltytech, LLC
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
import QtQuick.Dialogs 1.1

GridLayout {
    columns: 6

    property string rectProperty: 'geometry'
    property string valignProperty: 'valign'
    property string halignProperty: 'halign'
    property string useFontSizeProperty: 'shotcut:usePointSize'
    property string pointSizeProperty: 'shotcut:pointSize'
    property rect filterRect
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue:  '_shotcut:endValue'
    property var parameters: [rectProperty, halignProperty, valignProperty, 'size', 'style', 'fgcolour', 'family', 'weight', 'olcolour', 'outline', 'bgcolour', 'pad', useFontSizeProperty]

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function updateFilter(position) {
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
                filter.set(rectProperty, filter.getRect(startValue), 1.0, 0)
                filter.set(rectProperty, filter.getRect(middleValue), 1.0, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set(rectProperty, filter.getRect(middleValue), 1.0, filter.duration - filter.animateOut)
                filter.set(rectProperty, filter.getRect(endValue), 1.0, filter.duration - 1)
            }
        } else if (!positionKeyframesButton.checked) {
            filter.resetProperty(rectProperty)
            filter.set(rectProperty, filter.getRect(middleValue))
        } else if (position !== null) {
            filter.set(rectProperty, filterRect, 1.0, position)
        }
    }

    function getPointSize() {
        var pointSize = parseInt(filter.get(pointSizeProperty))
        if (!pointSize) {
            var ratio = fontDialog.font.pointSize / fontDialog.font.pixelSize
            pointSize = filter.get('size') * ratio
        }
        return pointSize
    }

    function refreshFontButton() {
        var s = filter.get('family')
        if (filter.getDouble('weight') > 10 * Font.Medium)
            s += ' ' + qsTr('Bold')
        if (filter.get('style') === 'italic')
            s += ' ' + qsTr('Italic')
        if (parseInt(filter.get(useFontSizeProperty)))
            s += ' ' + getPointSize()
        fontButton.text = s
    }

    function setControls() {
        fgColor.value = filter.get('fgcolour')
        fontButton.text = filter.get('family')
        outlineColor.value = filter.get('olcolour')
        outlineSpinner.value = filter.getDouble('outline')
        bgColor.value = filter.get('bgcolour')
        padSpinner.value = filter.getDouble('pad')
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
        fontDialog.font = Qt.font({
            family: filter.get('family'),
            pointSize: getPointSize(),
            italic: filter.get('style') === 'italic',
            weight: filter.getDouble('weight') / 10
        })
        fontDialog.fontFamily = filter.get('family')
        fontSizeCheckBox.checked = parseInt(filter.get(useFontSizeProperty))
        refreshFontButton()
        setKeyframedControls()
    }

    function setKeyframedControls() {
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
        positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
    }

    ExclusiveGroup { id: sizeGroup }
    ExclusiveGroup { id: halignGroup }
    ExclusiveGroup { id: valignGroup }

    Label {
        text: qsTr('Font')
        Layout.alignment: Qt.AlignRight
    }
    ColorPicker {
        id: fgColor
        eyedropper: false
        alpha: true
        onValueChanged: filter.set('fgcolour', value)
    }
    RowLayout {
        Layout.columnSpan: 4
        Button {
            id: fontButton
            onClicked: {
                if (fontSizeCheckBox.checked) {
                    fontDialog.font.pointSize = getPointSize()
                } else {
                    fontDialog.font.pointSize = 48
                }
                fontDialog.open()
            }
            FontDialog {
                id: fontDialog
                title: "Please choose a font"
                property string fontFamily: ''
                modality: application.dialogModality
                onFontChanged: {
                    filter.set('family', font.family)
                    filter.set('weight', 10 * font.weight )
                    filter.set('style', font.italic? 'italic' : 'normal' )
                    if (parseInt(filter.get(useFontSizeProperty))) {
                        filter.set('size', font.pixelSize)
                        filter.set(pointSizeProperty, font.pointSize)
                    }
                    refreshFontButton()
                }
                onAccepted: fontFamily = font.family
                onRejected: {
                    filter.set('family', fontFamily)
                    refreshFontButton()
                }
            }
        }
        CheckBox {
            id: fontSizeCheckBox
            text: qsTr('Use font size')
            onCheckedChanged: {
                filter.set(useFontSizeProperty, checked)
                if (checked) {
                    filter.set('size', fontDialog.font.pixelSize)
                    filter.set(pointSizeProperty, fontDialog.font.pointSize)
                } else {
                    filter.set('size', profile.height / text.split('\n').length)
                }
                refreshFontButton()
            }
        }
    }

    Label {
        text: qsTr('Outline')
        Layout.alignment: Qt.AlignRight
    }
    ColorPicker {
        id: outlineColor
        eyedropper: false
        alpha: true
        onValueChanged: filter.set('olcolour', value)
    }
    Label {
        text: qsTr('Thickness')
        Layout.alignment: Qt.AlignRight
    }
    SpinBox {
        id: outlineSpinner
        Layout.minimumWidth: 50
        Layout.columnSpan: 3
        minimumValue: 0
        maximumValue: 30
        decimals: 0
        onValueChanged: filter.set('outline', value)
    }

    Label {
        text: qsTr('Background')
        Layout.alignment: Qt.AlignRight
    }
    ColorPicker {
        id: bgColor
        eyedropper: false
        alpha: true
        onValueChanged: filter.set('bgcolour', value)
    }
    Label {
        text: qsTr('Padding')
        Layout.alignment: Qt.AlignRight
    }
    SpinBox {
        id: padSpinner
        Layout.minimumWidth: 50
        Layout.columnSpan: 3
        minimumValue: 0
        maximumValue: 100
        decimals: 0
        onValueChanged: filter.set('pad', value)
    }

    Label {
        text: qsTr('Position')
        Layout.alignment: Qt.AlignRight
    }
    RowLayout {
        Layout.columnSpan: 3
        TextField {
            id: rectX
            horizontalAlignment: Qt.AlignRight
            onEditingFinished: if (filterRect.x !== parseFloat(text)) {
                filterRect.x = parseFloat(text)
                updateFilter(getPosition())
            }
        }
        Label { text: ',' }
        TextField {
            id: rectY
            horizontalAlignment: Qt.AlignRight
            onEditingFinished: if (filterRect.y !== parseFloat(text)) {
                filterRect.y = parseFloat(text)
                updateFilter(getPosition())
            }
        }
    }
    UndoButton {
        onClicked: {
            rectX.text = rectY.text = 0
            filterRect.x = filterRect.y = 0
            updateFilter(getPosition())
        }
    }
    KeyframesButton {
        id: positionKeyframesButton
        Layout.rowSpan: 2
        checked: filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
        onToggled: {
            if (checked) {
                filter.clearSimpleAnimation(rectProperty)
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
        Layout.columnSpan: 3
        TextField {
            id: rectW
            horizontalAlignment: Qt.AlignRight
            onEditingFinished: if (filterRect.width !== parseFloat(text)) {
                filterRect.width = parseFloat(text)
                updateFilter(getPosition())
            }
        }
        Label { text: 'x' }
        TextField {
            id: rectH
            horizontalAlignment: Qt.AlignRight
            onEditingFinished: if (filterRect.height !== parseFloat(text)) {
                filterRect.height = parseFloat(text)
                updateFilter(getPosition())
            }
        }
    }
    UndoButton {
        onClicked: {
            rectW.text = profile.width
            rectH.text = profile.height
            filterRect.width = profile.width
            filterRect.height = profile.height
            updateFilter(getPosition())
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
        onClicked: filter.set(halignProperty, 'left')
    }
    RadioButton {
        id: centerRadioButton
        text: qsTr('Center')
        exclusiveGroup: halignGroup
        onClicked: filter.set(halignProperty, 'center')
    }
    RadioButton {
        id: rightRadioButton
        text: qsTr('Right')
        exclusiveGroup: halignGroup
        onClicked: filter.set(halignProperty, 'right')
    }
    UndoButton {
        onClicked: {
            centerRadioButton.checked = true
            filter.set(halignProperty, 'center')
        }
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
        onClicked: filter.set(valignProperty, 'top')
    }
    RadioButton {
        id: middleRadioButton
        text: qsTr('Middle', 'Text video filter')
        exclusiveGroup: valignGroup
        onClicked: filter.set(valignProperty, 'middle')
    }
    RadioButton {
        id: bottomRadioButton
        text: qsTr('Bottom')
        exclusiveGroup: valignGroup
        onClicked: filter.set(valignProperty, 'bottom')
    }
    UndoButton {
        onClicked: {
            bottomRadioButton.checked = true
            filter.set(valignProperty, 'bottom')
        }
    }
    
    Item { Layout.fillWidth: true }

    Connections {
        target: filter
        onChanged: setKeyframedControls()
        onInChanged: updateFilter(null)
        onOutChanged: updateFilter(null)
        onAnimateInChanged: updateFilter(null)
        onAnimateOutChanged: updateFilter(null)
    }

    Connections {
        target: producer
        onPositionChanged: setKeyframedControls()
    }
}
