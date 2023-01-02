/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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

SizePositionUI {
    fillProperty: 'transition.fill'
    distortProperty: 'transition.distort'
    legacyRectProperty: 'transition.geometry'
    rectProperty: 'transition.rect'
    valignProperty: 'transition.valign'
    halignProperty: 'transition.halign'
    backgroundProperty: 'background'
    rotationProperty: 'transition.fix_rotate_x'
    Component.onCompleted: {
        if (filter.isNew)
            filter.set('transition.threads', 0);
    }
}
