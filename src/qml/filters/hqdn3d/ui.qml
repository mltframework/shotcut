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
    property string spatial: '0'
    property string temporal: '1'
    property double spatialDefault: 0.04
    property double temporalDefault: 0.06

    function setControls() {
        var position = getPosition();
        blockUpdate = true;
        spatialSlider.value = filter.getDouble(spatial, position) * spatialSlider.maximumValue;
        spatialKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(spatial) > 0;
        temporalSlider.value = filter.getDouble(temporal, position) * temporalSlider.maximumValue;
        temporalKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0 && filter.keyframeCount(temporal) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        spatialSlider.enabled = temporalSlider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        setControls();
        updateFilter(spatial, spatialSlider.value / spatialSlider.maximumValue, spatialKeyframesButton, null);
        updateFilter(temporal, temporalSlider.value / temporalSlider.maximumValue, temporalKeyframesButton, null);
    }

    keyframableParameters: [spatial, temporal]
    startValues: [0.5, 0.5]
    middleValues: [spatialDefault, temporalDefault]
    endValues: [0.5, 0.5]
    width: 350
    height: 100
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(spatial, spatialDefault);
            filter.set(temporal, temporalDefault);
            filter.savePreset(preset.parameters);
        }
        setControls();
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

            parameters: [spatial, temporal]
            Layout.columnSpan: 3
            onBeforePresetLoaded: {
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                setControls();
                initializeSimpleKeyframes();
            }
        }

        Label {
            text: qsTr('Spatial')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: spatialSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(spatial, spatialSlider.value / spatialSlider.maximumValue, spatialKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: spatialSlider.value = spatialDefault * spatialSlider.maximumValue
        }

        Shotcut.KeyframesButton {
            id: spatialKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, spatial, spatialSlider.value / spatialSlider.maximumValue);
            }
        }

        Label {
            text: qsTr('Temporal')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: temporalSlider

            minimumValue: 0
            maximumValue: 100
            stepSize: 0.1
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(temporal, temporalSlider.value / temporalSlider.maximumValue, temporalKeyframesButton, getPosition())
        }

        Shotcut.UndoButton {
            onClicked: temporalSlider.value = temporalDefault * temporalSlider.maximumValue
        }

        Shotcut.KeyframesButton {
            id: temporalKeyframesButton

            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, temporal, temporalSlider.value / temporalSlider.maximumValue);
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
