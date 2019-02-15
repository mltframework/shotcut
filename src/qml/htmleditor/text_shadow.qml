/*
 * Copyright (c) 2013-2019 Meltytech, LLC
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
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import QtQuick.Controls.Styles 1.0

Item {
    id: root
    width: 260
    height: 170

    signal accepted(string outline)

    GridLayout {
        id: grid_layout1
        rows: 4
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr("Horizontal")
        }

        SpinBox {
            id: horizontalSpinbox
            minimumValue: -100
            maximumValue: 100
            stepSize: 1
            Layout.fillWidth: true
            focus: true
        }

        Label{ text: qsTr('pixels') }

        Label {
            text: qsTr("Vertical")
        }

        SpinBox {
            id: verticalSpinbox
            minimumValue: -100
            maximumValue: 100
            stepSize: 1
            Layout.fillWidth: true
            focus: true
        }

        Label{ text: qsTr('pixels') }

        Label {
            text: qsTr("Softness")
        }

        SpinBox {
            id: softnessSpinbox
            minimumValue: 0
            maximumValue: 100
            stepSize: 1
            Layout.fillWidth: true
            focus: true
        }

        Label{ text: qsTr('pixels') }

        Label {
            text: qsTr("Color")
        }

        Button {
            id: colorButton
            width: 20
            height: 20
            property var color: "black"
            style: ButtonStyle {
                background: Rectangle {
                    implicitWidth: 24
                    implicitHeight: 24
                    border.width: control.activeFocus ? 2 : 1
                    border.color: "gray"
                    radius: 3
                    color: colorButton.color
                }
            }
            onClicked: colorDialog.visible = true
        }

        Item {
            Layout.fillWidth: true
        }

        Item {
            Layout.columnSpan: 3
            Layout.fillHeight: true
        }

        Item {
            width: 10
        }

        Button {
            id: okButton
            text: qsTr("OK")
            isDefault: true
            onClicked: {
                if (horizontalSpinbox.value == 0 && verticalSpinbox.value == 0 && softnessSpinbox.value == 0)
                    root.accepted('')
                else
                    root.accepted(
                                horizontalSpinbox.value + 'px '+
                                verticalSpinbox.value + 'px ' +
                                softnessSpinbox.value + 'px ' +
                                colorButton.color)
                Qt.quit()
            }
        }

        Button {
            text: qsTr("Cancel")
            onClicked: Qt.quit()
        }
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Please choose a color")
        showAlphaChannel: false
        modality: Qt.ApplicationModal
        onAccepted: {
            colorButton.color = colorDialog.color
        }
    }
}
