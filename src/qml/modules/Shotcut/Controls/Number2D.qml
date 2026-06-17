/*
 * Copyright (c) 2026 Meltytech, LLC
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

// A pair of spinboxes for two-dimensional parameters (point or size).
// The caller sets valueX / valueY and connects to valuesModified to write back.
// Set decimals: 0 for integer parameters, > 0 for floating-point parameters.
RowLayout {
    id: root

    property real valueX: 0
    property real valueY: 0
    property int decimals: 0
    property real from: -99999
    property real to: 99999
    property real stepSize: 1
    property string labelFirst: 'X'
    property string labelSecond: 'Y'

    signal valuesModified

    spacing: 4

    onValueXChanged: {
        if (Math.abs(xBox.value - valueX) > 1e-9)
            xBox.value = valueX;
    }
    onValueYChanged: {
        if (Math.abs(yBox.value - valueY) > 1e-9)
            yBox.value = valueY;
    }

    Label {
        text: root.labelFirst
        textFormat: Text.PlainText
        verticalAlignment: Text.AlignVCenter
    }

    Shotcut.DoubleSpinBox {
        id: xBox

        Layout.fillWidth: true
        value: root.valueX
        decimals: root.decimals
        from: root.from
        to: root.to
        stepSize: root.stepSize
        onValueModified: {
            root.valueX = value;
            root.valuesModified();
        }
    }

    Label {
        text: root.labelSecond
        textFormat: Text.PlainText
        verticalAlignment: Text.AlignVCenter
    }

    Shotcut.DoubleSpinBox {
        id: yBox

        Layout.fillWidth: true
        value: root.valueY
        decimals: root.decimals
        from: root.from
        to: root.to
        stepSize: root.stepSize
        onValueModified: {
            root.valueY = value;
            root.valuesModified();
        }
    }
}
