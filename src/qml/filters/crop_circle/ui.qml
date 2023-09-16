/*
 * Copyright (c) 2020-2023 Meltytech, LLC
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut
import org.shotcut.qml as Shotcut

Shotcut.KeyframableFilter {

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        slider.value = filter.getDouble('radius', position) * slider.maximumValue;
        colorSwatch.value = filter.getColor('color', position);
        radiusKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('radius') > 0;
        colorKeyframesButton.checked = filter.keyframeCount('color') > 0 && filter.animateIn <= 0 && filter.animateOut <= 0;
        blockUpdate = false;
        slider.enabled = position <= 0 || (position >= (filter.animateIn - 1) && position <= (filter.duration - filter.animateOut)) || position >= (filter.duration - 1);
    }

    function updateParameters() {
        updateFilter('radius', slider.value / slider.maximumValue, radiusKeyframesButton, null);
        updateFilter('color', colorSwatch.value, colorKeyframesButton, null);
    }

    keyframableParameters: ['radius', 'color']
    startValues: [0.5, Qt.rgba(0, 0, 0, 1)]
    middleValues: [0.5, Qt.rgba(0, 0, 0, 1)]
    endValues: [0.5, Qt.rgba(0, 0, 0, 1)]
    width: 400
    height: 100
    Component.onCompleted: {
        filter.set('circle', 1);
        if (filter.isNew) {
            // Set default parameter values
            filter.set('color', Qt.rgba(0, 0, 0, 1));
            filter.set('radius', 0.5);
        }
        setControls();
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Radius')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: slider

            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter('radius', value / maximumValue, radiusKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: slider.value = 50
        }

        Shotcut.KeyframesButton {
            id: radiusKeyframesButton

            onToggled: {
                toggleKeyframes(checked, 'radius', slider.value / slider.maximumValue);
                setControls();
            }
        }

        Label {
            text: qsTr('Color')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.ColorPicker {
                id: colorSwatch

                property bool isReady: false

                alpha: true
                Component.onCompleted: isReady = true
                onValueChanged: {
                    if (isReady) {
                        updateFilter('color', Qt.color(value), colorKeyframesButton, getPosition());
                        filter.set("disable", 0);
                    }
                }
                onPickStarted: {
                    filter.set('disable', 1);
                }
                onPickCancelled: filter.set('disable', 0)
            }

            Shotcut.Button {
                text: qsTr('Transparent')
                onClicked: colorSwatch.value = Qt.rgba(0, 0, 0, 0)
            }
        }

        Shotcut.UndoButton {
            onClicked: colorSwatch.value = Qt.rgba(0, 0, 0, 1)
        }

        Shotcut.KeyframesButton {
            id: colorKeyframesButton
            onToggled: toggleKeyframes(checked, 'color', Qt.color(colorSwatch.value))
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        function onChanged() {
            setControls();
        }

        function onInChanged() {
            updateParameters();
        }

        function onOutChanged() {
            updateParameters();
        }

        function onAnimateInChanged() {
            updateParameters();
        }

        function onAnimateOutChanged() {
            updateParameters();
        }

        function onPropertyChanged(name) {
            setControls();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            setControls();
        }

        target: producer
    }
}
