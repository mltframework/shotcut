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

Item {
    property string rectProperty: 'geometry'
    property string valignProperty: 'valign'
    property string halignProperty: 'halign'
    property string useFontSizeProperty: 'shotcut:usePointSize'
    property string pointSizeProperty: 'shotcut:pointSize'
    property rect filterRect
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue:  '_shotcut:endValue'

    width: 500
    height: 350

    Component.onCompleted: {
        filter.blockSignals = true
        filter.set(middleValue, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(startValue, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(endValue, Qt.rect(0, 0, profile.width, profile.height))
        if (filter.isNew) {
            var presetParams = preset.parameters.slice()
            var index = presetParams.indexOf('argument')
            if (index > -1)
                presetParams.splice(index, 1)

            if (application.OS === 'Windows')
                filter.set('family', 'Verdana')
            filter.set('fgcolour', '#ffffffff')
            filter.set('bgcolour', '#00000000')
            filter.set('olcolour', '#aa000000')
            filter.set('outline', 3)
            filter.set('weight', 10 * Font.Normal)
            filter.set('style', 'normal')
            filter.set(useFontSizeProperty, false)
            filter.set('size', profile.height)

            filter.set(rectProperty,   '0%/50%:50%x50%')
            filter.set(valignProperty, 'bottom')
            filter.set(halignProperty, 'left')
            filter.savePreset(presetParams, qsTr('Bottom Left'))

            filter.set(rectProperty,   '50%/50%:50%x50%')
            filter.set(valignProperty, 'bottom')
            filter.set(halignProperty, 'right')
            filter.savePreset(presetParams, qsTr('Bottom Right'))

            filter.set(rectProperty,   '0%/0%:50%x50%')
            filter.set(valignProperty, 'top')
            filter.set(halignProperty, 'left')
            filter.savePreset(presetParams, qsTr('Top Left'))

            filter.set(rectProperty,   '50%/0%:50%x50%')
            filter.set(valignProperty, 'top')
            filter.set(halignProperty, 'right')
            filter.savePreset(presetParams, qsTr('Top Right'))

            filter.set(rectProperty,   '0%/76%:100%x14%')
            filter.set(valignProperty, 'bottom')
            filter.set(halignProperty, 'center')
            filter.savePreset(presetParams, qsTr('Lower Third'))

            // Add some animated presets.
            filter.animateIn = Math.round(profile.fps)
            filter.set(rectProperty,   '0=-100%/0%:100%x100%; :1.0=0%/0%:100%x100%')
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slide In From Left'))
            filter.set(rectProperty,   '0=100%/0%:100%x100%; :1.0=0%/0%:100%x100%')
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slide In From Right'))
            filter.set(rectProperty,   '0=0%/-100%:100%x100%; :1.0=0%/0%:100%x100%')
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slide In From Top'))
            filter.set(rectProperty,   '0=0%/100%:100%x100%; :1.0=0%/0%:100%x100%')
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slide In From Bottom'))
            filter.animateIn = 0
            filter.animateOut = Math.round(profile.fps)
            filter.set(rectProperty,   ':-1.0=0%/0%:100%x100%; -1=-100%/0%:100%x100%')
            filter.savePreset(presetParams.concat('shotcut:animOut'), qsTr('Slide Out Left'))
            filter.set(rectProperty,   ':-1.0=0%/0%:100%x100%; -1=100%/0%:100%x100%')
            filter.savePreset(presetParams.concat('shotcut:animOut'), qsTr('Slide Out Right'))
            filter.set(rectProperty,   ':-1.0=0%/0%:100%x100%; -1=0%/-100%:100%x100%')
            filter.savePreset(presetParams.concat('shotcut:animOut'), qsTr('Slide Out Top'))
            filter.set(rectProperty,   ':-1.0=0%/0%:100%x100%; -1=0%/100%:100%x100%')
            filter.savePreset(presetParams.concat('shotcut:animOut'), qsTr('Slide Out Bottom'))
            filter.animateOut = 0
            filter.animateIn = filter.duration
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=-5%/-5%:110%x110%')
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom In'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=0%/0%:100%x100%')
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom Out'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=-10%/-5%:110%x110%')
            filter.deletePreset(qsTr('Slow Pan Left'))
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Move Left'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=0%/-5%:110%x110%')
            filter.deletePreset(qsTr('Slow Pan Right'))
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Move Right'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=-5%/-10%:110%x110%')
            filter.deletePreset(qsTr('Slow Pan Up'))
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Move Up'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=-5%/0%:110%x110%')
            filter.deletePreset(qsTr('Slow Pan Down'))
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Move Down'))
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=-10%/-10%:110%x110%')
            filter.deletePreset(qsTr('Slow Zoom In, Pan Up Left'))
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom In, Move Up Left'))
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=0%/0%:110%x110%')
            filter.deletePreset(qsTr('Slow Zoom In, Pan Down Right'))
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom In, Move Down Right'))
            filter.set(rectProperty,   '0=-10%/0%:110%x110%; -1=0%/0%:100%x100%')
            filter.deletePreset(qsTr('Slow Zoom Out, Pan Up Right'))
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Move Up Right'))
            filter.set(rectProperty,   '0=0%/-10%:110%x110%; -1=0%/0%:100%x100%')
            filter.deletePreset(qsTr('Slow Zoom Out, Pan Down Left'))
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Move Down Left'))
            filter.animateIn = 0
            filter.resetProperty(rectProperty)

            // Add default preset.
            filter.set(rectProperty, '0%/0%:100%x100%')
            filter.savePreset(presetParams)
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
        fontDialog.fontFamily = filter.get('family')
        fontSizeCheckBox.checked = parseInt(filter.get(useFontSizeProperty))
        refreshFontButton()
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
    }

    ExclusiveGroup { id: sizeGroup }
    ExclusiveGroup { id: halignGroup }
    ExclusiveGroup { id: valignGroup }

    GridLayout {
        columns: 6
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
            Layout.columnSpan: 5
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
            text: qsTr('Text')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
        }
        TextArea {
            id: textArea
            Layout.columnSpan: 5
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
                    filter.set('size', profile.height / text.split('\n').length)
                filter.set('argument', text)
            }
        }

        Label {
            text: qsTr('Insert field')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 5
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
                    modality: application.dialogModality
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

        Item { Layout.fillHeight: true }
    }

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
