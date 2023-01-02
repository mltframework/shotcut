/*
 * Copyright (c) 2021 Meltytech, LLC
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
import QtQml.Models 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    property string paramDisplay: '0'
    property string paramUseInput: '1'

    width: 200
    height: 50
    Component.onCompleted: {
        filter.set('threads', 0);
        if (filter.isNew) {
            filter.set(paramUseInput, 1);
            filter.set(paramDisplay, 0.21);
        }
        var current = filter.getDouble(paramDisplay);
        for (var i = 0; i < displayModel.count; ++i) {
            if (displayModel.get(i).value === current) {
                displayCombo.currentIndex = i;
                break;
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label {
                text: qsTr('Display')
            }

            Shotcut.ComboBox {
                id: displayCombo

                implicitWidth: 200
                textRole: 'text'
                onActivated: {
                    filter.set(paramDisplay, displayModel.get(currentIndex).value);
                }

                model: ListModel {
                    id: displayModel

                    ListElement {
                        text: qsTr('Gray Alpha')
                        value: 0.21
                    }

                    ListElement {
                        text: qsTr('Red & Gray Alpha')
                        value: 0.36
                    }

                    ListElement {
                        text: qsTr('Checkered Background')
                        value: 1
                    }

                    ListElement {
                        text: qsTr('Black Background')
                        value: 0.5
                    }

                    ListElement {
                        text: qsTr('Gray Background')
                        value: 0.64
                    }

                    ListElement {
                        text: qsTr('White Background')
                        value: 0.79
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
