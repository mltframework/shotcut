/*
 * Copyright (c) 2015-2020 Meltytech, LLC
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

SequentialAnimation {
    property alias target: firstAnim.target

    loops: Animation.Infinite
    NumberAnimation {
        id: firstAnim
        property: "opacity"
        from: 0.3
        to: 0
        duration: 800
    }
    NumberAnimation {
        target: firstAnim.target
        property: "opacity"
        from: 0
        to: 0.3
        duration: 800
    }
    PauseAnimation { duration: 300 }
}

