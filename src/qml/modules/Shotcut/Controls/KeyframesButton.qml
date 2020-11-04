/*
 * Copyright (c) 2018-2020 Meltytech, LLC
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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import Shotcut.Controls 1.0 as Shotcut
import QtQuick.Dialogs 1.2

CheckBox {
    id: checkbox
    enabled: metadata !== null && metadata.keyframes.enabled
    opacity: enabled? 1.0 : 0.0

    signal toggled()

    style: CheckBoxStyle {
        background: Rectangle {
            implicitWidth: 20
            implicitHeight: 20
            radius: 3
            SystemPalette { id: activePalette }
            color: control.checked? activePalette.highlight : activePalette.button
            border.color: activePalette.shadow
            border.width: 1
        }
        indicator: ToolButton {
            x: 3
            implicitWidth: 16
            implicitHeight: 16
            iconName: 'chronometer'
            iconSource: 'qrc:///icons/oxygen/32x32/actions/chronometer.png'
        }
    }
    Shotcut.ToolTip { id: tooltip; text: qsTr('Use Keyframes for this parameter') }

    onClicked: {
        tooltip.isVisible = false
        if (!checked) {
           checked = true
           confirmDialog.visible = true
        } else {
            keyframes.show()
            keyframes.raise()
            toggled()
        }
        tooltip.isVisible = true
    }

    MessageDialog {
        id: confirmDialog
        visible: false
        modality: application.dialogModality
        icon: StandardIcon.Question
        title: qsTr("Confirm Removing Keyframes")
        text: qsTr('This will remove all keyframes for this parameter.<p>Do you still want to do this?')
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: {
            tooltip.isVisible = false
            checkbox.checked = false
            checkbox.toggled()
        }
        onNo: {
            tooltip.isVisible = false
            checkbox.checked = true
        }
    }
}
