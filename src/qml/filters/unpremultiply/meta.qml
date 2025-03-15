/*
 * Copyright (c) 2018-2025 Meltytech, LLC
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
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Unpremultiply Alpha')
    keywords: qsTr('disassociate associated straight', 'search keywords for the Unpremultiply Alpha video filter') + ' unpremultiply alpha #rgba'
    mlt_service: 'frei0r.premultiply'
    qml: 'ui.qml'
    icon: 'icon.webp'
}
