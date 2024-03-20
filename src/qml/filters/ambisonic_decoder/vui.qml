/*
 * Copyright (c) 2024 Meltytech, LLC
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
import Shotcut.Controls as Shotcut

Shotcut.VuiBase {
    property var keyframableParameters: ['yaw', 'pitch', 'roll', 'zoom']
    property var startValues: []
    property var middleValues: []
    property var endValues: []
    property real zoom: (video.zoom > 0) ? video.zoom : 1

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

    Flickable {
        anchors.fill: parent
        interactive: false
        clip: true
        contentWidth: video.rect.width * zoom
        contentHeight: video.rect.height * zoom
        contentX: video.offset.x
        contentY: video.offset.y

        Item {
            id: videoItem

            x: video.rect.x + 0.5 * video.rect.width
            y: video.rect.y + 0.5 * video.rect.height
            scale: zoom
            opacity: 0.5
            antialiasing: true
            transform: [
                Scale {
                    id: scale
                    xScale: filter.getDouble('zoom', getPosition()) + 1.25
                    yScale: xScale
                },
                Rotation {
                    id: yawRotation
                    angle: filter.getDouble('yaw', getPosition())
                    axis {
                        x: 0
                        y: 1
                        z: 0
                    }
                },
                Rotation {
                    id: pitchRotation
                    angle: filter.getDouble('pitch', getPosition())
                    axis {
                        x: 1
                        y: 0
                        z: 0
                    }
                },
                Rotation {
                    id: rollRotation
                    angle: filter.getDouble('roll', getPosition())
                    axis {
                        x: 0
                        y: 0
                        z: 1
                    }
                }
            ]
            Rectangle {
                anchors.centerIn: parent
                width: 100
                height: 10
                radius: 5
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 0
                        color: 'yellow'
                    }
                    GradientStop {
                        position: 1
                        color: 'red'
                    }
                }
            }
            Rectangle {
                anchors.centerIn: parent
                width: 10
                height: 100
                radius: 5
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: 'blue'
                    }
                    GradientStop {
                        position: 1
                        color: 'green'
                    }
                }
            }
        }
    }

    MouseArea {
        id: mouseArea

        property double startYaw
        property double startPitch
        property double startRoll
        property int startX
        property int startY

        anchors.fill: parent
        onPressed: mouse => {
            startX = mouse.x;
            startY = mouse.y;
            startYaw = filter.getDouble('yaw', getPosition());
            startPitch = filter.getDouble('pitch', getPosition());
            startRoll = filter.getDouble('roll', getPosition());
        }
        onPositionChanged: mouse => {
            if (mouse.modifiers === Qt.ControlModifier) {
                updateProperty('roll', clamp(startRoll + 0.5 * (startY - mouse.y), -180, 180));
            } else {
                updateProperty('yaw', clamp(startYaw + 0.2 * (startX - mouse.x), -360, 360));
                updateProperty('pitch', clamp(startPitch + 0.1 * (mouse.y - startY), -180, 180));
            }
        }
        onWheel: wheel => {
            var zoom = filter.getDouble('zoom', getPosition());
            console.log('' + wheel.angleDelta.y / 8 / 360);
            updateProperty('zoom', clamp(zoom + wheel.angleDelta.y / 8 / 360, -1, 1));
        }
    }

    Connections {
        function onChanged() {
            scale.xScale = filter.getDouble('zoom', getPosition()) + 1.25;
            pitchRotation.angle = filter.getDouble('pitch', getPosition());
            rollRotation.angle = filter.getDouble('roll', getPosition());
            yawRotation.angle = filter.getDouble('yaw', getPosition());
            videoItem.enabled = filter.get('disable') !== '1';
        }

        target: filter
    }
}
