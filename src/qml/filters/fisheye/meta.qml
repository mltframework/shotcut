/*
 * Copyright (c) 2022-2025 Meltytech, LLC
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
    name: qsTr("Fisheye")
    keywords: qsTr('deform lens distort wide angle panoramic hemispherical', 'search keywords for the Fisheye video filter') + ' gopro fisheye #rgba'
    mlt_service: 'frei0r.defish0r'
    qml: 'ui.qml'
    icon: 'icon.webp'
}
