/*
 * Copyright (c) 2020 Meltytech, LLC
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

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    width: 200
    height: 50
    property bool blockUpdate: true

    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('map', 0.0, 0)
            filter.set('map', filter.duration / profile.fps, filter.duration)
            filter.set('image_mode', 'nearest')
            filter.savePreset(preset.parameters)
        }
        setControls()
    }

    Connections {
        target: filter
        onInChanged: updateFilter(null)
        onOutChanged: updateFilter(null)
    }

    Connections {
        target: producer
        onPositionChanged: {
            blockUpdate = true
            mapSlider.value = filter.getDouble('map', getPosition())
            blockUpdate = false
            mapSlider.enabled = true
        }
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        mapSlider.value = filter.getDouble('map', position)
        var current = filter.get('image_mode')
        for (var i = 0; i < imageModeModel.count; ++i) {
            if (imageModeModel.get(i).value === current) {
                modeCombo.currentIndex = i
                break
            }
        }
        blockUpdate = false
        mapSlider.enabled = position <= 0 || position >= (filter.duration - 1)
    }

    function updateFilter(position) {
        if (blockUpdate) return
        if (position !== null) {
            filter.set('map', mapSlider.value, position)
        }
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
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
            text: qsTr('Map')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('Map the specified input time to the current time. Use keyframes to vary the time mappings over time.') }
        }
        SliderSpinner {
            id: mapSlider
            minimumValue: 0
            maximumValue: filter.duration / profile.fps
            suffix: ' s'
            decimals: 2
            onValueChanged: updateFilter(getPosition())
        }
        UndoButton {
            onClicked: mapSlider.value = 0.0
        }
        KeyframesButton {
            id: mapKeyframesButton
            checked: filter.keyframeCount('map') > 0
            onToggled: {
                if (checked) {
                    filter.set('map', mapSlider.value, getPosition())
                } else {
                    filter.resetProperty('map')
                    filter.set('map', mapSlider.value)
                }
            }
        }

        Label {
            text: qsTr('Image Mode')
            Layout.alignment: Qt.AlignRight
            ToolTip { text: qsTr('Use the specified image selection mode. Nearest will output the image that is nearest to the mapped time. Blend will blend all images that occur during the mapped time.') }
        }
        ComboBox {
            id: modeCombo
            implicitWidth: 180
            model: ListModel {
                id: imageModeModel
                ListElement { text: qsTr('Nearest'); value: 'nearest' }
                ListElement { text: qsTr('Blend'); value: 'blend' }
            }
            onCurrentIndexChanged: {
                filter.set('image_mode', imageModeModel.get(currentIndex).value)
            }
        }
        UndoButton {
            onClicked: modeCombo.currentIndex = 0
        }
        Item {}

        Item {
            Layout.fillHeight: true
        }
    }
}
