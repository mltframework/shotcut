/*
 * Copyright (c) 2020-2022 Meltytech, LLC
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
    property string rectProperty: 'geometry'
    property rect filterRect
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue:  '_shotcut:endValue'
    property string sizeProperty: '_shotcut:size'
    property string specialPresetProperty: 'shotcut:preset'
    property rect defaultRect: Qt.rect(Math.round(profile.width * 0.1),
                                       Math.round(profile.height * 0.1),
                                       Math.round(profile.width * 0.8),
                                       Math.round(profile.height * 0.8))

    width: 350
    height: 230

    Component.onCompleted: {
        filter.blockSignals = true
        filter.set(middleValue, defaultRect)
        filter.set(startValue, defaultRect)
        filter.set(endValue, defaultRect)
        if (filter.isNew) {
            var presetParams = [rectProperty]
            filter.set('html', '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd">
<html><head><meta name="qrichtext" content="1" /><style type="text/css">
p, li { white-space: pre-wrap; }
body { font-family:%1; font-size:72pt; font-weight:600; font-style:normal; color:#ffffff; }
</style></head><body></body></html>
'.arg(application.OS === 'Windows'? 'Verdana' : 'sans-serif'))
            filter.set('argument', '')
            filter.set('bgcolour', '#00000000')

            filter.set(rectProperty,   '5%/66.66%:90%x28.34%')
            filter.savePreset(presetParams, qsTr('Lower Third'))
            filter.set(rectProperty,   '0%/0%:100%x100%')
            filter.savePreset(presetParams, qsTr('Full Screen'))

            // Add some animated presets.
            filter.animateIn = filter.duration
            filter.set(specialPresetProperty, 'scroll-down')
            filter.savePreset(['shotcut:animIn', specialPresetProperty], qsTr('Scroll Down'))
            filter.set(specialPresetProperty, 'scroll-up')
            filter.savePreset(['shotcut:animIn', specialPresetProperty], qsTr('Scroll Up'))
            filter.set(specialPresetProperty, 'scroll-right')
            filter.savePreset(['shotcut:animIn', specialPresetProperty], qsTr('Scroll Right'))
            filter.set(specialPresetProperty, 'scroll-left')
            filter.savePreset(['shotcut:animIn', specialPresetProperty], qsTr('Scroll Left'))
            filter.resetProperty(specialPresetProperty)

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

            // Add default preset.
            filter.animateIn = 0
            filter.resetProperty(rectProperty)
            filter.set(rectProperty, '10%/10%:80%x80%')
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
        bgColor.value = filter.get('bgcolour')
        switch (filter.get('overflow-y')) {
        case '':
            automaticOverflowRadioButton.checked = true
            break;
        case '0':
            hiddenOverflowRadioButton.checked = true
            break;
        default:
            visibleOverflowRadioButton.checked = true
        }
    }

    function getTextDimensions() {
        var document = filter.getRect(sizeProperty)
        if (bgColor.value.substring(0, 3) !== '#00') {
            document.height = Math.max(document.height, filterRect.height)
        }
        return document
    }

    function setKeyframedControls() {
        var position = getPosition()
        var newValue = filter.getRect(rectProperty, position)
        if (filterRect !== newValue) {
            filterRect = newValue
            rectX.value = filterRect.x
            rectY.value = filterRect.y
            rectW.value = filterRect.width
            rectH.value = filterRect.height
        }
        var enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
        rectX.enabled = enabled
        rectY.enabled = enabled
        rectW.enabled = enabled
        rectH.enabled = enabled
        positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0

        var document = getTextDimensions()
        if (parseInt(sizeW.text) !== Math.round(document.width) || parseInt(sizeH.text) !== Math.round(document.height)) {
            sizeW.text = Math.round(document.width)
            sizeH.text = Math.round(document.height)
            handleSpecialPreset()
        }
    }

    function handleSpecialPreset() {
        if (filter.get(specialPresetProperty)) {
            var document = getTextDimensions()
            filter.blockSignals = true
            filter.resetProperty(rectProperty)
            filter.animateIn = filter.duration
            filter.blockSignals = false
            var s
            if (filter.get(specialPresetProperty) === 'scroll-down') {
                s = '0=' + filterRect.x + '/-' + Math.round(document.height) + ':' + filterRect.width + 'x' + filterRect.height +
                 '; -1=' + filterRect.x + '/' + profile.height + ':' + filterRect.width + 'x' + filterRect.height
            } else if (filter.get(specialPresetProperty) === 'scroll-up') {
                s = '0=' + filterRect.x + '/' + profile.height + ':' + filterRect.width + 'x' + filterRect.height +
                 '; -1=' + filterRect.x + '/-' + Math.round(document.height) + ':' + filterRect.width + 'x' + filterRect.height
            } else if (filter.get(specialPresetProperty) === 'scroll-right') {
                s = '0=-' + Math.round(document.width) + '/' + filterRect.y + ':' + filterRect.width + 'x' + filterRect.height +
                '; -1=' + profile.width + '/' + filterRect.y + ':' + filterRect.width + 'x' + filterRect.height
            } else if (filter.get(specialPresetProperty) === 'scroll-left') {
                s = '0=' + profile.width + '/' + filterRect.y + ':' + filterRect.width + 'x' + filterRect.height +
                '; -1=-' + Math.round(document.width) + '/' + filterRect.y + ':' + filterRect.width + 'x' + filterRect.height
            }
            if (s) {
                console.log(filter.get(specialPresetProperty) + ': ' + s)
                filter.set(rectProperty, s)
            }
        }
    }

    GridLayout {
        columns: 6
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            id: preset
            parameters: [rectProperty, 'bgcolour', 'overflow-y']
            Layout.columnSpan: 5
            onBeforePresetLoaded: {
                filter.resetProperty(rectProperty)
                filter.resetProperty(specialPresetProperty)
            }
            onPresetSelected: {
                handleSpecialPreset()
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

        Shotcut.TipBox {
            Layout.columnSpan: parent.columns
            Layout.margins: 10
            Layout.fillWidth: true
            text: qsTr('Click in the rectangle atop the video to edit the text.')
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 3
            Shotcut.DoubleSpinBox {
                id: rectX
                horizontalAlignment: Qt.AlignRight
                Layout.minimumWidth: 100
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: {
                    if (Math.abs(filterRect.x - value) > 1) {
                        filterRect.x = value
                        updateFilter(getPosition())
                    }
                }
            }
            Label { text: ','; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
            Shotcut.DoubleSpinBox {
                id: rectY
                horizontalAlignment: Qt.AlignRight
                Layout.minimumWidth: 100
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: {
                    if (Math.abs(filterRect.y - value) > 1) {
                        filterRect.y = value
                        updateFilter(getPosition())
                    }
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filterRect.x = rectX.value = defaultRect.x
                filterRect.y = rectY.value = defaultRect.y
                updateFilter(getPosition())
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
                checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
            }
        }

        Label {
            text: qsTr('Background size')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 3
            Shotcut.DoubleSpinBox {
                id: rectW
                horizontalAlignment: Qt.AlignRight
                Layout.minimumWidth: 100
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: {
                    if (Math.abs(filterRect.width - value) > 1) {
                        filterRect.width = value
                        updateFilter(getPosition())
                    }
                }
            }
            Label { text: 'x'; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
            Shotcut.DoubleSpinBox {
                id: rectH
                horizontalAlignment: Qt.AlignRight
                Layout.minimumWidth: 100
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: {
                    if (Math.abs(filterRect.height - value) > 1) {
                        filterRect.height = value
                        updateFilter(getPosition())
                    }
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filterRect.width = rectW.value = defaultRect.width
                filterRect.height = rectH.value = defaultRect.height
                updateFilter(getPosition())
            }
        }

        Label {
            text: qsTr('Text size')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 3
            TextField {
                id: sizeW
                horizontalAlignment: Qt.AlignRight
                readOnly: true
                opacity: 0.7
                selectByMouse: true
                persistentSelection: true
                MouseArea {
                    acceptedButtons: Qt.RightButton
                    anchors.fill: parent
                    onClicked: contextMenu.popup()
                }
                Menu {
                    id: contextMenu
                    width: 150
                    MenuItem {
                        action: Action {
                            text: qsTr('Copy')
                            onTriggered: { sizeW.selectAll(); sizeW.copy() }
                        }
                    }
                }
            }
            Label { text: 'x'; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
            TextField {
                id: sizeH
                horizontalAlignment: Qt.AlignRight
                readOnly: true
                opacity: 0.7
                selectByMouse: true
                persistentSelection: true
                MouseArea {
                    acceptedButtons: Qt.RightButton
                    anchors.fill: parent
                    onClicked: contextMenu2.popup()
                }
                Menu {
                    id: contextMenu2
                    width: 150
                    MenuItem {
                        action: Action {
                            text: qsTr('Copy')
                            onTriggered: { sizeH.selectAll(); sizeH.copy() }
                        }
                    }
                }
            }
        }
        Item { Layout.columnSpan: 2; Layout.fillWidth: true }

        Label {
            text: qsTr('Background color')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ColorPicker {
            id: bgColor
            Layout.columnSpan: 3
            eyedropper: false
            alpha: true
            onValueChanged: filter.set('bgcolour', value)
        }
        Shotcut.UndoButton {
            onClicked: bgColor.value = '#00000000'
        }
        Item { Layout.fillWidth: true }

        Label {
            text: qsTr('Overflow')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 3
            ButtonGroup { id: overflowGroup }
            RadioButton {
                id: automaticOverflowRadioButton
                text: qsTr('Automatic')
                ButtonGroup.group: overflowGroup
                onClicked: {
                    filter.set('overflow-y', '')
                    filter.resetProperty('overflow-y')
                }
            }
            RadioButton {
                id: visibleOverflowRadioButton
                text: qsTr('Visible')
                ButtonGroup.group: overflowGroup
                onClicked: filter.set('overflow-y', 1)
            }
            RadioButton {
                id: hiddenOverflowRadioButton
                text: qsTr('Hidden')
                ButtonGroup.group: overflowGroup
                onClicked: filter.set('overflow-y', 0)
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.resetProperty('overflow-y')
                automaticOverflowRadioButton.checked = true
            }
        }
        Item { Layout.fillWidth: true }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        function onChanged() { setKeyframedControls() }
        function onInChanged() { updateFilter(null) }
        function onOutChanged() { updateFilter(null) }
        function onAnimateInChanged() { updateFilter(null) }
        function onAnimateOutChanged() { updateFilter(null) }
    }

    Connections {
        target: producer
        function onPositionChanged() { setKeyframedControls() }
    }
}
