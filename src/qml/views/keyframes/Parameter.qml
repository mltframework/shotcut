/*
 * Copyright (c) 2018 Meltytech, LLC
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

import QtQuick 2.0
import QtQml.Models 2.1

Item {
    id: parameterRoot
    property alias model: keyframeDelegateModel.model
    property alias rootIndex: keyframeDelegateModel.rootIndex
    property bool isLocked: false
    property bool isCurrent: false
    property var selection: []

    signal clicked(var keyframe, var parameter)

    Repeater { id: keyframesRepeater; model: keyframeDelegateModel }

    DelegateModel {
        id: keyframeDelegateModel
        Keyframe {
            position: (filter.in - producer.in) + model.frame
            interpolation: model.interpolation
            value: model.name
            anchors.verticalCenter: parameterRoot.verticalCenter
            isSelected: isCurrent && selection.indexOf(index) !== -1

            onClicked: {
                parameterRoot.clicked(keyframe, parameterRoot)
                isCurrent = true
                selection = [keyframe.DelegateModel.itemsIndex]
            }
        }
    }
}
