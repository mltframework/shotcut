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
 
import QtQuick 2.12
import QtQuick.Controls 2.12

ComboBox {
    // workaround incorrect color of chosen combo item on Fusion theme
    SystemPalette { id: activePalette }
    palette.buttonText: activePalette.buttonText
    focusPolicy: Qt.NoFocus
    hoverEnabled: focusReason !== Qt.TabFocusReason && focusReason !== Qt.BacktabFocusReason
}
