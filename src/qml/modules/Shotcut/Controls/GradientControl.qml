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

import QtQuick 2.1
import QtQuick.Controls 2.12
import QtQuick.Dialogs
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0 as Shotcut

RowLayout {
    property var colors: []
    property alias spinnerVisible: gradientSpinner.visible
    property var _stopHandles: []

    signal gradientChanged()

    function _updateColorDisplay() {
        if (colors.length < 1) {
            gradientView.stops = [];
            for (var idx = 0; idx < _stopHandles.length; idx++) {
                _stopHandles[idx].destroy();
            }
            _stopHandles = [];
            return ;
        }
        var newStops = [];
        var stepSize = (colors.length > 1) ? 1 / (colors.length - 1) : 0;
        for (var idx = 0; idx < colors.length; idx++) {
            newStops.push(stopComponent.createObject(gradientView, {
                "position": stepSize * idx,
                "color": colors[idx]
            }));
        }
        gradientView.stops = newStops;
        for (var idx = 0; idx < _stopHandles.length; idx++) {
            _stopHandles[idx].destroy();
        }
        var newHandles = [];
        for (var idx = 0; idx < colors.length; idx++) {
            newHandles.push(stopHandle.createObject(gradientFrame, {
                "stopIndex": idx,
                "colorList": colors
            }));
        }
        _stopHandles = newHandles;
    }

    function _setStopColor(index, color) {
        colors[index] = color;
        _updateColorDisplay();
        gradientChanged();
    }

    function _setStopCount(count) {
        var oldLength = colors.length;
        var newLength = count;
        var newColors = Array(newLength);
        // Copy values from the old colors list
        for (var idx = 0; idx < Math.min(oldLength, newLength); idx++) {
            newColors[idx] = colors[idx];
        }
        // If the size is increased, copy the last color to fill in.
        for (var idx = oldLength; idx < newLength; idx++) {
            newColors[idx] = colors[colors.length - 1];
        }
        colors = newColors;
        _updateColorDisplay();
        gradientChanged();
    }

    Component.onCompleted: _updateColorDisplay()
    onColorsChanged: {
        gradientSpinner.value = colors.length;
        _updateColorDisplay();
    }

    Component {
        id: stopComponent

        GradientStop {
        }

    }

    Component {
        id: stopHandle

        Rectangle {
            id: handelRect

            property int stopIndex: 0
            property var colorList: []

            function chooseColor() {
                colorDialog.visible = true;
            }

            x: colorList.length == 1 ? (parent.width / 2) - (width / 2) : stopIndex == 0 ? 0 : stopIndex == colorList.length - 1 ? parent.width - width : stopIndex * (parent.width / (colorList.length - 1)) - (width / 2)
            y: 0
            color: typeof colorList[stopIndex] !== 'undefined' ? colorList[stopIndex] : "gray"
            border.color: "gray"
            border.width: 1
            width: 10
            height: parent.height
            radius: 2
            visible: colorList.length > 1

            ColorDialog {
                id: colorDialog

                title: qsTr("Color #%1").arg(stopIndex + 1)
                showAlphaChannel: true
                color: handelRect.color
                onAccepted: {
                    // Make a copy of the current value.
                    var myColor = Qt.darker(handelRect.color, 1);
                    // Ignore alpha when comparing.
                    myColor.a = currentColor.a;
                    // If the user changed color but left alpha at 0,
                    // they probably want to reset alpha to opaque.
                    console.log('currentColor.a=' + currentColor.a + ' currentColor=' + currentColor + ' myColor=' + myColor);
                    if (currentColor.a === 0 && (!Qt.colorEqual(currentColor, myColor) || (Qt.colorEqual(currentColor, 'transparent') && Qt.colorEqual(myColor, 'transparent'))))
                        currentColor.a = 1;

                    parent.parent._setStopColor(handelRect.stopIndex, String(currentColor));
                }
                modality: application.dialogModality
            }

            Shotcut.HoverTip {
                text: qsTr('Color: %1\nClick to change').arg(color)
            }

        }

    }

    Rectangle {
        id: gradientFrame

        Layout.fillWidth: true
        implicitHeight: 20
        implicitWidth: 200
        color: "transparent"

        Rectangle {
            id: gradientRect

            width: parent.width
            height: parent.height - 6
            y: 3
            border.color: "gray"
            border.width: 1
            radius: 4
        }

        Gradient {
            anchors.fill: gradientRect
            anchors.margins: gradientRect.border.width
            source: gradientRect
            start: Qt.point(0, 0)
            end: Qt.point(width, 0)
            gradient: Gradient {
                id: gradientView
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (_stopHandles.length == 0)
                    return ;

                var nearestStop = Math.floor(mouseX / (parent.width / _stopHandles.length));
                if (nearestStop >= _stopHandles.length)
                    nearestStop = _stopHandles.length - 1;

                _stopHandles[nearestStop].chooseColor();
            }
        }

    }

    Shotcut.DoubleSpinBox {
        id: gradientSpinner

        Layout.alignment: Qt.AlignVCenter
        width: 100
        value: colors.length
        from: 1
        to: 10
        stepSize: 1
        suffix: qsTr('colors', 'gradient control')
        onValueChanged: {
            if (value != colors.length)
                _setStopCount(value);

        }
    }

}
