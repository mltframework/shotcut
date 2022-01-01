/*
 * Copyright (c) 2019-2022 Meltytech, LLC
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
    keyframableParameters: ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14']
    property var defaultValues: [0, 100, 0.5, 0, 100, 1, 0, 700, 1, 0, 5000, 1, 0, 15000, 0.5]
    startValues: [0, 100, 0.5, 0, 100, 1, 0, 700, 1, 0, 5000, 1, 0, 15000, 0.5]
    middleValues: [0, 100, 0.5, 0, 100, 1, 0, 700, 1, 0, 5000, 1, 0, 15000, 0.5]
    endValues: [0, 100, 0.5, 0, 100, 1, 0, 700, 1, 0, 5000, 1, 0, 15000, 0.5]

    width: 200
    height: 550

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('0', defaultValues[0])
            filter.set('1', defaultValues[1])
            filter.set('2', defaultValues[2])
            filter.set('3', defaultValues[3])
            filter.set('4', defaultValues[4])
            filter.set('5', defaultValues[5])
            filter.set('6', defaultValues[6])
            filter.set('7', defaultValues[7])
            filter.set('8', defaultValues[8])
            filter.set('9', defaultValues[9])
            filter.set('0', defaultValues[10])
            filter.set('11', defaultValues[11])
            filter.set('12', defaultValues[12])
            filter.set('13', defaultValues[13])
            filter.set('14', defaultValues[14])
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        slider0.value = filter.getDouble('0', position)
        slider1.value = filter.getDouble('1', position)
        slider2.value = filter.getDouble('2', position)
        slider3.value = filter.getDouble('3', position)
        slider4.value = filter.getDouble('4', position)
        slider5.value = filter.getDouble('5', position)
        slider6.value = filter.getDouble('6', position)
        slider7.value = filter.getDouble('7', position)
        slider8.value = filter.getDouble('8', position)
        slider9.value = filter.getDouble('9', position)
        slider10.value = filter.getDouble('10', position)
        slider11.value = filter.getDouble('11', position)
        slider12.value = filter.getDouble('12', position)
        slider13.value = filter.getDouble('13', position)
        slider14.value = filter.getDouble('14', position)
        keyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('0') > 0
        blockUpdate = false
        enableControls(isSimpleKeyframesActive())
    }

    function enableControls(enabled) {
        slider0.enabled = slider1.enabled = slider2.enabled = slider3.enabled =
            slider4.enabled = slider5.enabled = slider6.enabled = slider7.enabled =
            slider8.enabled = slider9.enabled = slider10.enabled = slider11.enabled =
            slider12.enabled = slider13.enabled = slider14.enabled = enabled
    }

    function updateSimpleKeyframes(position) {
        if (blockUpdate) return
        updateFilter('0', slider0.value, keyframesButton, position)
        updateFilter('1', slider1.value, keyframesButton, position)
        updateFilter('2', slider2.value, keyframesButton, position)
        updateFilter('3', slider3.value, keyframesButton, position)
        updateFilter('4', slider4.value, keyframesButton, position)
        updateFilter('5', slider5.value, keyframesButton, position)
        updateFilter('6', slider6.value, keyframesButton, position)
        updateFilter('7', slider7.value, keyframesButton, position)
        updateFilter('8', slider8.value, keyframesButton, position)
        updateFilter('9', slider9.value, keyframesButton, position)
        updateFilter('10', slider10.value, keyframesButton, position)
        updateFilter('11', slider11.value, keyframesButton, position)
        updateFilter('12', slider12.value, keyframesButton, position)
        updateFilter('13', slider13.value, keyframesButton, position)
        updateFilter('14', slider14.value, keyframesButton, position)
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
            parameters: keyframableParameters
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
            text: qsTr('<b>Low Shelf</b>')
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            Layout.rowSpan: 20
            height: (keyframesButton.height + 5) * Layout.rowSpan
            SystemPalette { id: activePalette }
            Rectangle {
                color: activePalette.text
                width: 1
                height: parent.height / 2
                Layout.alignment: Qt.AlignHCenter
            }
            Shotcut.KeyframesButton {
                id: keyframesButton
                onToggled: {
                    toggleKeyframes(checked, '0', slider0.value)
                    toggleKeyframes(checked, '1', slider1.value)
                    toggleKeyframes(checked, '2', slider2.value)
                    toggleKeyframes(checked, '3', slider3.value)
                    toggleKeyframes(checked, '4', slider4.value)
                    toggleKeyframes(checked, '5', slider5.value)
                    toggleKeyframes(checked, '6', slider6.value)
                    toggleKeyframes(checked, '7', slider7.value)
                    toggleKeyframes(checked, '8', slider8.value)
                    toggleKeyframes(checked, '9', slider9.value)
                    toggleKeyframes(checked, '10', slider10.value)
                    toggleKeyframes(checked, '11', slider11.value)
                    toggleKeyframes(checked, '12', slider12.value)
                    toggleKeyframes(checked, '13', slider13.value)
                    toggleKeyframes(checked, '14', slider14.value)
                    setControls()
                }
            }
            Rectangle {
                color: activePalette.text
                width: 1
                height: parent.height / 2
                Layout.alignment: Qt.AlignHCenter
            }
        }

        Label {
            text: qsTr('Gain')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider0
            minimumValue: -30
            maximumValue: 30
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider0.value = defaultValues[0]
        }

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider1
            minimumValue: 20
            maximumValue: 20000
            stepSize: 0.1
            decimals: 1
            suffix: ' Hz'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider1.value = defaultValues[1]
        }

        Label {
            text: qsTr('Slope')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider2
            minimumValue: 0
            maximumValue: 1
            stepSize: 0.1
            decimals: 1
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider2.value = defaultValues[2]
        }

        Label {
            text: qsTr('<b>Band 1</b>')
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            text: qsTr('Gain')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider3
            minimumValue: -30
            maximumValue: 30
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider3.value = defaultValues[3]
        }

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider4
            minimumValue: 20
            maximumValue: 20000
            stepSize: 0.1
            decimals: 1
            suffix: ' Hz'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider4.value = defaultValues[4]
        }

        Label {
            text: qsTr('Q', 'Parametric equalizer bandwidth')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider5
            minimumValue: 0
            maximumValue: 4
            stepSize: 0.1
            decimals: 1
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider5.value = defaultValues[5]
        }

        Label {
            text: qsTr('<b>Band 2</b>')
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            text: qsTr('Gain')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider6
            minimumValue: -30
            maximumValue: 30
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider6.value = defaultValues[6]
        }

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider7
            minimumValue: 20
            maximumValue: 20000
            stepSize: 0.1
            decimals: 1
            suffix: ' Hz'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider7.value = defaultValues[7]
        }

        Label {
            text: qsTr('Q', 'Parametric equalizer bandwidth')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider8
            minimumValue: 0
            maximumValue: 4
            stepSize: 0.1
            decimals: 1
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider8.value = defaultValues[8]
        }
        
        Label {
            text: qsTr('<b>Band 3</b>')
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            text: qsTr('Gain')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider9
            minimumValue: -30
            maximumValue: 30
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider9.value = defaultValues[9]
        }

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider10
            minimumValue: 20
            maximumValue: 20000
            stepSize: 0.1
            decimals: 1
            suffix: ' Hz'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider10.value = defaultValues[10]
        }

        Label {
            text: qsTr('Q', 'Parametric equalizer bandwidth')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider11
            minimumValue: 0
            maximumValue: 4
            stepSize: 0.1
            decimals: 1
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider11.value = defaultValues[11]
        }

        Label {
            text: qsTr('<b>High Shelf</b>')
            Layout.columnSpan: 3
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            text: qsTr('Gain')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider12
            minimumValue: -30
            maximumValue: 30
            stepSize: 0.1
            decimals: 1
            suffix: ' dB'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider12.value = defaultValues[12]
        }

        Label {
            text: qsTr('Frequency')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider13
            minimumValue: 20
            maximumValue: 20000
            stepSize: 0.1
            decimals: 1
            suffix: ' Hz'
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider13.value = defaultValues[13]
        }

        Label {
            text: qsTr('Slope')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider14
            minimumValue: 0
            maximumValue: 1
            stepSize: 0.1
            decimals: 1
            onValueChanged: updateSimpleKeyframes(getPosition())
        }
        Shotcut.UndoButton {
            onClicked: slider14.value = defaultValues[14]
        }

        Item {
            Layout.fillHeight: true;
        }
    }

    Connections {
        target: filter
        onInChanged: updateSimpleKeyframes(null)
        onOutChanged: updateSimpleKeyframes(null)
        onAnimateInChanged: updateSimpleKeyframes(null)
        onAnimateOutChanged: updateSimpleKeyframes(null)
        onPropertyChanged: setControls()
    }

    Connections {
        target: producer
        onPositionChanged: setControls()
    }
}
