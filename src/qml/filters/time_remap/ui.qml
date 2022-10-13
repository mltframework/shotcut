/*
 * Copyright (c) 2020-2022 Meltytech, LLC
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
import QtQuick.Dialogs
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    property bool blockUpdate: true

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function setControls() {
        if (blockUpdate)
            return ;

        var outPosition = getPosition();
        var inSeconds = filter.getDouble('map', outPosition);
        var inPosition = Math.round(inSeconds * profile.fps);
        blockUpdate = true;
        mapSpinner.value = inPosition;
        var current = filter.get('image_mode');
        for (var i = 0; i < imageModeModel.count; ++i) {
            if (imageModeModel.get(i).value === current) {
                modeCombo.currentIndex = i;
                break;
            }
        }
        pitchCheckbox.checked = filter.get('pitch') === '1';
        var speed = filter.getDouble('speed');
        speedLabel.text = Math.abs(speed).toFixed(5) + "x";
        if (speed < 0)
            directionLabel.text = qsTr('Reverse');
        else if (speed > 0)
            directionLabel.text = qsTr('Forward');
        else
            directionLabel.text = qsTr('Freeze');
        inputTimeLabel.text = qsTr("%L1s").arg(inSeconds);
        outputTimeLabel.text = qsTr("%L1s").arg(outPosition / profile.fps);
        blockUpdate = false;
    }

    width: 200
    height: 225
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set('map', 0, 0);
            filter.set('map', (filter.duration - 1) / profile.fps, filter.duration - 1);
            filter.set('image_mode', 'nearest');
            filter.set('pitch', 0);
            filter.savePreset(preset.parameters);
            application.showStatusMessage(qsTr('Hold %1 to drag a keyframe vertical only or %2 to drag horizontal only').arg(application.OS === 'OS X' ? '⌘' : 'Ctrl').arg(application.OS === 'OS X' ? '⌥' : 'Alt'));
        }
        blockUpdate = false;
        timer.restart();
    }

    Connections {
        function onInChanged() {
            timer.restart();
        }

        function onOutChanged() {
            timer.restart();
        }

        function onPropertyChanged() {
            timer.restart();
        }

        target: filter
    }

    Timer {
        id: timer

        interval: 200
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            setControls();
        }
    }

    Connections {
        function onPositionChanged() {
            timer.restart();
        }

        target: producer
    }

    Dialog {
        id: speedDialog

        property var direction: 'after'

        title: direction == 'after' ? qsTr('Set Speed After') : qsTr('Set Speed Before')
        standardButtons: StandardButton.Ok | StandardButton.Cancel
        modality: application.dialogModality
        width: 400
        height: 95
        onAccepted: {
            var currPosition = getPosition();
            var maxPosition = producer.out - producer.in;
            var currValue = filter.getDouble("map", currPosition);
            var maxValue = (producer.length - producer.in) / profile.fps;
            var newValue = currValue;
            var newPosition = currPosition;
            var lock = lockCombo.getLock();
            if (direction == 'after' && lock == 'out') {
                var nextPosition = filter.getNextKeyframePosition("map", currPosition);
                if (nextPosition > currPosition) {
                    var deltaTime = ((nextPosition - currPosition) / profile.fps) * speedSlider.value;
                    var nextValue = filter.getDouble("map", nextPosition);
                    newValue = nextValue - deltaTime;
                }
            } else if (direction == 'before' && lock == 'out') {
                var prevPosition = filter.getPrevKeyframePosition("map", currPosition);
                if (prevPosition < currPosition && prevPosition >= 0) {
                    var deltaTime = ((currPosition - prevPosition) / profile.fps) * speedSlider.value;
                    var prevValue = filter.getDouble("map", prevPosition);
                    newValue = prevValue + deltaTime;
                }
            } else if (direction == 'after' && lock == 'in') {
                // Lock the input value
                filter.set('map', currValue, currPosition);
                // Calculate the value of the next keyframe
                var nextPosition = filter.getNextKeyframePosition("map", currPosition);
                if (nextPosition == -1 || nextPosition > maxPosition)
                    nextPosition = maxPosition;

                if (nextPosition > currPosition) {
                    var deltaTime = ((nextPosition - currPosition) / profile.fps) * speedSlider.value;
                    newValue = currValue + deltaTime;
                    newPosition = nextPosition;
                }
                if (newValue > maxValue) {
                    var deltaPosition = ((maxValue - currValue) / speedSlider.value) * profile.fps;
                    newPosition = Math.floor(currPosition + deltaPosition);
                    var deltaTime = ((newPosition - currPosition) / profile.fps) * speedSlider.value;
                    newValue = currValue + deltaTime;
                }
                if (newValue < 0) {
                    var deltaPosition = ((0 - currValue) / speedSlider.value) * profile.fps;
                    newPosition = Math.floor(currPosition + deltaPosition);
                    var deltaTime = ((newPosition - currPosition) / profile.fps) * speedSlider.value;
                    newValue = currValue + deltaTime;
                }
            } else if (direction == 'before' && lock == 'in') {
                // Lock the input value
                filter.set('map', currValue, currPosition);
                // Calculate the value of the next keyframe
                var prevPosition = filter.getPrevKeyframePosition("map", currPosition);
                if (prevPosition < 0 || prevPosition == currPosition)
                    prevPosition = 0;

                if (prevPosition < currPosition && prevPosition >= 0) {
                    var deltaTime = ((currPosition - prevPosition) / profile.fps) * speedSlider.value;
                    newValue = currValue - deltaTime;
                    newPosition = prevPosition;
                }
                if (newValue > maxValue) {
                    var deltaPosition = Math.ceil(((maxValue - currValue) / speedSlider.value) * profile.fps);
                    newPosition = currPosition + deltaPosition;
                    var deltaTime = ((currPosition - newPosition) / profile.fps) * speedSlider.value;
                    newValue = currValue - deltaTime;
                }
                if (newValue < 0) {
                    var deltaPosition = Math.ceil(((0 - currValue) / speedSlider.value) * profile.fps);
                    newPosition = currPosition + deltaPosition;
                    var deltaTime = ((currPosition - newPosition) / profile.fps) * speedSlider.value;
                    newValue = currValue - deltaTime;
                }
            }
            if (newValue < 0)
                newValue = 0;

            if (newValue > maxValue)
                newValue = maxValue;

            if (newPosition < 0)
                newPosition = 0;

            if (newPosition > maxPosition)
                newPosition = maxPosition;

            filter.set('map', newValue, newPosition);
            timer.restart();
        }

        ColumnLayout {
            Shotcut.SliderSpinner {
                id: speedSlider

                Layout.bottomMargin: 8
                Layout.preferredWidth: 400
                value: 1
                minimumValue: -3
                maximumValue: 3
                decimals: 6
                stepSize: 0.1
                suffix: "x"
            }

            Shotcut.ComboBox {
                id: lockCombo

                function getLock() {
                    return lockModel.get(currentIndex).value;
                }

                Layout.preferredWidth: 400
                textRole: "text"
                currentIndex: 0

                model: ListModel {
                    id: lockModel

                    ListElement {
                        text: qsTr('Modify current mapping')
                        value: 'out'
                    }

                    ListElement {
                        text: qsTr('Lock current mapping')
                        value: 'in'
                    }

                }

            }

            Label {
                Layout.bottomMargin: 12
                Layout.preferredWidth: 400
                wrapMode: Text.WordWrap
                text: qsTr('"Modify current mapping" will modify the input time at the current position.\n' + '"Lock current mapping" will lock the input time at the current position and modify the value of an adjacent keyframe')
            }

        }

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
            parameters: ['map', 'image_mode', 'pitch']
            onBeforePresetLoaded: {
                filter.resetProperty(parameters[0]);
            }
            onPresetSelected: {
                timer.restart();
                mapKeyframesButton.checked = filter.keyframeCount(parameters[0]) > 0;
            }
        }

        Label {
            text: qsTr('Time')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Map the specified input time to the current time. Use keyframes to vary the time mappings over time.')
            }

        }

        Row {
            Shotcut.TimeSpinner {
                id: mapSpinner

                minimumValue: 0
                maximumValue: producer.length - producer.in
                saveButtonVisible: false
                undoButtonVisible: false
                onValueChanged: {
                    if (blockUpdate)
                        return ;

                    filter.set('map', mapSpinner.value / profile.fps, getPosition());
                    timer.restart();
                }
            }

            Shotcut.Button {
                anchors.verticalCenter: parent.verticalCenter
                icon.name: 'format-indent-less'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/format-indent-less.png'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    speedDialog.direction = 'before';
                    speedDialog.open();
                }

                Shotcut.HoverTip {
                    text: qsTr('Set the input time to achieve a desired speed before the current frame.')
                }

            }

            Shotcut.Button {
                anchors.verticalCenter: parent.verticalCenter
                icon.name: 'format-indent-more'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/format-indent-more.png'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    speedDialog.direction = 'after';
                    speedDialog.open();
                }

                Shotcut.HoverTip {
                    text: qsTr('Set the input time to achieve a desired speed after the current frame.')
                }

            }

        }

        Shotcut.UndoButton {
            onClicked: {
                filter.blockSignals = true;
                filter.resetProperty('map');
                filter.set('map', 0, 0);
                filter.set('map', filter.duration / profile.fps, filter.duration);
                filter.blockSignals = false;
                filter.changed('map');
                timer.restart();
            }
        }

        Label {
            text: qsTr('Image mode')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Use the specified image selection mode. Nearest will output the image that is nearest to the mapped time. Blend will blend all images that occur during the mapped time.')
            }

        }

        Shotcut.ComboBox {
            id: modeCombo

            Layout.columnSpan: parent.columns - 2
            implicitWidth: 180
            textRole: "text"
            onCurrentIndexChanged: {
                if (blockUpdate)
                    return ;

                filter.set('image_mode', imageModeModel.get(currentIndex).value);
            }

            model: ListModel {
                id: imageModeModel

                ListElement {
                    text: qsTr('Nearest')
                    value: 'nearest'
                }

                ListElement {
                    text: qsTr('Blend')
                    value: 'blend'
                }

            }

        }

        Shotcut.UndoButton {
            onClicked: modeCombo.currentIndex = 0
        }

        Label {
        }

        CheckBox {
            id: pitchCheckbox

            Layout.columnSpan: parent.columns - 2
            text: qsTr('Enable pitch compensation')
            onCheckedChanged: {
                if (blockUpdate)
                    return ;

                filter.set('pitch', checked);
            }
        }

        Shotcut.UndoButton {
            onClicked: pitchCheckbox.checked = false
        }

        Rectangle {
            Layout.columnSpan: parent.columns
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
            text: qsTr('Speed:')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The instantaneous speed of the last frame that was processed.')
            }

        }

        Label {
            id: speedLabel

            Layout.columnSpan: parent.columns - 1
        }

        Label {
            text: qsTr('Direction:')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The instantaneous direction of the last frame that was processed.')
            }

        }

        Label {
            id: directionLabel

            Layout.columnSpan: parent.columns - 1
        }

        Label {
            text: qsTr('Input Time:')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The original clip time of the frame.')
            }

        }

        Label {
            id: inputTimeLabel

            Layout.columnSpan: parent.columns - 1
        }

        Label {
            text: qsTr('Output Time:')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('The mapped output time for the input frame.')
            }

        }

        Label {
            id: outputTimeLabel

            Layout.columnSpan: parent.columns - 1
        }

        Item {
            Layout.fillHeight: true
        }

    }

}
