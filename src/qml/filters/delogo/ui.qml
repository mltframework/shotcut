/*
 * Copyright (c) 2018 Meltytech, LLC
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

import QtQuick 2.0
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import Shotcut.Controls 1.0

Item {
    // Center the box in the image by default
    property double wDefault: profile.width / 5
    property double hDefault: profile.height / 5
    property double xDefault: profile.width / 2 - wDefault / 2
    property double yDefault: profile.height / 2 - hDefault / 2

    property var defaultParameters: ["av.x", "av.y", "av.w", "av.h"]

    width: 200
    height: 125
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set("av.x", xDefault)
            filter.set("av.y", yDefault)
            filter.set("av.w", wDefault)
            filter.set("av.h", hDefault)
            filter.savePreset(defaultParameters)
        }
        setControls()
    }

    function setControls() {
        rectX.text = filter.get("av.x")
        rectY.text = filter.get("av.y")
        rectW.text = filter.get("av.w")
        rectH.text = filter.get("av.h")
    }

    function setFilter() {
        filter.set("av.x", parseInt(rectX.text))
        filter.set("av.y", parseInt(rectY.text))
        filter.set("av.w", parseInt(rectW.text))
        filter.set("av.h", parseInt(rectH.text))
    }

    GridLayout {
        columns: 5
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            id: preset
            parameters: defaultParameters
            Layout.columnSpan: 4
            onPresetSelected: setControls()
        }

        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            TextField {
                id: rectX
                horizontalAlignment: Qt.AlignRight
                validator: IntValidator {
                    bottom: 0
                }
                onEditingFinished: {
                    text = Math.min(parseInt(text), profile.width - parseInt(rectW.text) - 1)
                    text = Math.max(parseInt(text), 1)
                    if (filter.get("av.x") !== parseInt(text)) setFilter()
                }
            }
            Label { text: ',' }
            TextField {
                id: rectY
                horizontalAlignment: Qt.AlignRight
                validator: IntValidator {
                    bottom: 0
                }
                onEditingFinished: {
                    text = Math.min(parseInt(text), profile.height - parseInt(rectH.text) - 1)
                    text = Math.max(parseInt(text), 1)
                    if (filter.get("av.y") !== parseInt(text)) setFilter()
                }
            }
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Layout.columnSpan: 4
            TextField {
                id: rectW
                horizontalAlignment: Qt.AlignRight
                validator: IntValidator {
                    bottom: 0
                }
                onEditingFinished: {
                    text = Math.min(parseInt(text), profile.width - parseInt(rectX.text) - 1)
                    text = Math.max(parseInt(text), 1)
                    if (filter.get("av.w") !== parseInt(text)) setFilter()
                }
            }
            Label { text: 'x' }
            TextField {
                id: rectH
                horizontalAlignment: Qt.AlignRight
                validator: IntValidator {
                    bottom: 0
                }
                onEditingFinished: {
                    text = Math.min(parseInt(text), profile.height - parseInt(rectY.text) - 1)
                    text = Math.max(parseInt(text), 1)
                    if (filter.get("av.h") !== parseInt(text)) setFilter()
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: filter
        onChanged: {
            setControls()
        }
    }
}
