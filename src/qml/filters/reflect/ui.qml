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
    width: 200
    height: 50
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('mirror', 'horizontal');
        } else {
            var current = filter.get('mirror');
            for (var i = 0; i < modeModel.count; ++i) {
                if (modeModel.get(i).value === current) {
                    modeCombo.currentIndex = i;
                    break;
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label {
                text: qsTr('Mode')
            }

            Shotcut.ComboBox {
                id: modeCombo

                implicitWidth: 200
                textRole: 'text'
                onActivated: {
                    filter.set('mirror', modeModel.get(currentIndex).value);
                    filter.set('reverse', currentIndex % 2);
                }

                model: ListModel {
                    id: modeModel

                    ListElement {
                        text: qsTr('Right')
                        value: 'horizontal'
                    }

                    ListElement {
                        text: qsTr('Left')
                        value: 'horizontal'
                    }

                    ListElement {
                        text: qsTr('Bottom')
                        value: 'vertical'
                    }

                    ListElement {
                        text: qsTr('Top')
                        value: 'vertical'
                    }

                }

            }

            Shotcut.UndoButton {
                onClicked: {
                    filter.set('mirror', modeModel.get(0).value);
                    filter.set('reverse', 0);
                    modeCombo.currentIndex = 0;
                }
            }

        }

        Item {
            Layout.fillHeight: true
        }

    }

}
