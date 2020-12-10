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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0

KeyframableFilter {
    property string xcenter: '0'
    property string ycenter: '1'
    property string correctionnearcenter: '2'
    property string correctionnearedges: '3'
    
    property double xcenterDefault: 0.500
    property double ycenterDefault: 0.500
    property double correctionnearcenterDefault: 0.500
    property double correctionnearedgesDefault: 0.500
     
    keyframableParameters: [xcenter, ycenter, correctionnearcenter, correctionnearedges]
    startValues: [0.5, 0.5, 0.5, 0.5]
    middleValues: [xcenterDefault, ycenterDefault, correctionnearcenterDefault, correctionnearedgesDefault]
    endValues: [0.5, 0.5, 0.5, 0.5]

    width: 300
    height: 150

    Component.onCompleted: {
        filter.set('threads', 0)
        if (filter.isNew) {
            filter.set(xcenter, xcenterDefault)
            filter.set(ycenter, ycenterDefault)
            filter.set(correctionnearcenter, correctionnearcenterDefault)
            filter.set(correctionnearedges, correctionnearedgesDefault)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        xcenterSlider.value = filter.getDouble(xcenter, position) * xcenterSlider.maximumValue
        ycenterSlider.value = filter.getDouble(ycenter, position) * ycenterSlider.maximumValue
        correctionnearcenterSlider.value = filter.getDouble(correctionnearcenter, position) * correctionnearcenterSlider.maximumValue
        correctionnearedgesSlider.value = filter.getDouble(correctionnearedges, position) * correctionnearedgesSlider.maximumValue
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        xcenterSlider.enabled = ycenterSlider.enabled = correctionnearcenterSlider.enabled = correctionnearedgesSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        updateFilter(xcenter, xcenterSlider.value / xcenterSlider.maximumValue, xcenKeyframesButton)
        updateFilter(ycenter, ycenterSlider.value / ycenterSlider.maximumValue, ycentKeyframesButton)
        updateFilter(correctionnearcenter, correctionnearcenterSlider.value / correctionnearcenterSlider.maximumValue, cncenKeyframesButton)
        updateFilter(correctionnearedges, correctionnearedgesSlider.value / correctionnearedgesSlider.maximumValue, cnedgeKeyframesButton)
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
            parameters: [xcenter, ycenter, correctionnearcenter, correctionnearedges]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                filter.resetProperty(xcenter)
                filter.resetProperty(ycenter)
                filter.resetProperty(correctionnearcenter)
                filter.resetProperty(correctionnearedges)
            }
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
            }
        }

        Label {
            text: qsTr('X Center')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: xcenterSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(xcenter, xcenterSlider.value / xcenterSlider.maximumValue, xcenterKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: xcenterSlider.value = xcenterDefault * xcenterSlider.maximumValue
        }
        KeyframesButton {
            id: xcenterKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(xcenter) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, xcenter, xcenterSlider.value / xcenterSlider.maximumValue)
            }
        }

        Label {
            text: qsTr('Y Center')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: ycenterSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(ycenter, ycenterSlider.value / ycenterSlider.maximumValue, ycenterKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: ycenterSlider.value = ycenterDefault * ycenterSlider.maximumValue
        }
        KeyframesButton {
            id: ycenterKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(ycenter) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, ycenter, ycenterSlider.value / ycenterSlider.maximumValue)
            }
        }

Label {
            text: qsTr('Correction at Center')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: correctionnearcenterSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(correctionnearcenter, correctionnearcenterSlider.value / correctionnearcenterSlider.maximumValue, cncenKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: correctionnearcenterSlider.value = correctionnearcenterDefault * correctionnearcenterSlider.maximumValue
        }
        KeyframesButton {
            id: cncenKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(correctionnearcenter) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, correctionnearcenter, correctionnearcenterSlider.value / correctionnearcenterSlider.maximumValue)
            }
        }

        Label {
            text: qsTr('Correction at Edges')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: correctionnearedgesSlider
            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(correctionnearedges, correctionnearedgesSlider.value / correctionnearedgesSlider.maximumValue, cnedgeKeyframesButton, getPosition())
        }
        UndoButton {
            onClicked: correctionnearedgesSlider.value = correctionnearedgesDefault * correctionnearedgesSlider.maximumValue
        }
        KeyframesButton {
            id: cnedgeKeyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(correctionnearedges) > 0
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, correctionnearedges, correctionnearedgesSlider.value / correctionnearedgesSlider.maximumValue)
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
