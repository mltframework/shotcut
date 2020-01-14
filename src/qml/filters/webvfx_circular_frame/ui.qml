/*
 * Copyright (c) 2013-2020 Meltytech, LLC
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
import QtQuick.Controls.Styles 1.1
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import Shotcut.Controls 1.0

Item {
    width: 400
    height: 100
    property bool blockUpdate: true
    property double startValue: 0.5
    property double middleValue: 0.5
    property double endValue: 0.5

    Component.onCompleted: {
        filter.set('transparent', 1)
        filter.set('mlt_resolution_scale', 1)
        if (filter.isNew) {
            filter.set('resource', filter.path + 'filter-demo.html')
            // Set default parameter values
            filter.set('color', '#ff000000')
            filter.set('radius', 0.5)
        } else {
            middleValue = filter.getDouble('radius', filter.animateIn)
            if (filter.animateIn > 0)
                startValue = filter.getDouble('radius', 0)
            if (filter.animateOut > 0)
                endValue = filter.getDouble('radius', filter.duration - 1)
        }
        setControls()
        colorSwatch.value = filter.get('color')
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0)
    }

    function setControls() {
        var position = getPosition()
        blockUpdate = true
        slider.value = filter.getDouble('radius', position) * slider.maximumValue
        blockUpdate = false
        slider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1)
    }

    function updateFilter(position) {
        if (blockUpdate) return
        var value = slider.value / 100.0

        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValue = value
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValue = value
            else
                middleValue = value
        }

        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty('radius')
            keyframesButton.checked = false
            if (filter.animateIn > 0) {
                filter.set('radius', startValue, 0)
                filter.set('radius', middleValue, filter.animateIn - 1)
            }
            if (filter.animateOut > 0) {
                filter.set('radius', middleValue, filter.duration - filter.animateOut)
                filter.set('radius', endValue, filter.duration - 1)
            }
        } else if (!keyframesButton.checked) {
            filter.resetProperty('radius')
            filter.set('radius', middleValue)
        } else if (position !== null) {
            filter.set('radius', value, position)
        }
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }
        SliderSpinner {
            id: slider
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(getPosition())
        }
        UndoButton {
            onClicked: slider.value = 50
        }
        KeyframesButton {
            id: keyframesButton
            checked: filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('radius') > 0
            onToggled: {
                var value = slider.value / 100.0
                if (checked) {
                    blockUpdate = true
                    filter.clearSimpleAnimation('radius')
                    blockUpdate = false
                    filter.set('radius', value, getPosition())
                } else {
                    filter.resetProperty('radius')
                    filter.set('radius', value)
                }
            }
        }


        Label {
            text: qsTr('Color')
            Layout.alignment: Qt.AlignRight
        }
        ColorPicker {
            id: colorSwatch
            alpha: true
            Layout.columnSpan: 2
            property bool isReady: false
            Component.onCompleted: isReady = true
            onValueChanged: {
                if (isReady) {
                    filter.set('color', '' + value)
                    filter.set("disable", 0);
                }
            }
            onPickStarted: {
                filter.set("disable", 1);
            }
            onPickCancelled: filter.set('disable', 0)
        }
        Item {}

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        onInChanged: updateFilter(null)
        onOutChanged: updateFilter(null)
        onAnimateInChanged: updateFilter(null)
        onAnimateOutChanged: updateFilter(null)
    }

    Connections {
        target: producer
        onPositionChanged: {
            if (filter.animateIn > 0 || filter.animateOut > 0) {
                setControls()
            } else {
                blockUpdate = true
                slider.value = filter.getDouble('radius', getPosition()) * slider.maximumValue
                blockUpdate = false
                slider.enabled = true
            }
        }
    }
}
