/*
 * Copyright (c) 2019-2021 Meltytech, LLC
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

Shotcut.KeyframableFilter {
    property string amount: '0'
    property double amountDefault: 0.20
    
    keyframableParameters: [amount]
    startValues: [0.5]
    middleValues: [amountDefault]
    endValues: [0.5]

    width: 350
    height: 100

    Component.onCompleted: {
        if (filter.isNew) {
            // Property 1 sets the type of blur: [0, 0.33] = Exponential, [0.34, 0.66] = Lowpass, [0.67, 0.99] = Gaussian
            filter.set('1', '0.99')
            filter.set(amount, amountDefault)
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        amountSlider.value = filter.getDouble(amount, position) * amountSlider.maximumValue
        amountKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(amount) > 0
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        amountSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        updateFilter(amount, amountSlider.value / amountSlider.maximumValue, amountKeyframesButton, null)
        
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 4

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            id: preset
            parameters: [amount]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeyframes()
            }
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
            }
        }

        Label {
            text: qsTr('Amount')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: amountSlider
            minimumValue: 0
            maximumValue: 100.0
            stepSize: .1
            decimals: 1
            suffix: ' '
            onValueChanged: updateFilter(amount, amountSlider.value / amountSlider.maximumValue, amountKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: amountSlider.value = amountDefault * amountSlider.maximumValue
        }
        Shotcut.KeyframesButton {
            id: amountKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, amount, amountSlider.value / amountSlider.maximumValue)
            }
        }
        
        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        onChanged: setControls()
        onInChanged: updateSimpleKeyframes()
        onOutChanged: updateSimpleKeyframes()
        onAnimateInChanged: updateSimpleKeyframes()
        onAnimateOutChanged: updateSimpleKeyframes()
        onPropertyChanged: setControls()
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
