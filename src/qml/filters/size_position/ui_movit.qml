/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('rect', '0/50%:50%x50%')
            filter.savePreset(['rect'], qsTr('Bottom Left'))
            filter.set('rect', '50%/50%:50%x50%')
            filter.savePreset(['rect'], qsTr('Bottom Right'))
            filter.set('rect', '0/0:50%x50%')
            filter.savePreset(['rect'], qsTr('Top Left'))
            filter.set('rect', '50%/0:50%x50%')
            filter.savePreset(['rect'], qsTr('Top Right'))
            filter.set('rect', '0/0:100%x100%')
            filter.savePreset(['rect'])
        }
    }
    GridLayout {
        columns: 2
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Preset {
            parameters: ['rect']
            onPresetSelected: filter.changed()
        }

        Item { Layout.fillHeight: true }
    }
}
