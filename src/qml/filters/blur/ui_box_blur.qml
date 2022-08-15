/*
 * Copyright (c) 2022 Meltytech, LLC
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
    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        wslider.value = filter.getDouble('hradius', position);
        widthKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('hradius') > 0;
        hslider.value = filter.getDouble('vradius', position);
        heightKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount('vradius') > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        wslider.enabled = hslider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        updateFilter('hradius', wslider.value, widthKeyframesButton, null);
        updateFilter('vradius', hslider.value, heightKeyframesButton, null);
    }

    width: 200
    height: 50
    keyframableParameters: ['hradius', 'vradius']
    startValues: [0, 0]
    middleValues: [5, 5]
    endValues: [0, 0]
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('hradius', 5);
            filter.set('vradius', 5);
            filter.savePreset(preset.parameters);
        }
        setControls();
    }

    GridLayout {
        columns: 4
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            Layout.columnSpan: parent.columns - 1
            parameters: ['hradius', 'vradius']
            onBeforePresetLoaded: {
                filter.resetProperty('hradius');
                filter.resetProperty('vradius');
            }
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        Label {
            text: qsTr('Width')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: wslider

            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter('hradius', wslider.value, widthKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: wslider.value = 5
        }

        Shotcut.KeyframesButton {
            id: widthKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'hradius', wslider.value);
            }
        }

        Label {
            text: qsTr('Height')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: hslider

            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter('vradius', hslider.value, heightKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: hslider.value = 5
        }

        Shotcut.KeyframesButton {
            id: heightKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, 'vradius', hslider.value);
            }
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
            updateSimpleKeyframes();
        }

        function onOutChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateInChanged() {
            updateSimpleKeyframes();
        }

        function onAnimateOutChanged() {
            updateSimpleKeyframes();
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
