/*
 * Copyright (c) 2025 Meltytech, LLC
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
    property bool channelMaskExists: filter.get("output_chmask") !== ''

    function updateFilter(channel) {
        let value = parseInt(filter.get('output_chmask'));
        value ^= 1 << channel;
        filter.set('output_chmask', value);
    }

    width: 350
    height: 10
    Component.onCompleted: {
        if (filter.isNew) {
            filter.set('output_chmask', 3);
        }

        const value = parseInt(filter.get('output_chmask'));
        leftToggle.checked = value & (1 << 0);
        rightToggle.checked = value & (1 << 1);
        centerToggle.checked = value & (1 << 2);
        lfToggle.checked = value & (1 << 3);
        leftSurroundToggle.checked = value & (1 << (application.audioChannels() > 4 ? 4 : 2));
        rightSurroundToggle.checked = value & (1 << (application.audioChannels() > 4 ? 5 : 3));
    }

    Row {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 10

        Label {
            text: qsTr('Target channels')
            visible: leftToggle.visible
        }

        Shotcut.ToggleButton {
            id: leftToggle

            text: qsTr('L', 'Left audio channel')
            implicitWidth: 22
            visible: channelMaskExists && application.audioChannels() > 2
            onClicked: updateFilter(0)

            Shotcut.HoverTip {
                text: qsTr('Left')
            }
        }

        Shotcut.ToggleButton {
            id: rightToggle

            text: qsTr('R', 'Right audio channel')
            implicitWidth: 22
            visible: channelMaskExists && application.audioChannels() > 2
            onClicked: updateFilter(1)

            Shotcut.HoverTip {
                text: qsTr('Right')
            }
        }

        Shotcut.ToggleButton {
            id: centerToggle

            text: qsTr('C', 'Center audio channel')
            implicitWidth: 22
            visible: channelMaskExists && application.audioChannels() > 4
            onClicked: updateFilter(2)

            Shotcut.HoverTip {
                text: qsTr('Center')
            }
        }

        Shotcut.ToggleButton {
            id: lfToggle

            text: qsTr('LF', 'Low Frequency audio channel')
            implicitWidth: 22
            visible: channelMaskExists && application.audioChannels() > 4
            onClicked: updateFilter(3)

            Shotcut.HoverTip {
                text: qsTr('Low Frequency')
            }
        }

        Shotcut.ToggleButton {
            id: leftSurroundToggle

            text: qsTr('Ls', 'Left surround audio channel')
            implicitWidth: 22
            visible: channelMaskExists && application.audioChannels() > 2
            onClicked: updateFilter(application.audioChannels() > 4 ? 4 : 2)

            Shotcut.HoverTip {
                text: qsTr('Left Surround')
            }
        }

        Shotcut.ToggleButton {
            id: rightSurroundToggle

            text: qsTr('Rs', 'Right surround audio channel')
            implicitWidth: 22
            visible: channelMaskExists && application.audioChannels() > 2
            onClicked: updateFilter(application.audioChannels() > 4 ? 5 : 3)

            Shotcut.HoverTip {
                text: qsTr('Right Surround')
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
