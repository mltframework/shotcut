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
import QtQuick.Dialogs 1.1

Item {
    property string rectProperty: 'geometry'
    property string valignProperty: 'valign'
    property string halignProperty: 'halign'
    property string useFontSizeProperty: 'shotcut:usePointSize'
    property string pointSizeProperty: 'shotcut:pointSize'
    property rect filterRect: filter.getRect(rectProperty)
    width: 500
    height: 350

    Component.onCompleted: {
        if (filter.isNew) {
            if (application.OS === 'Windows')
                filter.set('family', 'Verdana')
            filter.set('fgcolour', '#ffffffff')
            filter.set('bgcolour', '#00000000')
            filter.set('olcolour', '#ff000000')
            filter.set('weight', 10 * Font.Normal)
            filter.set('style', 'normal')
            filter.set(useFontSizeProperty, false)

            filter.set(rectProperty,   '0/50%:50%x50%')
            filter.set(valignProperty, 'bottom')
            filter.set(halignProperty, 'left')
            filter.savePreset(preset.parameters, qsTr('Bottom Left'))

            filter.set(rectProperty,   '50%/50%:50%x50%')
            filter.set(valignProperty, 'bottom')
            filter.set(halignProperty, 'right')
            filter.savePreset(preset.parameters, qsTr('Bottom Right'))

            filter.set(rectProperty,   '0/0:50%x50%')
            filter.set(valignProperty, 'top')
            filter.set(halignProperty, 'left')
            filter.savePreset(preset.parameters, qsTr('Top Left'))

            filter.set(rectProperty,   '50%/0:50%x50%')
            filter.set(valignProperty, 'top')
            filter.set(halignProperty, 'right')
            filter.savePreset(preset.parameters, qsTr('Top Right'))

            filter.set(rectProperty,   '0/76%:100%x14%')
            filter.set(valignProperty, 'bottom')
            filter.set(halignProperty, 'center')
            filter.savePreset(preset.parameters, qsTr('Lower Third'))

            filter.set(rectProperty, '0/0:100%x100%')
            filter.set(valignProperty, 'bottom')
            filter.set(halignProperty, 'center')
            filter.set('size', filterRect.height)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setFilter() {
        var x = parseFloat(rectX.text)
        var y = parseFloat(rectY.text)
        var w = parseFloat(rectW.text)
        var h = parseFloat(rectH.text)
        if (x !== filterRect.x ||
            y !== filterRect.y ||
            w !== filterRect.width ||
            h !== filterRect.height) {
            filterRect.x = x
            filterRect.y = y
            filterRect.width = w
            filterRect.height = h
            filter.set(rectProperty, '%L1%/%L2%:%L3%x%L4%'
                       .arg(x / profile.width * 100)
                       .arg(y / profile.height * 100)
                       .arg(w / profile.width * 100)
                       .arg(h / profile.height * 100))
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
        textArea.text = filter.get('argument')
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
        fontSizeCheckBox.checked = parseInt(filter.get(useFontSizeProperty))
        refreshFontButton()
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
            parameters: [rectProperty, halignProperty, valignProperty, 'argument', 'size', 'style',
            'fgcolour', 'family', 'weight', 'olcolour', 'outline', 'bgcolour', 'pad', useFontSizeProperty]
            Layout.columnSpan: 4
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Text')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
        }
        TextArea {
            id: textArea
            Layout.columnSpan: 4
            textFormat: TextEdit.PlainText
            wrapMode: TextEdit.NoWrap
            Layout.minimumHeight: 40
            Layout.maximumHeight: 80
            Layout.minimumWidth: preset.width
            Layout.maximumWidth: preset.width
            text: '__empty__' // workaround initialization problem
            property int maxLength: 256
            onTextChanged: {
                if (text === '__empty__') return
                if (length > maxLength) {
                    text = text.substring(0, maxLength)
                    cursorPosition = maxLength
                }
                if (!parseInt(filter.get(useFontSizeProperty)))
                    filter.set('size', filterRect.height / text.split('\n').length)
                filter.set('argument', text)
            }
        }

        Label {
            text: qsTr('Insert field')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            Button {
                text: qsTr('# (Hash sign)')
                onClicked: textArea.insert(textArea.cursorPosition, '\\#')
            }
            Button {
                text: qsTr('Timecode')
                onClicked: textArea.insert(textArea.cursorPosition, '#timecode#')
            }
            Button {
                text: qsTr('Frame #', 'Frame number')
                onClicked: textArea.insert(textArea.cursorPosition, '#frame#')
            }
            Button {
                text: qsTr('File date')
                onClicked: textArea.insert(textArea.cursorPosition, '#localfiledate#')
            }
            Button {
                text: qsTr('File name')
                onClicked: textArea.insert(textArea.cursorPosition, '#resource#')
            }
        }

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
            Layout.columnSpan: 3
            Button {
                id: fontButton
                onClicked: {
                    fontDialog.font.pointSize = getPointSize()
                    fontDialog.open()
                }
                FontDialog {
                    id: fontDialog
                    title: "Please choose a font"
                    property string fontFamily: ''
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
                    onRejected: filter.set('family', fontFamily)
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
                        filter.set('size', filterRect.height / text.split('\n').length)
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
            Layout.columnSpan: 2
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
            Layout.columnSpan: 2
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
            Layout.columnSpan: 4
            TextField {
                id: rectX
                text: filterRect.x
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
            Label { text: ',' }
            TextField {
                id: rectY
                text: filterRect.y
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
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
                text: filterRect.width
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
            }
            Label { text: 'x' }
            TextField {
                id: rectH
                text: filterRect.height
                horizontalAlignment: Qt.AlignRight
                onEditingFinished: setFilter()
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
            text: qsTr('Middle')
            exclusiveGroup: valignGroup
            onClicked: filter.set(valignProperty, 'middle')
        }
        RadioButton {
            id: bottomRadioButton
            text: qsTr('Bottom')
            exclusiveGroup: valignGroup
            onClicked: filter.set(valignProperty, 'bottom')
        }
        Item { Layout.fillWidth: true }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        onChanged: {
            var newValue = filter.getRect(rectProperty)
            if (filterRect !== newValue)
                filterRect = newValue
        }
    }
}

