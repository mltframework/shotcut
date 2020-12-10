/*
 * Copyright (c) 2019-2020 Meltytech, LLC
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

KeyframableFilter {
    property string center: '0'
    property string linearwidth: '1'
    property string linearscalefactor: '2'
    property string nonlinearscalefactor: '3'
    
    property double centerDefault: 0.50
    property double linearwidthDefault: 0.00
    property double linearscalefactorDefault: 0.50
    property double nonlinearscalefactorDefault: 0.50
     
    keyframableParameters: [center, linearwidth, linearscalefactor, nonlinearscalefactor]
    startValues: [0.5, 0.5, 0.5, 0.5]
    middleValues: [centerDefault, linearwidthDefault, linearscalefactorDefault, nonlinearscalefactorDefault]
    endValues: [0.5, 0.5, 0.5, 0.5]

    width: 350
    height: 100

    Component.onCompleted: {
        filter.set('threads', 0)
        if (filter.isNew) {
            filter.set(center, centerDefault)
            filter.set(linearwidth, linearwidthDefault)
            filter.set(linearscalefactor, linearscalefactorDefault)
            filter.set(nonlinearscalefactor, nonlinearscalefactorDefault)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        centerSlider.value = filter.getDouble(center, position) * centerSlider.maximumValue
        linearwidthSlider.value = filter.getDouble(linearwidth, position) * linearwidthSlider.maximumValue
        linearscalefactorSlider.value = filter.getDouble(linearscalefactor, position) * linearscalefactorSlider.maximumValue
        nonlinearscalefactorSlider.value = filter.getDouble(nonlinearscalefactor, position) * nonlinearscalefactorSlider.maximumValue
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        centerSlider.enabled = linearwidthSlider.enabled = linearscalefactorSlider.enabled = nonlinearscalefactorSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        updateFilter(center, centerSlider.value / centerSlider.maximumValue, centerKeyframesButton)
        updateFilter(linearwidth, linearwidthSlider.value / linearwidthSlider.maximumValue, linwKeyframesButton)
        updateFilter(linearscalefactor, linearscalefactorSlider.value / linearscalefactorSlider.maximumValue, lsfKeyframesButton)
        updateFilter(nonlinearscalefactor, nonlinearscalefactorSlider.value / nonlinearscalefactorSlider.maximumValue, nlsfKeyframesButton)
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 4

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: [center, linearwidth, linearscalefactor, nonlinearscalefactor]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty(center)
                filter.resetProperty(linearwidth)
                filter.resetProperty(linearscalefactor)
                filter.resetProperty(nonlinearscalefactor)
            }
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
            }
        }

        Label {
            text: qsTr('Center')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('Horizontal center position of the linear area.') }
        }
        SliderSpinner {
            id: centerSlider
            minimumValue: 0
            maximumValue: 100.0
            stepSize: 0.1
            decimals: 1
            suffix: ' '
            onValueChanged: updateFilter(center, centerSlider.value / centerSlider.maximumValue, centerKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: centerSlider.value = centerDefault * centerSlider.maximumValue
        }
        KeyframesButton {
            id: centerKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(center) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, center, centerSlider.value / centerSlider.maximumValue)
            }
        }

        Label {
            text: qsTr('Linear width')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('Width of the linear area.') }
        }
        SliderSpinner {
            id: linearwidthSlider
            minimumValue: 0
            maximumValue: 100.0
            stepSize: 0.1
            decimals: 1
            suffix: ' '
            onValueChanged: updateFilter(linearwidth, linearwidthSlider.value / linearwidthSlider.maximumValue, linwKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: linearwidthSlider.value = linearwidthDefault * linearwidthSlider.maximumValue
        }
        KeyframesButton {
            id: linwKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(linearwidth) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, linearwidth, linearwidthSlider.value / linearwidthSlider.maximumValue)
            }
        }

Label {
            text: qsTr('Linear scale factor')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('Amount the linear area is scaled.') }
        }
        SliderSpinner {
            id: linearscalefactorSlider
            minimumValue: 0
            maximumValue: 100.0
            stepSize: 0.1
            decimals: 1
            suffix: ' '
            onValueChanged: updateFilter(linearscalefactor, linearscalefactorSlider.value / linearscalefactorSlider.maximumValue, lsfKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: linearscalefactorSlider.value = linearscalefactorDefault * linearscalefactorSlider.maximumValue
        }
        KeyframesButton {
            id: lsfKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(linearscalefactor) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, linearscalefactor, linearscalefactorSlider.value / linearscalefactorSlider.maximumValue)
            }
        }

        Label {
            text: qsTr('Non-Linear scale factor')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('Amount the outer left and outer right areas are scaled non linearly.') }
        }
        SliderSpinner {
            id: nonlinearscalefactorSlider
            minimumValue: 0
            maximumValue: 100.0
            stepSize: 0.1
            decimals: 1
            suffix: ' '
            onValueChanged: updateFilter(nonlinearscalefactor, nonlinearscalefactorSlider.value / nonlinearscalefactorSlider.maximumValue, nlsfKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: nonlinearscalefactorSlider.value = nonlinearscalefactorDefault * nonlinearscalefactorSlider.maximumValue
        }
        KeyframesButton {
            id: nlsfKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(nonlinearscalefactor) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, nonlinearscalefactor, nonlinearscalefactorSlider.value / nonlinearscalefactorSlider.maximumValue)
            }
        }
        
        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        onInChanged: updateSimpleKeyframes()
        onOutChanged: updateSimpleKeyframes()
        onAnimateInChanged: updateSimpleKeyframes()
        onAnimateOutChanged: updateSimpleKeyframes()
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
