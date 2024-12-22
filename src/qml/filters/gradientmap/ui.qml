/*
 * Copyright (c) 2024 Meltytech, LLC
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

Item {
    property var defaultParameters: ['stop.1', 'stop.2', 'stop.3', 'stop.4', 'stop.5', 'stop.6', 'stop.7', 'stop.8', 'stop.9', 'stop.10', 'stop.11']
    property bool _disableUpdate: true
    property var stops: []
    property alias spinnerVisible: stopSpinner.visible
    property var _stopHandles: []

    signal gradientChanged

    function _parseStop(stop) {
        const exp = /^(#|0[xX])?([\da-fA-F]+).(.+)$/;
        let tokens = stop.match(exp);
        return {
            "color": (tokens[1] ?? "") + (tokens[2] ?? ""),
            "position": parseFloat(tokens[3] ?? "")
        };
    }

    function _compareStop(prev, next) {
        return prev.position - next.position;
    }

    function parseStops(stopList) {
        let newStops = Array(stopList.length);
        for (let idx = 0; idx < stopList.length; idx++) {
            newStops[idx] = _parseStop(stopList[idx]);
        }
        newStops.sort(_compareStop);
        stops = newStops;
    }

    function stringizeStops() {
        let stopList = Array(stops.length);
        for (let idx = 0; idx < stopList.length; idx++) {
            const stop = stops[idx];
            stopList[idx] = `${stop.color} ${stop.position}`;
        }
        return stopList;
    }

    function _updateColorDisplay() {
        if (stops.length < 1) {
            gradientView.stops = [];
            for (let idx = 0; idx < _stopHandles.length; idx++) {
                _stopHandles[idx].destroy();
            }
            _stopHandles = [];
            return;
        }
        stops.sort(_compareStop);
        let newStops = Array.from(stops, function (stop) {
            return stopComponent.createObject(gradientView, stop);
        });
        gradientView.stops = newStops;
        for (let idx = 0; idx < _stopHandles.length; idx++) {
            _stopHandles[idx].destroy();
        }
        let newHandles = [];
        for (let idx = 0; idx < stops.length; idx++) {
            newHandles.push(stopHandle.createObject(gradientFrame, {
                "stopIndex": idx,
                "stopList": stops,
                "prev": idx == 0 ? null : stops[idx - 1],
                "next": idx == stops.length - 1 ? null : stops[idx + 1]
            }));
        }
        _stopHandles = newHandles;
    }

    function _setStopColor(index, color, position) {
        stops[index].color = color;
        stops[index].position = position;
        if (stopSpinner.value - 1 !== index) {
            stopSpinner.value = index + 1;
        }
        _updateColorDisplay();
        gradientChanged();
    }

    function _setStopPosition(index, position) {
        gradientView.stops[index].position = position;
    }

    function _insertStop(index, stop) {
        let newStops = Array.from(stops);
        newStops.splice(index, 0, stop);
        stops = newStops;
        _updateColorDisplay();
        gradientChanged();
    }

    function _removeStop(index) {
        let newStops = Array.from(stops);
        newStops.splice(index, 1);
        stops = newStops;
        _updateColorDisplay();
        gradientChanged();
    }
    function setControls() {
        _disableUpdate = true;
        parseStops(filter.getGradient('stop'));
        _disableUpdate = false;
    }

    width: 350
    height: 180
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('stop.1', '#170400 0.0');
            filter.set('stop.2', '#6b4e3b 0.25');
            filter.set('stop.3', '#c5a989 0.65');
            filter.set('stop.4', '#dfcebf 0.85');
            filter.set('stop.5', '#f6e5d5 1.0');
            filter.savePreset(defaultParameters, qsTr('Sepia'));
            filter.set('stop.1', '#15222a 0.0');
            filter.set('stop.2', '#121121 0.15');
            filter.set('stop.3', '#0a46f2 0.35');
            filter.set('stop.4', '#f41397 0.53');
            filter.set('stop.5', '#ffa351 1.0');
            filter.savePreset(defaultParameters, qsTr('Thermal'));
            filter.set('stop.1', '#000000 0.0');
            filter.set('stop.2', '#ffffff 1.0');
            filter.resetProperty('stop.3');
            filter.resetProperty('stop.4');
            filter.resetProperty('stop.5');
            filter.savePreset(defaultParameters);
        }
        setControls();
        _updateColorDisplay();
    }
    onStopsChanged: _updateColorDisplay()

    Component {
        id: stopComponent

        GradientStop {}
    }

    Component {
        id: stopHandle

        Rectangle {
            id: handleRect

            property int stopIndex: 0
            property var stopList: []
            property var prev: null
            property var next: null

            function chooseColor() {
                colorDialog.open();
            }

            function getPosition() {
                return (x + width / 2) / parent.width;
            }

            x: (typeof stopList[stopIndex] != 'undefined' ? stopList[stopIndex].position : 0.0) * parent.width - width / 2
            y: 0
            color: typeof stopList[stopIndex] != 'undefined' ? stopList[stopIndex].color : "gray"
            border.color: "gray"
            border.width: 1
            width: 10
            height: parent.height
            radius: 2
            visible: stopList.length > 1

            onXChanged: {
                if (stopArea.drag.active) {
                    let position = getPosition();
                    _setStopPosition(stopIndex, position);
                    stopPosition.value = position * 100;
                }
            }

            Shotcut.ColorDialog {
                id: colorDialog

                title: qsTr("Color #%1").arg(stopIndex + 1)
                selectedColor: handleRect.color
                onAccepted: _setStopColor(handleRect.stopIndex, String(selectedColor), getPosition())
            }

            Shotcut.HoverTip {
                text: qsTr('Color: %1\nClick to select, drag to change position').arg(color)
            }

            MouseArea {
                id: stopArea
                anchors.fill: parent
                drag.target: parent
                drag.axis: Drag.XAxis
                drag.minimumX: ((parent.prev && parent.prev.position) || 0) * gradientFrame.width - width / 2
                drag.maximumX: ((parent.next && parent.next.position) || 1) * gradientFrame.width - width / 2
                drag.onActiveChanged: {
                    if (!drag.active) {
                        const position = getPosition();
                        stopPosition.value = position * 100;
                        _setStopColor(handleRect.stopIndex, String(handleRect.color), position);
                    } else if (stopSpinner.value - 1 !== handleRect.stopIndex) {
                        stopSpinner.value = handleRect.stopIndex + 1;
                    }
                }
                onClicked: {
                    if (stopSpinner.value - 1 !== handleRect.stopIndex) {
                        stopSpinner.value = handleRect.stopIndex + 1;
                    }
                }
            }
        }
    }

    GridLayout {
        id: gradientMap
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            id: preset
            parameters: defaultParameters
            Layout.columnSpan: 2
            onPresetSelected: setControls()
            onBeforePresetLoaded: {
                // Clear all gradient colors before loading the new values
                filter.setGradient('stop', []);
            }
        }

        Rectangle {
            id: gradientFrame

            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.rightMargin: 5
            implicitHeight: 20
            color: "transparent"

            Rectangle {
                id: gradientRect

                width: parent.width
                height: parent.height - 6
                y: 3
                border.color: "gray"
                border.width: 1
                radius: 4
                gradient: Gradient {
                    id: gradientView
                    orientation: Gradient.Horizontal
                }
            }
        }

        RowLayout {
            Button {
                enabled: stops.length <= 10
                icon.name: 'list-add'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/list-add.png'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    if (stops.length < 1) {
                        _insertStop(0, {
                            "color": "#000000",
                            "position": 0.5
                        });
                        return;
                    }
                    let index = stopSpinner.value - 1;
                    let prev;
                    let next;
                    if (index === 0) {
                        prev = stops[index];
                        if (prev.position <= 0.0) {
                            ++index;
                            next = stops[index];
                        } else {
                            _insertStop(0, {
                                "color": prev.color,
                                "position": 0.0
                            });
                            return;
                        }
                    } else {
                        prev = stops[index - 1];
                        next = stops[index];
                    }
                    if (typeof next === 'undefined') {
                        _insertStop(index, {
                            "color": prev.color,
                            "position": 1.0
                        });
                        return;
                    }
                    _insertStop(index, {
                        "color": next.color,
                        "position": (prev.position + next.position) / 2.0
                    });
                }
            }

            Button {
                enabled: stops.length > 0
                icon.name: 'list-remove'
                icon.source: 'qrc:///icons/oxygen/32x32/actions/list-remove.png'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: _removeStop(stopSpinner.value - 1)
            }
        }

        Label {
            text: qsTr('Stop')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Layout.columnSpan: 2

            Shotcut.DoubleSpinBox {
                id: stopSpinner

                Layout.minimumWidth: 50
                horizontalAlignment: Qt.AlignRight
                value: 1
                from: 1
                to: stops.length
                stepSize: 1
                onValueChanged: stopPosition.value = stops[value - 1].position * 100
            }

            Label {
                text: qsTr('Position')
            }

            Shotcut.DoubleSpinBox {
                id: stopPosition

                Layout.minimumWidth: 80
                horizontalAlignment: Qt.AlignRight
                value: typeof stops[stopSpinner.value - 1] != 'undefined' ? stops[stopSpinner.value - 1].position : 0
                decimals: 1
                stepSize: 0.1
                suffix: " %"
                onValueModified: _setStopColor(stopSpinner.value - 1, stops[stopSpinner.value - 1].color, value / 100)
            }

            Label {
                text: qsTr('Color')
            }

            Rectangle {
                id: stopColor

                implicitHeight: 20
                implicitWidth: height
                color: typeof stops[stopSpinner.value - 1] != 'undefined' ? stops[stopSpinner.value - 1].color : "gray"
                border.color: "gray"
                border.width: 1

                MouseArea {
                    anchors.fill: parent
                    onClicked: _stopHandles[stopSpinner.value - 1].chooseColor()
                }
            }
        }
        Item {
            Layout.fillHeight: true
        }
    }
}
