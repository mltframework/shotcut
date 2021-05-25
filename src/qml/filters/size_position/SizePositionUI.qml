/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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
    property string fillProperty
    property string distortProperty
    property string legacyRectProperty: null
    property string rectProperty
    property string valignProperty
    property string halignProperty
    property string backgroundProperty
    property string rotationProperty
    property rect filterRect
    property string startValue: '_shotcut:startValue'
    property string middleValue: '_shotcut:middleValue'
    property string endValue:  '_shotcut:endValue'
    property string rotationStartValue: '_shotcut:rotationStartValue'
    property string rotationMiddleValue: '_shotcut:rotationMiddleValue'
    property string rotationEndValue:  '_shotcut:rotationEndValue'
    property bool blockUpdate: true
    property rect defaultRect
    property real aspectRatio: producer.displayAspectRatio

    width: 425
    height: 250

    Component.onCompleted: {
        if (rotationProperty) {
            preset.parameters.push(rotationProperty)
        }

        filter.blockSignals = true
        filter.set(middleValue, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(startValue, Qt.rect(0, 0, profile.width, profile.height))
        filter.set(endValue, Qt.rect(0, 0, profile.width, profile.height))
        // Compute the default rectangle used also for parameter undos
        // Enforce the aspect ratio for fill mode
        if (producer.displayAspectRatio > profile.aspectRatio) {
            defaultRect.width = profile.width
            defaultRect.height = defaultRect.width / producer.displayAspectRatio
        } else {
            defaultRect.height = profile.height
            defaultRect.width = defaultRect.height * producer.displayAspectRatio
        }
        defaultRect.x = Math.round((profile.width - defaultRect.width) / 2)
        defaultRect.y = Math.round((profile.height - defaultRect.height) / 2)
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

            // Add some animated presets.
            filter.set(valignProperty, 'middle')
            filter.set(halignProperty, 'center')
            filter.animateIn = Math.round(profile.fps)
            filter.set(rectProperty,   '0=-100%/0%:100%x100%; :1.0=0%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slide In From Left'))
            filter.set(rectProperty,   '0=100%/0%:100%x100%; :1.0=0%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slide In From Right'))
            filter.set(rectProperty,   '0=0%/-100%:100%x100%; :1.0=0%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slide In From Top'))
            filter.set(rectProperty,   '0=0%/100%:100%x100%; :1.0=0%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slide In From Bottom'))
            filter.animateIn = 0
            filter.animateOut = Math.round(profile.fps)
            filter.set(rectProperty,   ':-1.0=0%/0%:100%x100%; -1=-100%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animOut'), qsTr('Slide Out Left'))
            filter.set(rectProperty,   ':-1.0=0%/0%:100%x100%; -1=100%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animOut'), qsTr('Slide Out Right'))
            filter.set(rectProperty,   ':-1.0=0%/0%:100%x100%; -1=0%/-100%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animOut'), qsTr('Slide Out Top'))
            filter.set(rectProperty,   ':-1.0=0%/0%:100%x100%; -1=0%/100%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animOut'), qsTr('Slide Out Bottom'))
            filter.set(fillProperty, 1)
            filter.animateOut = 0
            filter.animateIn = filter.duration
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=-5%/-5%:110%x110%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom In'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=0%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom Out'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=-10%/-5%:110%x110%')
            filter.deletePreset(qsTr('Slow Pan Left'))
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Move Left'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=0%/-5%:110%x110%')
            filter.deletePreset(qsTr('Slow Pan Right'))
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Move Right'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=-5%/-10%:110%x110%')
            filter.deletePreset(qsTr('Slow Pan Up'))
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Move Up'))
            filter.set(rectProperty,   '0=-5%/-5%:110%x110%; -1=-5%/0%:110%x110%')
            filter.deletePreset(qsTr('Slow Pan Down'))
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Move Down'))
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=-10%/-10%:110%x110%')
            filter.deletePreset(qsTr('Slow Zoom In, Pan Up Left'))
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom In, Move Up Left'))
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=0%/0%:110%x110%')
            filter.deletePreset(qsTr('Slow Zoom In, Pan Down Right'))
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom In, Move Down Right'))
            filter.set(rectProperty,   '0=-10%/0%:110%x110%; -1=0%/0%:100%x100%')
            filter.deletePreset(qsTr('Slow Zoom Out, Pan Up Right'))
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Move Up Right'))
            filter.set(rectProperty,   '0=0%/-10%:110%x110%; -1=0%/0%:100%x100%')
            filter.deletePreset(qsTr('Slow Zoom Out, Pan Down Left'))
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Move Down Left'))
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=-5%/-10%:110%x110%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom In, Hold Bottom'))
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=-5%/0%:110%x110%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom In, Hold Top'))
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=0%/-5%:110%x110%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom In, Hold Left'))
            filter.set(rectProperty,   '0=0%/0%:100%x100%; -1=-10%/-5%:110%x110%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom In, Hold Right'))
            filter.set(rectProperty,   '0=-5%/-10%:110%x110%; -1=0%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Hold Bottom'))
            filter.set(rectProperty,   '0=-5%/0%:110%x110%; -1=0%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Hold Top'))
            filter.set(rectProperty,   '0=0%/-5%:110%x110%; -1=0%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Hold Left'))
            filter.set(rectProperty,   '0=-10%/-5%:110%x110%; -1=0%/0%:100%x100%')
            filter.savePreset(preset.parameters.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Hold Right'))
            filter.animateIn = 0
            filter.resetProperty(rectProperty)

            filter.set('_shotcut:test_locale', 0.1)
            if (filter.get('_shotcut:test_locale') === '0,1') {
                filter.set(rectProperty,
                      '00:00:00,000= -7,937%  -7,648% 115% 115%; 00:00:00,080= -7,781% -11,815% 115% 115%;'
                    + '00:00:00,160= -0,094% -13,019% 115% 115%; 00:00:00,240= -7,313%  -9,037% 115% 115%;'
                    + '00:00:00,320= -7,469% -13,760% 115% 115%; 00:00:00,400=-10,229%  -5,593% 115% 115%;'
                    + '00:00:00,480= -6,615% -11,074% 115% 115%; 00:00:00,560= -5,031%  -6,074% 115% 115%;'
                    + '00:00:00,640= -2,990%  -6,074% 115% 115%; 00:00:00,720= -3,260%  -3,574% 115% 115%;'
                    + '00:00:00,800= -5,229%  -7,093% 115% 115%; 00:00:00,880= -5,906%  -3,574% 115% 115%;'
                    + '00:00:00,960=-10,958%  -9,315% 115% 115%; 00:00:01,040= -7,500%  -7,500% 115% 115%')
                filter.savePreset(preset.parameters, qsTr('Shake 1 Second - Scaled'))
                filter.set(rectProperty,
                     '00:00:00,000=  -0,437%  -0,148% 100% 100%; 00:00:00,080= -0,281%  -4,315% 100% 100%;'
                   + '00:00:00,160=   7,406%  -5,519% 100% 100%; 00:00:00,240=  0,187%  -1,537% 100% 100%;'
                   + '00:00:00,320=   0,031%  -6,260% 100% 100%; 00:00:00,400= -2,729%   1,907% 100% 100%;'
                   + '00:00:00,480=   0,885%  -3,574% 100% 100%; 00:00:00,560=  2,469%   1,426% 100% 100%;'
                   + '00:00:00,640=   4,510%   1,426% 100% 100%; 00:00:00,720=  4,240%   3,926% 100% 100%;'
                   + '00:00:00,800=   2,271%   0,407% 100% 100%; 00:00:00,880=  1,594%   3,926% 100% 100%;'
                   + '00:00:00,960=  -3,458%  -1,815% 100% 100%; 00:00:01,040=  0,000%   0,000% 100% 100%')
                filter.savePreset(preset.parameters, qsTr('Shake 1 Second - Unscaled'))
            } else {
                filter.set(rectProperty,
                      '00:00:00.000= -7.937%  -7.648% 115% 115%; 00:00:00.080= -7.781% -11.815% 115% 115%;'
                    + '00:00:00.160= -0.094% -13.019% 115% 115%; 00:00:00.240= -7.313%  -9.037% 115% 115%;'
                    + '00:00:00.320= -7.469% -13.760% 115% 115%; 00:00:00.400=-10.229%  -5.593% 115% 115%;'
                    + '00:00:00.480= -6.615% -11.074% 115% 115%; 00:00:00.560= -5.031%  -6.074% 115% 115%;'
                    + '00:00:00.640= -2.990%  -6.074% 115% 115%; 00:00:00.720= -3.260%  -3.574% 115% 115%;'
                    + '00:00:00.800= -5.229%  -7.093% 115% 115%; 00:00:00.880= -5.906%  -3.574% 115% 115%;'
                    + '00:00:00.960=-10.958%  -9.315% 115% 115%; 00:00:01.040= -7.500%  -7.500% 115% 115%')
                filter.savePreset(preset.parameters, qsTr('Shake 1 Second - Scaled'))
                filter.set(rectProperty,
                     '00:00:00.000=  -0.437%  -0.148% 100% 100%; 00:00:00.080= -0.281%  -4.315% 100% 100%;'
                   + '00:00:00.160=   7.406%  -5.519% 100% 100%; 00:00:00.240=  0.187%  -1.537% 100% 100%;'
                   + '00:00:00.320=   0.031%  -6.260% 100% 100%; 00:00:00.400= -2.729%   1.907% 100% 100%;'
                   + '00:00:00.480=   0.885%  -3.574% 100% 100%; 00:00:00.560=  2.469%   1.426% 100% 100%;'
                   + '00:00:00.640=   4.510%   1.426% 100% 100%; 00:00:00.720=  4.240%   3.926% 100% 100%;'
                   + '00:00:00.800=   2.271%   0.407% 100% 100%; 00:00:00.880=  1.594%   3.926% 100% 100%;'
                   + '00:00:00.960=  -3.458%  -1.815% 100% 100%; 00:00:01.040=  0.000%   0.000% 100% 100%')
                filter.savePreset(preset.parameters, qsTr('Shake 1 Second - Unscaled'))
            }
            filter.resetProperty('_shotcut:test_locale')
            filter.resetProperty(rectProperty)

            // Add default preset.
            if (backgroundProperty)
                filter.set(backgroundProperty, 'color:#00000000')
            filter.set(rectProperty, defaultRect)
            filter.get(rectProperty)
            filter.savePreset(preset.parameters)
        } else {
            if (legacyRectProperty !== null) {
                var old = filter.get(legacyRectProperty)
                if (old && old.length > 0) {
                    filter.resetProperty(legacyRectProperty)
                    filter.set(rectProperty, old)
                }
            }
            filter.set(middleValue, filter.getRect(rectProperty, filter.animateIn + 1))
            if (filter.animateIn > 0)
                filter.set(startValue, filter.getRect(rectProperty, 0))
            if (filter.animateOut > 0)
                filter.set(endValue, filter.getRect(rectProperty, filter.duration - 1))
            filter.set(rotationMiddleValue, filter.getDouble(rotationProperty, filter.animateIn + 1))
            if (filter.animateIn > 0)
                filter.set(rotationStartValue, filter.getDouble(rotationProperty, 0))
            if (filter.animateOut > 0)
                filter.set(rotationEndValue, filter.getRect(rotationProperty, filter.duration - 1))
        }
        updateAspectRatio()
        filter.blockSignals = false
        setControls()
        setKeyframedControls()
        if (filter.isNew)
            setFilter(getPosition())
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function isFitMode() {
        return filter.get(fillProperty) === '0' && filter.get(distortProperty) === '0'
    }

    function isFillMode() {
        return filter.get(fillProperty) === '1' && filter.get(distortProperty) !== '1'
    }

    function updateAspectRatio() {
        if (filter.get(fillProperty) === '1' && filter.get(distortProperty) === '0') {
            aspectRatio = producer.displayAspectRatio
        } else {
            aspectRatio = filterRect.width / Math.max(filterRect.height, 1)
        }
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

    function updateRotation(position) {
        if (blockUpdate) return
        if (position !== null) {
            filter.blockSignals = true
            if (position <= 0 && filter.animateIn > 0) {
                filter.set(rotationStartValue, rotationSlider.value)
            } else if (position >= filter.duration - 1 && filter.animateOut > 0) {
                filter.set(rotationEndValue, rotationSlider.value)
            } else {
                filter.set(rotationMiddleValue, rotationSlider.value)
            }
            filter.blockSignals = false
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(rotationProperty)
            rotationKeyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set(rotationProperty, filter.getDouble(rotationStartValue), 0)
                filter.set(rotationProperty, filter.getDouble(rotationMiddleValue), filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set(rotationProperty, filter.getDouble(rotationMiddleValue), filter.duration - filter.animateOut)
                filter.set(rotationProperty, filter.getDouble(rotationEndValue), filter.duration - 1)
            }
        } else if (!rotationKeyframesButton.checked) {
            filter.resetProperty(rotationProperty)
            filter.set(rotationProperty, filter.getDouble(rotationMiddleValue))
        } else if (position !== null) {
            filter.set(rotationProperty, rotationSlider.value, position)
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
        else if (align === 'right')
            rightRadioButton.checked = true
        align = filter.get(valignProperty)
        if (align === 'top')
            topRadioButton.checked = true
        else if (align === 'center' || align === 'middle')
            middleRadioButton.checked = true
        else if (align === 'bottom')
            bottomRadioButton.checked = true
        if (backgroundProperty) {
            var s = filter.get(backgroundProperty)
            if (s.substring(0, 6) === 'color:')
                bgColor.value = s.substring(6)
            else if  (s.substring(0, 7) === 'colour:')
                bgColor.value = s.substring(7)
        }
    }

    function isSimpleKeyframesActive() {
        var position = getPosition()
        return  position <= 0
            || (position >= (filter.animateIn - 1) &&
                position <= (filter.duration - filter.animateOut))
            ||  position >= (filter.duration - 1)
    }

    function setKeyframedControls() {
        var enabled = isSimpleKeyframesActive()
        var position = getPosition()
        var newValue = filter.getRect(rectProperty, position)
        if (filterRect !== newValue) {
            updateAspectRatio()
            if (isFillMode()) {
                // enforce the aspect ratio
                if (producer.displayAspectRatio > profile.aspectRatio) {
                    newValue.height = newValue.width / producer.displayAspectRatio
                } else {
                    newValue.width = newValue.height * producer.displayAspectRatio
                }
            }
            filterRect = newValue
            rectX.value = filterRect.x.toFixed()
            rectY.value = filterRect.y.toFixed()
            rectW.value = filterRect.width.toFixed()
            rectH.value = filterRect.height.toFixed()
            blockUpdate = true
            scaleSlider.update()
        }
        positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
        if (rotationProperty) {
            blockUpdate = true
            rotationSlider.value = filter.getDouble(rotationProperty, position)
            rotationSlider.enabled = enabled
            rotationKeyframesButton.checked = filter.keyframeCount(rotationProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
        }
        blockUpdate = false
        rectX.enabled = enabled
        rectY.enabled = enabled
        rectW.enabled = enabled
        rectH.enabled = enabled
    }

    function toggleKeyframes(isEnabled, parameter, value) {
        if (isEnabled) {
            blockUpdate = true
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                // Reset all of the simple keyframes.
                resetSimpleKeyframes()
                filter.animateIn = 0
                blockUpdate = false
                filter.animateOut = 0
            } else {
                filter.clearSimpleAnimation(parameter)
                blockUpdate = false
            }
            // Set this keyframe value.
            filter.set(parameter, value, getPosition())
        } else {
            // Remove keyframes and set the parameter.
            filter.resetProperty(parameter)
            filter.set(parameter, value)
        }
    }

    function scaleByWidth(value) {
        var centerX = filterRect.x + filterRect.width / 2
        var rightX = filterRect.x + filterRect.width
        filterRect.width = value
        if (centerRadioButton.checked) {
            filterRect.x = rectX.value = centerX - filterRect.width / 2
        } else if (rightRadioButton.checked) {
            filterRect.x = rectX.value = rightX - filterRect.width
        }
        var middleY = filterRect.y + filterRect.height / 2
        var bottomY = filterRect.y + filterRect.height
        filterRect.height = rectH.value = value / Math.max(aspectRatio, 1.0e-6)
        if (middleRadioButton.checked) {
            filterRect.y = rectY.value = middleY - filterRect.height / 2
        }
        else if (bottomRadioButton.checked) {
            filterRect.y = rectY.value = bottomY - filterRect.height
        }
        scaleSlider.update()
        setFilter(getPosition())
    }

    function scaleByHeight(value) {
        var middleY = filterRect.y + filterRect.height / 2
        var bottomY = filterRect.y + filterRect.height
        filterRect.height = value
        if (middleRadioButton.checked) {
            filterRect.y = rectY.value = middleY - filterRect.height / 2
        } else if (bottomRadioButton.checked) {
            filterRect.y = rectY.value = bottomY - filterRect.height
        }
        var centerX = filterRect.x + filterRect.width / 2
        var rightX = filterRect.x + filterRect.width
        filterRect.width = rectW.value = value * aspectRatio
        if (centerRadioButton.checked) {
            filterRect.x = rectX.value = centerX - filterRect.width / 2
        }
        else if (rightRadioButton.checked) {
            filterRect.x = rectX.value = rightX - filterRect.width
        }
        scaleSlider.update()
        setFilter(getPosition())
    }

    ButtonGroup { id: sizeGroup }
    ButtonGroup { id: halignGroup }
    ButtonGroup { id: valignGroup }

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
            parameters: [fillProperty, distortProperty, rectProperty, halignProperty, valignProperty]
            Layout.columnSpan: 5
            onBeforePresetLoaded: {
                filter.resetProperty(rectProperty)
                filter.resetProperty(rotationProperty)
            }
            onPresetSelected: {
                filter.removeRectPercents(rectProperty)
                setControls()
                setKeyframedControls()
                positionKeyframesButton.checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
                filter.blockSignals = true
                filter.set(middleValue, filter.getRect(rectProperty, filter.animateIn + 1))
                if (filter.animateIn > 0)
                    filter.set(startValue, filter.getRect(rectProperty, 0))
                if (filter.animateOut > 0)
                    filter.set(endValue, filter.getRect(rectProperty, filter.duration - 1))
                filter.set(rotationMiddleValue, filter.getDouble(rotationProperty, filter.animateIn + 1))
                if (filter.animateIn > 0)
                    filter.set(rotationStartValue, filter.getDouble(rotationProperty, 0))
                if (filter.animateOut > 0)
                    filter.set(rotationEndValue, filter.getRect(rotationProperty, filter.duration - 1))
                filter.blockSignals = false
            }
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
                    if (Math.abs(filterRect.x - value) >= 1) {
                        filterRect.x = value
                        setFilter(getPosition())
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
                    if (Math.abs(filterRect.y - value) >= 1) {
                        filterRect.y = value
                        setFilter(getPosition())
                    }
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filterRect.x = rectX.value = defaultRect.x
                filterRect.y = rectY.value = defaultRect.y
                setFilter(getPosition())
            }
        }
        ColumnLayout {
            Layout.rowSpan: 3
            height: positionKeyframesButton.height * 3
            SystemPalette { id: activePalette }
            Rectangle {
                color: activePalette.text
                width: 1
                height: positionKeyframesButton.height
                anchors.horizontalCenter: positionKeyframesButton.horizontalCenter
            }
            Shotcut.KeyframesButton {
                id: positionKeyframesButton
                onToggled: {
                    if (checked) {
                        blockUpdate = true
                        filter.clearSimpleAnimation(rectProperty)
                        blockUpdate = false
                        filter.set(rectProperty, filterRect, 1.0, getPosition())
                    } else {
                        filter.resetProperty(rectProperty)
                        filter.set(rectProperty, filterRect)
                    }
                    checked = filter.keyframeCount(rectProperty) > 0 && filter.animateIn <= 0 && filter.animateOut <= 0
                }
            }
            Rectangle {
                color: activePalette.text
                width: 1
                height: positionKeyframesButton.height
                anchors.horizontalCenter: positionKeyframesButton.horizontalCenter
            }
        }

        Label {
            text: qsTr('Size')
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
                from: 0
                to: 999999999
                onValueModified: {
                    if (Math.abs(filterRect.width - value) >= 1) {
                        if (isFillMode()) {
                            scaleByWidth(value)
                        } else {
                            filterRect.width = value
                            setFilter(getPosition())
                        }
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
                from: 0
                to: 999999999
                onValueModified: {
                    if (Math.abs(filterRect.height - value) >= 1) {
                        if (isFillMode()) {
                            scaleByHeight(value)
                        } else {
                            filterRect.height = value
                            setFilter(getPosition())
                        }
                    }
                }
            }
        }
        Shotcut.UndoButton {
            id: sizeUndoButton
            onClicked: {
                filterRect.width = rectW.value = defaultRect.width
                filterRect.height = rectH.value = defaultRect.height
                scaleSlider.update()
                setFilter(getPosition())
            }
        }

        Label {
            text: qsTr('Zoom')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: scaleSlider
            Layout.columnSpan: 3
            minimumValue: 0.1
            maximumValue: 1000
            decimals: 1
            suffix: ' %'
            function update() {
                blockUpdate = true
                scaleSlider.value = Math.min(filterRect.width / defaultRect.width * 100, scaleSlider.maximumValue)
                blockUpdate = false
            }
            onValueChanged: {
                if (!blockUpdate && Math.abs(value - filterRect.width / defaultRect.width * 100) > 0.1) {
                    var centerX = filterRect.x + filterRect.width / 2
                    var rightX = filterRect.x + filterRect.width
                    filterRect.width = rectW.value = defaultRect.width * value / 100
                    if (centerRadioButton.checked) {
                        filterRect.x = rectX.value = centerX - filterRect.width / 2
                    } else if (rightRadioButton.checked) {
                        filterRect.x = rectX.value = rightX - filterRect.width
                    }
                    var middleY = filterRect.y + filterRect.height / 2
                    var bottomY = filterRect.y + filterRect.height
                    filterRect.height = rectH.value = Math.round(filterRect.width / Math.max(aspectRatio, 1.0e-6))
                    if (middleRadioButton.checked) {
                        filterRect.y = rectY.value = middleY - filterRect.height / 2
                    } else if (bottomRadioButton.checked) {
                        filterRect.y = rectY.value = bottomY - filterRect.height
                    }
                    setFilter(getPosition())
                }
            }
        }
        Shotcut.UndoButton {
            enabled: scaleSlider.enabled
            onClicked: scaleSlider.value = 100
        }

        Label {
            text: qsTr('Size mode')
            Layout.alignment: Qt.AlignRight
        }
        RadioButton {
            id: fitRadioButton
            text: qsTr('Fit')
            ButtonGroup.group: sizeGroup
            onClicked: {
                filter.set(fillProperty, 0)
                filter.set(distortProperty, 0)
                updateAspectRatio()
            }
        }
        RadioButton {
            id: fillRadioButton
            text: qsTr('Fill')
            ButtonGroup.group: sizeGroup
            onClicked: {
                filter.set(fillProperty, 1)
                filter.set(distortProperty, 0)
                updateAspectRatio()
                // enforce the aspect ratio
                if (producer.displayAspectRatio > profile.aspectRatio) {
                    filterRect.height = rectH.value = filterRect.width / producer.displayAspectRatio
                } else {
                    filterRect.width = rectW.value = filterRect.height * producer.displayAspectRatio
                }
                setFilter(getPosition())
            }
        }
        RadioButton {
            id: distortRadioButton
            text: qsTr('Distort')
            ButtonGroup.group: sizeGroup
            onClicked: {
                filter.set(fillProperty, 1)
                filter.set(distortProperty, 1)
                updateAspectRatio()
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                fitRadioButton.checked = true
                filter.set(fillProperty, 0)
                filter.set(distortProperty, 0)
                updateAspectRatio()
            }
        }
        Item { width: 1 }

        Label {
            text: qsTr('Horizontal fit')
            Layout.alignment: Qt.AlignRight
        }
        RadioButton {
            id: leftRadioButton
            text: qsTr('Left')
            ButtonGroup.group: halignGroup
            onClicked: filter.set(halignProperty, 'left')
        }
        RadioButton {
            id: centerRadioButton
            text: qsTr('Center')
            ButtonGroup.group: halignGroup
            onClicked: filter.set(halignProperty, 'center')
        }
        RadioButton {
            id: rightRadioButton
            text: qsTr('Right')
            ButtonGroup.group: halignGroup
            onClicked: filter.set(halignProperty, 'right')
        }
        Shotcut.UndoButton {
            onClicked: {
                leftRadioButton.checked = true
                filter.set(halignProperty, 'left')
            }
        }
        Item { width: 1 }

        Label {
            text: qsTr('Vertical fit')
            Layout.alignment: Qt.AlignRight
        }
        RadioButton {
            id: topRadioButton
            text: qsTr('Top')
            ButtonGroup.group: valignGroup
            onClicked: filter.set(valignProperty, 'top')
        }
        RadioButton {
            id: middleRadioButton
            text: qsTr('Middle', 'Size and Position video filter')
            ButtonGroup.group: valignGroup
            onClicked: filter.set(valignProperty, 'middle')
        }
        RadioButton {
            id: bottomRadioButton
            text: qsTr('Bottom')
            ButtonGroup.group: valignGroup
            onClicked: filter.set(valignProperty, 'bottom')
        }
        Shotcut.UndoButton {
            onClicked: {
                topRadioButton.checked = true
                filter.set(valignProperty, 'top')
            }
        }
        Item { width: 1 }

        Label {
            text: qsTr('Rotation')
            Layout.alignment: Qt.AlignRight
            visible: !!rotationProperty
        }
        Shotcut.SliderSpinner {
            id: rotationSlider
            Layout.columnSpan: 3
            visible: !!rotationProperty
            minimumValue: -360
            maximumValue: 360
            decimals: 1
            suffix: qsTr(' °', 'degrees')
            onValueChanged: updateRotation(getPosition())
        }
        Shotcut.UndoButton {
            visible: !!rotationProperty
            onClicked: rotationSlider.value = 0
        }
        Shotcut.KeyframesButton {
            id: rotationKeyframesButton
            visible: !!rotationProperty
            onToggled: {
                toggleKeyframes(checked, rotationProperty, rotationSlider.value)
                setControls()
            }
        }

        Label {
            text: qsTr('Background color')
            Layout.alignment: Qt.AlignRight
            visible: bgColor.visible
        }
        Shotcut.ColorPicker {
            id: bgColor
            visible: !!backgroundProperty
            Layout.columnSpan: 3
            eyedropper: true
            alpha: true
            onValueChanged: filter.set(backgroundProperty, 'color:' + value)
        }
        Shotcut.UndoButton {
            visible: bgColor.visible
            onClicked: bgColor.value = '#00000000'
        }
        Item {
            width: 1
            visible: bgColor.visible
        }

        Item { Layout.fillHeight: true }
    }

    function updateSimpleKeyframes() {
        if (rotationProperty) {
            updateRotation()
        }
        setFilter(null)
    }

    Connections {
        target: filter
        onChanged: setKeyframedControls()
        onInChanged: updateSimpleKeyframes()
        onOutChanged: updateSimpleKeyframes()
        onAnimateInChanged: updateSimpleKeyframes()
        onAnimateOutChanged: updateSimpleKeyframes()
        onPropertyChanged: setKeyframedControls()
    }

    Connections {
        target: producer
        onPositionChanged: setKeyframedControls()
    }

    Connections {
        target: parameters
        onKeyframeAdded: {
            if (parameter == rotationProperty) {
                var n = filter.getDouble(parameter, position)
                filter.set(parameter, n, position)
            }
        }
    }
}
