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
import Snapflow.Controls as Snapflow

RowLayout {
    property string channelMaskProperty: 'channel_mask'
    property bool channelMaskExists: filter.get(channelMaskProperty) !== ''

    function setChannelsControls() {
        const value = parseInt(filter.get(channelMaskProperty));
        leftToggle.checked = value & (1 << 0);
        rightToggle.checked = value & (1 << 1);
        centerToggle.checked = value & (1 << 2);
        lfToggle.checked = value & (1 << 3);
        leftSurroundToggle.checked = value & (1 << (application.audioChannels() > 4 ? 4 : 2));
        rightSurroundToggle.checked = value & (1 << (application.audioChannels() > 4 ? 5 : 3));
    }

    function updateChannelMask(channel) {
        let value = parseInt(filter.get(channelMaskProperty));
        value ^= 1 << channel;
        filter.set(channelMaskProperty, value);
    }

    spacing: 10

    Snapflow.ToggleButton {
        id: leftToggle

        text: qsTr('L', 'Left audio channel')
        implicitWidth: 22
        onClicked: updateChannelMask(0)

        Snapflow.HoverTip {
            text: qsTr('Left')
        }
    }

    Snapflow.ToggleButton {
        id: rightToggle

        text: qsTr('R', 'Right audio channel')
        implicitWidth: 22
        onClicked: updateChannelMask(1)

        Snapflow.HoverTip {
            text: qsTr('Right')
        }
    }

    Snapflow.ToggleButton {
        id: centerToggle

        text: qsTr('C', 'Center audio channel')
        visible: application.audioChannels() > 4
        implicitWidth: 22
        onClicked: updateChannelMask(2)

        Snapflow.HoverTip {
            text: qsTr('Center')
        }
    }

    Snapflow.ToggleButton {
        id: lfToggle

        text: qsTr('LF', 'Low Frequency audio channel')
        visible: application.audioChannels() > 4
        implicitWidth: 22
        onClicked: updateChannelMask(3)

        Snapflow.HoverTip {
            text: qsTr('Low Frequency')
        }
    }

    Snapflow.ToggleButton {
        id: leftSurroundToggle

        text: qsTr('Ls', 'Left surround audio channel')
        visible: application.audioChannels() > 2
        implicitWidth: 22
        onClicked: updateChannelMask(application.audioChannels() > 4 ? 4 : 2)

        Snapflow.HoverTip {
            text: qsTr('Left Surround')
        }
    }

    Snapflow.ToggleButton {
        id: rightSurroundToggle

        text: qsTr('Rs', 'Right surround audio channel')
        visible: application.audioChannels() > 2
        implicitWidth: 22
        onClicked: updateChannelMask(application.audioChannels() > 4 ? 5 : 3)

        Snapflow.HoverTip {
            text: qsTr('Right Surround')
        }
    }

    Item {
        Layout.fillWidth: true
    }

    Snapflow.UndoButton {
        onClicked: {
            filter.set(channelMaskProperty, -1);
            setChannelsControls();
        }
    }
}
