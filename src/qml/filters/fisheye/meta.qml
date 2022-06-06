/*
 * Copyright (c) 2015 Meltytech, LLC
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
 
/*
 * Updated UI by hypov8 
 * v1 may2022 (shotcut 22.04.25)
 *
 * original source from forum user TwitchyMcJoe
 * https://forum.shotcut.org/t/using-defish0r-as-a-filter-code-for-a-useful-defisheye-effect/181
 *
 */

import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("DeFisheye(GoPro)")
    mlt_service: 'frei0r.defish0r'
    qml: 'ui.qml'
}
