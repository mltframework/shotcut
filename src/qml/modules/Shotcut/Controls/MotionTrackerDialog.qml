/*
 * Copyright (c) 2023 Meltytech, LLC
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
import QtQuick.Window
import Shotcut.Controls as Shotcut
import org.shotcut.qml as Shotcut

Window {
    id: motionTrackerDialog

    signal accepted(int motionTrackerRow, string operation, int startFrame)
    signal reset

    SystemPalette {
        id: dialogPalette
        colorGroup: SystemPalette.Active
    }

    flags: Qt.Dialog
    color: dialogPalette.window
    modality: Qt.ApplicationModal
    title: qsTr('Load Keyframes from Motion Tracker')
    minimumWidth: 360
    minimumHeight: 160

    GridLayout {
        anchors.fill: parent
        anchors.margins: 10
        columns: 2

        Label {
            text: qsTr('Motion tracker')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: motionTrackerCombo

            implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted
            textRole: 'display'
            valueRole: 'display'
            currentIndex: indexOfValue(filter.get(motionTrackerModel.nameProperty))
            model: motionTrackerModel

            onActivated: {
                if (currentIndex > 0) {
                    filter.set(motionTrackerModel.nameProperty, currentText);
                }
            }
        }

        Label {
            text: qsTr('Adjust')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: trackingOperationCombo

            implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted
            textRole: 'text'
            valueRole: 'value'
            currentIndex: indexOfValue(filter.get(motionTrackerModel.operationProperty))
            model: [{
                    "text": '',
                    "value": ''
                }, {
                    "text": qsTr('Relative Position'),
                    "value": 'relativePos'
                }, {
                    "text": qsTr('Offset Position'),
                    "value": 'offsetPos'
                }, {
                    "text": qsTr('Absolute Position'),
                    "value": 'absPos'
                }, {
                    "text": qsTr('Size And Position'),
                    "value": 'absSizePos'
                },]

            onActivated: {
                if (currentIndex > 0) {
                    filter.set(motionTrackerModel.operationProperty, currentValue);
                }
            }
        }

        Item {
            width: 1
        }

        RowLayout {
            RadioButton {
                id: startRadioButton
                text: qsTr('From start')
                checked: true
            }
            RadioButton {
                id: currentRadioButton
                text: qsTr('Current position')
            }
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.columnSpan: parent.columns
            Layout.alignment: Qt.AlignRight
            focus: true

            Shotcut.Button {
                text: qsTr('Apply')
                onClicked: {
                    motionTrackerDialog.hide();
                    if (motionTrackerCombo.currentIndex > 0 && trackingOperationCombo.currentIndex > 0) {
                        let operation = trackingOperationCombo.currentValue;
                        let startFrame = currentRadioButton.checked ? getPosition() : 0;
                        accepted(motionTrackerCombo.currentIndex, operation, startFrame);
                    }
                }
            }

            Shotcut.Button {
                text: qsTr('Reset')
                onClicked: {
                    motionTrackerDialog.hide();
                    motionTrackerCombo.currentIndex = 0;
                    trackingOperationCombo.currentIndex = 0;
                    startRadioButton.checked = true;
                    reset();
                }
            }

            Shotcut.Button {
                text: qsTr('Cancel')
                onClicked: motionTrackerDialog.hide()
            }
        }
    }
}
