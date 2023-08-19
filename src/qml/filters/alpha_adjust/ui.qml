/*
 * Copyright (c) 2015-2023 Meltytech, LLC
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
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

Item {
    property string paramOperation: '2'
    property string paramThreshold: '3'
    property string paramAmount: '4'
    property string paramInvert: '5'

    width: 200
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(paramOperation, 0);
            filter.set(paramThreshold, 0.5);
            filter.set(paramAmount, 0.5);
        }
        var current = filter.getDouble(paramOperation);
        if (current === 0.8 && filter.getDouble(paramAmount) === 1.0) {
            modeCombo.currentIndex = operationModel.count - 1;
        } else {
            for (var i = 0; i < operationModel.count; ++i) {
                if (operationModel.get(i).value === current) {
                    modeCombo.currentIndex = i;
                    break;
                }
            }
        }
        sliderAmount.value = filter.getDouble(paramAmount) * 100;
        invertCheckbox.checked = filter.get(paramInvert) === '1';
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 3

        Label {
            text: qsTr('Mode')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: modeCombo

            implicitWidth: 180
            textRole: 'text'
            onActivated: {
                if (operationModel.get(currentIndex).value === -1) {
                    filter.set(paramOperation, 0.8);
                    sliderAmount.value = 100;
                } else {
                    if (filter.getDouble(paramOperation) === 0.8 && filter.getDouble(paramAmount) === 1.0)
                        sliderAmount.value = 50;
                    filter.set(paramOperation, operationModel.get(currentIndex).value);
                }
            }

            model: ListModel {
                id: operationModel

                ListElement {
                    text: qsTr('No Change')
                    value: 0
                }

                ListElement {
                    text: qsTr('Shave')
                    value: 0.2
                }

                ListElement {
                    text: qsTr('Shrink Hard')
                    value: 0.3
                }

                ListElement {
                    text: qsTr('Shrink Soft')
                    value: 0.4
                }

                ListElement {
                    text: qsTr('Grow Hard')
                    value: 0.6
                }

                ListElement {
                    text: qsTr('Grow Soft')
                    value: 0.7
                }

                ListElement {
                    text: qsTr('Threshold')
                    value: 0.8
                }

                ListElement {
                    text: qsTr('Blur')
                    value: 1
                }

                ListElement {
                    text: qsTr('Reset')
                    value: -1
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(paramOperation, operationModel.get(0).value);
                modeCombo.currentIndex = 0;
            }
        }

        Label {
            text: qsTr('Amount')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.SliderSpinner {
            id: sliderAmount

            enabled: operationModel.get(modeCombo.currentIndex).value !== -1
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            value: filter.getDouble(paramAmount) * 100
            onValueChanged: {
                filter.set(paramAmount, value / 100);
                filter.set(paramThreshold, value / 100);
            }
        }

        Shotcut.UndoButton {
            onClicked: sliderAmount.value = 50
        }

        Label {
        }

        CheckBox {
            id: invertCheckbox

            text: qsTr('Invert')
            onCheckedChanged: filter.set(paramInvert, checked)
        }

        Shotcut.UndoButton {
            onClicked: invertCheckbox.checked = false
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
