/*
 * Copyright (c) 2019-2025 Meltytech, LLC
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

            textRole: 'text'
            onActivated: {
                filter.set(propertyName, comboItems.get(currentIndex).value);
            }

            model: ListModel {
                id: comboItems

                ListElement {
                    text: qsTr('Over')
                    value: 'normal'
                }

                ListElement {
                    text: qsTr('None')
                    value: ''
                }

                ListElement {
                    text: qsTr('Add')
                    value: 'add'
                }

                ListElement {
                    text: qsTr('Saturate')
                    value: 'saturate'
                }

                ListElement {
                    text: qsTr('Multiply')
                    value: 'multiply'
                }

                ListElement {
                    text: qsTr('Screen')
                    value: 'screen'
                }

                ListElement {
                    text: qsTr('Overlay')
                    value: 'overlay'
                }

                ListElement {
                    text: qsTr('Darken')
                    value: 'darken'
                }

                ListElement {
                    text: qsTr('Dodge')
                    value: 'colordodge'
                }

                ListElement {
                    text: qsTr('Burn')
                    value: 'colorburn'
                }

                ListElement {
                    text: qsTr('Hard Light')
                    value: 'hardlight'
                }

                ListElement {
                    text: qsTr('Soft Light')
                    value: 'softlight'
                }

                ListElement {
                    text: qsTr('Difference')
                    value: 'difference'
                }

                ListElement {
                    text: qsTr('Exclusion')
                    value: 'exclusion'
                }

                ListElement {
                    text: qsTr('HSL Hue')
                    value: 'hslhue'
                }

                ListElement {
                    text: qsTr('HSL Saturation')
                    value: 'hslsaturatation'
                }

                ListElement {
                    text: qsTr('HSL Color')
                    value: 'hslcolor'
                }

                ListElement {
                    text: qsTr('HSL Luminosity')
                    value: 'hslluminocity'
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
