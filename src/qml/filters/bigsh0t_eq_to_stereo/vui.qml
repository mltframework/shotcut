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

import QtQuick 2.1
import Shotcut.Controls 1.0 as Shotcut

Shotcut.VuiBase {
    property var keyframableParameters: ['yaw', 'pitch', 'roll', 'fov']
    property var startValues: []
    property var middleValues: []
    property var endValues: []

    function clamp(x, min, max) {
        return Math.max(min, Math.min(max, x));
    }

    function getPosition() {
        return Math.max(producer.position - (filter.in - producer.in), 0);
    }

    function updateProperty(name, value) {
        var index = keyframableParameters.indexOf(name);
        var position = getPosition();
        if (position !== null) {
            if (position <= 0 && filter.animateIn > 0)
                startValues[index] = value;
            else if (position >= filter.duration - 1 && filter.animateOut > 0)
                endValues[index] = value;
            else
                middleValues[index] = value;
        }
        if (filter.animateIn > 0 || filter.animateOut > 0) {
            filter.resetProperty(name);
            if (filter.animateIn > 0) {
                filter.set(name, startValues[index], 0);
                filter.set(name, middleValues[index], filter.animateIn - 1);
            }
            if (filter.animateOut > 0) {
                filter.set(name, middleValues[index], filter.duration - filter.animateOut);
                filter.set(name, endValues[index], filter.duration - 1);
            }
        } else if (filter.keyframeCount(name) <= 0) {
            filter.resetProperty(name);
            filter.set(name, middleValues[index]);
        } else if (position !== null) {
            filter.set(name, value, position);
        }
    }

    Connections {
        function onChanged() {
            mouseArea.enabled = filter.get('disable') !== '1';
        }

        target: filter
    }

    MouseArea {
        id: mouseArea

        property double startYaw
        property double startPitch
        property double startRoll
        property int startX
        property int startY

        anchors.fill: parent
        onPressed: {
            startX = mouse.x;
            startY = mouse.y;
            startYaw = filter.getDouble('yaw', getPosition());
            startPitch = filter.getDouble('pitch', getPosition());
            startRoll = filter.getDouble('roll', getPosition());
        }
        onPositionChanged: {
            if (mouse.modifiers == Qt.ControlModifier) {
                updateProperty('roll', clamp(startRoll + 0.5 * (startY - mouse.y), -180, 180));
            } else {
                updateProperty('yaw', clamp(startYaw + 0.2 * (startX - mouse.x), -360, 360));
                updateProperty('pitch', clamp(startPitch + 0.1 * (mouse.y - startY), -180, 180));
            }
        }
        onWheel: {
            var fov = filter.getDouble('fov', getPosition());
            updateProperty('fov', clamp(fov + 0.03 * wheel.angleDelta.y, 0, 180));
        }
    }

}
