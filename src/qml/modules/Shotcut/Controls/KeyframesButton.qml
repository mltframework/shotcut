/*
 * Copyright (c) 2018-2023 Meltytech, LLC
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
import QtQuick.Dialogs
import Shotcut.Controls as Shotcut

ToolButton {
    id: checkbox

    signal toggled

    enabled: metadata !== null && metadata.keyframes.enabled
    opacity: enabled ? 1 : 0
    padding: 2
    checkable: true
    hoverEnabled: true
    palette.buttonText: activePalette.buttonText
    icon.name: 'chronometer'
    icon.source: 'qrc:///icons/oxygen/32x32/actions/chronometer.png'
    onClicked: {
        if (!checked) {
            checked = true;
            confirmRemoveAdvancedDialog.open();
        } else {
            if (parameters.simpleKeyframesInUse()) {
                checked = false;
                confirmRemoveSimpleDialog.open();
            }
            if (checked) {
                application.showStatusMessage(qsTr('Hold %1 to drag a keyframe vertical only or %2 to drag horizontal only').arg(application.OS === 'OS X' ? '⌘' : 'Ctrl').arg(application.OS === 'OS X' ? '⌥' : 'Alt'));
                keyframes.show();
                keyframes.raise();
                toggled();
            }
        }
    }

    SystemPalette {
        id: activePalette
    }

    Shotcut.HoverTip {
        text: qsTr('Use Keyframes for this parameter')
    }

    Shotcut.MessageDialog {
        id: confirmRemoveAdvancedDialog

        title: qsTr("Confirm Removing Keyframes")
        text: qsTr('This will remove all keyframes for this parameter.<p>Do you still want to do this?')
        buttons: Shotcut.MessageDialog.Yes | Shotcut.MessageDialog.No
        onAccepted: {
            checkbox.checked = false;
            checkbox.toggled();
            parameters.reload();
        }
        onRejected: {
            checkbox.checked = true;
        }
    }

    Shotcut.MessageDialog {
        id: confirmRemoveSimpleDialog

        title: qsTr("Confirm Removing Simple Keyframes")
        text: qsTr('This will remove all simple keyframes for all parameters.<p>Simple keyframes will be converted to advanced keyframes.<p>Do you still want to do this?')
        buttons: Shotcut.MessageDialog.Yes | Shotcut.MessageDialog.No
        onAccepted: {
            checkbox.checked = true;
            parameters.removeSimpleKeyframes();
            parameters.reload();
        }
        onRejected: {
            checkbox.checked = false;
        }
    }

    background: Rectangle {
        implicitWidth: 20
        implicitHeight: 20
        radius: 3
        color: checked ? activePalette.highlight : activePalette.button
        border.color: activePalette.shadow
        border.width: 1
    }
}
