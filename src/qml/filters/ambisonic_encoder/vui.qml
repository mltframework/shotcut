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
    property var keyframableParameters: ['azimuth', 'elevation']
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

    Component.onCompleted: translate.update()

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
            opacity: 0.5
            antialiasing: true
            transform: Translate {
                id: translate
                function update() {
                    let a = filter.getDouble('azimuth', getPosition());
                    x = (a - Math.trunc(a / 180) * 360) / 360 * video.rect.width;
                    a = -filter.getDouble('elevation', getPosition());
                    y = (a - Math.trunc(a / 180) * 360) / 360 * video.rect.height;
                }
            }
            Rectangle {
                anchors.centerIn: parent
                width: 15
                height: 15
                radius: width / 2
                opacity: enabled ? 0.5 : 0
                border.width: 2
                border.color: Qt.rgba(1, 1, 1, enabled ? 0.9 : 0)
                gradient: Gradient {
                    GradientStop {
                        position: mouseArea.pressed ? 0 : 1
                        color: 'black'
                    }

                    GradientStop {
                        position: mouseArea.pressed ? 1 : 0
                        color: 'white'
                    }
                }
            }
        }

        Item {
            x: video.rect.x + 0.5 * video.rect.width
            y: video.rect.y + 0.5 * video.rect.height
            opacity: videoItem.enabled ? 0.5 : 0

            Rectangle {
                color: 'transparent'
                border.width: 2
                border.color: 'black'
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: 1
                anchors.verticalCenterOffset: 1
                width: 0.5 * video.rect.width
                height: 0.5 * video.rect.height
            }
            Rectangle {
                color: 'transparent'
                border.width: 2
                border.color: 'white'
                anchors.centerIn: parent
                width: 0.5 * video.rect.width
                height: 0.5 * video.rect.height
            }
        }
    }

    MouseArea {
        id: mouseArea

        property double startAzimuth
        property double startElevation
        property int startX
        property int startY

        enabled: videoItem.enabled
        anchors.fill: parent
        onPressed: mouse => {
            startX = mouse.x;
            startY = mouse.y;
            startAzimuth = filter.getDouble('azimuth', getPosition());
            startElevation = filter.getDouble('elevation', getPosition());
        }
        onPositionChanged: mouse => {
            updateProperty('azimuth', clamp(startAzimuth + 0.5 * (mouse.x - startX), -360, 360));
            updateProperty('elevation', clamp(startElevation + 0.5 * (startY - mouse.y), -360, 360));
        }
    }

    Connections {
        function onChanged() {
            translate.update();
            videoItem.enabled = filter.get('disable') !== '1';
        }

        function onPropertyChanged(name) {
            translate.update();
        }

        target: filter
    }

    Connections {
        function onPositionChanged() {
            translate.update();
        }

        target: producer
    }
}
