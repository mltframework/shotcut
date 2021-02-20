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

Item {
    width: 200
    height: 50
    property bool blockUpdate: false

    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('map', 0.0, 0)
            filter.set('map', filter.duration / profile.fps, filter.duration - 1)
            filter.set('image_mode', 'nearest')
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    Connections {
        target: filter
        onInChanged: {
            setControls()
        }
        onOutChanged: {
            setControls()
        }
        onPropertyChanged: {
            setControls()
        }
    }

    Timer {
        id: timer
        interval: 200
        repeat: false
        onTriggered: {
            setControls()
        }
    }

    Connections {
        target: producer
        onPositionChanged: {
            timer.start()
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        if (blockUpdate) return
        var position = getPosition()
        blockUpdate = true
        mapSpinner.value = filter.getDouble('map', position) * profile.fps
        var current = filter.get('image_mode')
        for (var i = 0; i < imageModeModel.count; ++i) {
            if (imageModeModel.get(i).value === current) {
                modeCombo.currentIndex = i
                break
            }
        }
        var speed = filter.getDouble('speed')
        speedLabel.text = Math.abs(speed).toFixed(5) + "x"
        if (speed < 0 ) {
            directionLabel.text = qsTr('Reverse')
        } else if (speed > 0 ) {
            directionLabel.text = qsTr('Forward')
        } else {
            directionLabel.text = qsTr('Freeze')
        }
        blockUpdate = false
    }

    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            id: preset
            Layout.columnSpan: parent.columns - 1
            parameters: ['map']
            onBeforePresetLoaded: {
                filter.resetProperty(parameters[0])
            }
            onPresetSelected: {
                setControls()
                mapKeyframesButton.checked = filter.keyframeCount(parameters[0]) > 0
            }
        }

        Label {
            text: qsTr('Time')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Map the specified input time to the current time. Use keyframes to vary the time mappings over time.') }
        }
        Shotcut.TimeSpinner {
            id: mapSpinner
            minimumValue: 0
            maximumValue: 1000000
            saveButtonVisible: false
            undoButtonVisible: false
            onValueChanged: {
                if (blockUpdate) return
                filter.set('map', mapSpinner.value / profile.fps, getPosition())
                timer.start()
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.resetProperty('map')
                filter.set('map', 0.0, 0)
                filter.set('map', filter.duration / profile.fps, filter.duration)
                setControls()
            }
        }

        Label {
            text: qsTr('Image mode')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Use the specified image selection mode. Nearest will output the image that is nearest to the mapped time. Blend will blend all images that occur during the mapped time.') }
        }
        Shotcut.ComboBox {
            id: modeCombo
            implicitWidth: 180
            model: ListModel {
                id: imageModeModel
                ListElement { text: qsTr('Nearest'); value: 'nearest' }
                ListElement { text: qsTr('Blend'); value: 'blend' }
            }
            textRole: "text"
            onCurrentIndexChanged: {
                if (blockUpdate) return
                filter.set('image_mode', imageModeModel.get(currentIndex).value)
            }
        }
        Shotcut.UndoButton {
            onClicked: modeCombo.currentIndex = 0
        }

        Rectangle {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.minimumHeight: 12
            color: 'transparent'
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 2
                radius: 2
                color: activePalette.text
            }
        }

        Label {
            text: qsTr('Speed')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('The instantaneous speed of the last frame that was processed.') }
        }
        Label {
            id: speedLabel
            Layout.columnSpan: 2
        }

        Label {
            text: qsTr('Direction')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('The instantaneous direction of the last frame that was processed.') }
        }
        Label {
            id: directionLabel
            Layout.columnSpan: 2
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
