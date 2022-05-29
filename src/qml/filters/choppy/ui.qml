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

Shotcut.KeyframableFilter {
    property string amount: 'amount'
    property int amountDefault: 5

    keyframableParameters: [amount]
    startValues: [0]
    middleValues: [amountDefault]
    endValues: [0]

    width : 350
    height: 100

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(amount, amountDefault)
            filter.savePreset(preset.parameters)
        }
        setControls();
    }

    function setControls() {
        blockUpdate = true
        amountSlider.value = filter.getDouble(amount, getPosition())
        amountKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(amount) > 0
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        amountSlider.enabled = enabled
    }

    function updateSimpleKeyframes() {
        updateFilter(amount, amountSlider.value, amountKeyframesButton, null)
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
            Layout.columnSpan: parent.columns - 1
            parameters: [amount]
            onBeforePresetLoaded: {
                resetSimpleKeyframes()
            }
            onPresetSelected: {
                setControls()
                initializeSimpleKeyframes()
            }
        }

        Label {
            text: qsTr('Repeat')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: amountSlider
            minimumValue: 0
            maximumValue: Math.round(profile.fps)
            stepSize: 1
            suffix: qsTr(' frames')
            spinnerWidth: 110
            onValueChanged: updateFilter(amount, amountSlider.value, amountKeyframesButton, getPosition())
        }
        Shotcut.UndoButton {
            onClicked: amountSlider.value = amountDefault
        }
        Shotcut.KeyframesButton {
            id: amountKeyframesButton
            onToggled: {
                enableControls(true)
                toggleKeyframes(checked, amount, amountSlider.value)
            }
        }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        function onChanged() { setControls() }
        function onInChanged() { updateSimpleKeyframes() }
        function onOutChanged() { updateSimpleKeyframes() }
        function onAnimateInChanged() { updateSimpleKeyframes() }
        function onAnimateOutChanged() { updateSimpleKeyframes() }
        function onPropertyChanged(name) { setControls() }
    }

    Connections {
        target: producer
        function onPositionChanged() { setControls() }
    }
}
