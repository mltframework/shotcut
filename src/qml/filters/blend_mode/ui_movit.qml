/*
 * Copyright (c) 2025-2026 Meltytech, LLC
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
    property string propertyName: 'mode'

    width: 100
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            combo.currentIndex = 0;
            filter.set(propertyName, comboItems.get(0).value);
        } else {
            // Initialize parameter values
            var value = filter.get(propertyName);
            for (var i = 0; i < comboItems.count; i++) {
                if (value === comboItems.get(i).value) {
                    combo.currentIndex = i;
                    break;
                }
            }
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 4

        Label {
            text: qsTr('Blend mode')
        }

        Shotcut.ComboBox {
            id: combo

            implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted
            textRole: 'text'
            onActivated: {
                filter.set(propertyName, comboItems.get(currentIndex).value);
            }

            model: ListModel {
                id: comboItems

                ListElement {
                    text: qsTr('Source Over')
                    value: '0'
                }

                ListElement {
                    text: qsTr('Destination Over')
                    value: '1'
                }

                ListElement {
                    text: qsTr('Clear')
                    value: '2'
                }

                ListElement {
                    text: qsTr('Source')
                    value: '3'
                }

                ListElement {
                    text: qsTr('Destination')
                    value: '4'
                }

                ListElement {
                    text: qsTr('Source In')
                    value: '5'
                }

                ListElement {
                    text: qsTr('Destination In')
                    value: '6'
                }

                ListElement {
                    text: qsTr('Source Out')
                    value: '7'
                }

                ListElement {
                    text: qsTr('Destination Out')
                    value: '8'
                }

                ListElement {
                    text: qsTr('Source Atop')
                    value: '9'
                }

                ListElement {
                    text: qsTr('Destination Atop')
                    value: '10'
                }

                ListElement {
                    text: qsTr('XOR')
                    value: '11'
                }

                ListElement {
                    text: qsTr('Plus')
                    value: '12'
                }

                ListElement {
                    text: qsTr('Multiply')
                    value: '13'
                }

                ListElement {
                    text: qsTr('Screen')
                    value: '14'
                }

                ListElement {
                    text: qsTr('Overlay')
                    value: '15'
                }

                ListElement {
                    text: qsTr('Darken')
                    value: '16'
                }

                ListElement {
                    text: qsTr('Lighten')
                    value: '17'
                }

                ListElement {
                    text: qsTr('Color Dodge')
                    value: '18'
                }

                ListElement {
                    text: qsTr('Color Burn')
                    value: '19'
                }

                ListElement {
                    text: qsTr('Hard Light')
                    value: '20'
                }

                ListElement {
                    text: qsTr('Soft Light')
                    value: '21'
                }

                ListElement {
                    text: qsTr('Difference')
                    value: '22'
                }

                ListElement {
                    text: qsTr('Exclusion')
                    value: '23'
                }
            }
        }

        Shotcut.UndoButton {
            onClicked: {
                filter.set(propertyName, comboItems.get(0).value);
                combo.currentIndex = 0;
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
