/*
 * Copyright (c) 2018-2023 Meltytech, LLC
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
import QtQml.Models
import QtQuick
import org.shotcut.qml

Item {
    id: parameterRoot

    property alias rootIndex: keyframeDelegateModel.rootIndex
    property bool isCurve: false
    property double minimum: 0
    property double maximum: 1
    property bool isLocked: false

    signal clicked(var keyframe, var parameter)
    signal rightClicked(var keyframe, var parameter)

    function setMinMax(zoomHeight) {
        minimum = zoomHeight ? model.lowest : model.minimum;
        maximum = zoomHeight ? model.highest : model.maximum;
    }

    function getKeyframeCount() {
        return keyframesRepeater.count;
    }

    function getKeyframe(keyframeIndex) {
        if (keyframeIndex < keyframesRepeater.count)
            return keyframesRepeater.itemAt(keyframeIndex);
        else
            return null;
    }

    clip: true
    onMinimumChanged: canvas.requestPaint()
    onMaximumChanged: canvas.requestPaint()

    // When maximum == minimum only show the middle label
    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 3
        visible: isCurve && (maximum != minimum)
        opacity: 0.5
        color: activePalette.buttonText
        text: Number(maximum).toLocaleString(Qt.locale(), 'f', 1)
    }

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 3
        visible: isCurve
        opacity: 0.6
        color: activePalette.buttonText
        text: Number(minimum + ((maximum - minimum) / 2)).toLocaleString(Qt.locale(), 'f', 1)
    }

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 3
        visible: isCurve && (maximum != minimum)
        opacity: 0.6
        color: activePalette.buttonText
        text: Number(minimum).toLocaleString(Qt.locale(), 'f', 1)
    }

    Repeater {
        id: keyframesRepeater

        model: keyframeDelegateModel
        onCountChanged: canvas.requestPaint()
    }

    Canvas {
        id: canvas

        function distance(x0, y0, x1, y1) {
            return Math.sqrt(Math.pow(x1 - x0, 2) + Math.pow(y1 - y0, 2));
        }

        function drawCatmullRom(ctx, i, alpha, tension) {
            var g = i - 2 >= 0 ? i - 2 : i;
            var h = i - 1 >= 0 ? i - 1 : i;
            var j = i + 1 < keyframesRepeater.count ? i + 1 : i;
            var widthOffset = keyframesRepeater.itemAt(0).width / 2;
            var heightOffset = keyframesRepeater.itemAt(0).height / 2;
            var p = [{
                    "x": keyframesRepeater.itemAt(g).x + widthOffset,
                    "y": keyframesRepeater.itemAt(g).y + heightOffset
                }, {
                    "x": keyframesRepeater.itemAt(h).x + widthOffset,
                    "y": keyframesRepeater.itemAt(h).y + heightOffset
                }, {
                    "x": keyframesRepeater.itemAt(i).x + widthOffset,
                    "y": keyframesRepeater.itemAt(i).y + heightOffset
                }, {
                    "x": keyframesRepeater.itemAt(j).x + widthOffset,
                    "y": keyframesRepeater.itemAt(j).y + heightOffset
                }];
            if (p[0].x == p[1].x) {
                p[0].x -= 10000;
            }
            if (p[3].x == p[2].x) {
                p[3].x += 10000;
            }
            var step = 1 / keyframesRepeater.itemAt(i).width;
            for (var t = 0.0; t < 1.0; t += step) {
                var m1 = 0.0;
                var m2 = 0.0;
                var t12 = Math.pow(distance(p[1].x, p[1].y, p[2].x, p[2].y), alpha);
                if (tension > 0.0 || (p[1].y < p[0].y && p[1].y > p[2].y) || (p[1].y > p[0].y && p[1].y < p[2].y)) {
                    var t01 = Math.pow(distance(p[0].x, p[0].y, p[1].x, p[1].y), alpha);
                    m1 = Math.abs(tension) * (p[2].y - p[1].y + t12 * ((p[1].y - p[0].y) / t01 - (p[2].y - p[0].y) / (t01 + t12)));
                }
                if (tension > 0.0 || (p[2].y < p[1].y && p[2].y > p[3].y) || (p[2].y > p[1].y && p[2].y < p[3].y)) {
                    var t23 = Math.pow(distance(p[2].x, p[2].y, p[3].x, p[3].y), alpha);
                    m2 = Math.abs(tension) * (p[2].y - p[2].y + t12 * ((p[3].y - p[2].y) / t23 - (p[3].y - p[1].y) / (t12 + t23)));
                }
                var a = 2.0 * (p[1].y - p[2].y) + m1 + m2;
                var b = -3.0 * (p[1].y - p[2].y) - m1 - m1 - m2;
                var c = m1;
                var d = p[1].y;
                var xt = p[1].x + ((p[2].x - p[1].x) * t);
                var yt = a * t * t * t + b * t * t + c * t + d;
                context.lineTo(xt, yt);
            }
        }

        visible: isCurve
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            ctx.strokeStyle = activePalette.buttonText;
            ctx.lineWidth = 1;
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.beginPath();
            if (keyframesRepeater.count) {
                var widthOffset = keyframesRepeater.itemAt(0).width / 2;
                var heightOffset = keyframesRepeater.itemAt(0).height / 2;
                // Draw extent before first keyframe.
                ctx.moveTo(0, keyframesRepeater.itemAt(0).y + heightOffset);
                ctx.lineTo(keyframesRepeater.itemAt(0).x + widthOffset, keyframesRepeater.itemAt(0).y + heightOffset);
                // Draw lines between keyframes.
                for (var i = 1; i < keyframesRepeater.count; i++) {
                    switch (keyframesRepeater.itemAt(i - 1).interpolation) {
                    case KeyframesModel.LinearInterpolation:
                        ctx.lineTo(keyframesRepeater.itemAt(i).x + widthOffset, keyframesRepeater.itemAt(i).y + heightOffset);
                        break;
                    case KeyframesModel.SmoothLooseInterpolation:
                        drawCatmullRom(ctx, i, 0.0, 1.0);
                        ctx.moveTo(keyframesRepeater.itemAt(i).x + widthOffset, keyframesRepeater.itemAt(i).y + heightOffset);
                        break;
                    case KeyframesModel.SmoothNaturalInterpolation:
                        drawCatmullRom(ctx, i, 0.5, -1.0);
                        ctx.moveTo(keyframesRepeater.itemAt(i).x + widthOffset, keyframesRepeater.itemAt(i).y + heightOffset);
                        break;
                    case KeyframesModel.SmoothTightInterpolation:
                        drawCatmullRom(ctx, i, 0.5, 0.0);
                        ctx.moveTo(keyframesRepeater.itemAt(i).x + widthOffset, keyframesRepeater.itemAt(i).y + heightOffset);
                        break;
                    case KeyframesModel.DiscreteInterpolation:
                    default:
                        ctx.lineTo(keyframesRepeater.itemAt(i).x + widthOffset, keyframesRepeater.itemAt(i - 1).y + heightOffset);
                        ctx.moveTo(keyframesRepeater.itemAt(i).x + widthOffset, keyframesRepeater.itemAt(i).y + heightOffset);
                        break;
                    }
                }
                // Draw extent after last keyframe.
                ctx.lineTo(width, keyframesRepeater.itemAt(i - 1).y + heightOffset);
            }
            ctx.stroke();
        }
    }

    DelegateModel {
        id: keyframeDelegateModel

        model: parameters

        Keyframe {
            property int frame: model.frame ? model.frame : 0

            interpolation: model.interpolation ? model.interpolation : 0
            name: model.name ? model.name : ""
            value: model.value ? model.value : 0
            minDragX: (filter.in - producer.in + model.minimumFrame) * timeScale - width / 2
            maxDragX: (filter.in - producer.in + model.maximumFrame) * timeScale - width / 2
            isSelected: root.currentTrack === parameterRoot.DelegateModel.itemsIndex && root.selection.indexOf(index) !== -1
            isCurve: parameterRoot.isCurve
            minimum: parameterRoot.minimum
            maximum: parameterRoot.maximum
            parameterIndex: parameterRoot.DelegateModel.itemsIndex
            onClicked: keyframe => parameterRoot.clicked(keyframe, parameterRoot)
            onRightClicked: keyframe => parameterRoot.rightClicked(keyframe, parameterRoot)
            onInterpolationChanged: canvas.requestPaint()
            Component.onCompleted: {
                position = (filter.in - producer.in) + model.frame;
            }
            onFrameChanged: {
                position = (filter.in - producer.in) + model.frame;
            }
            onPositionChanged: canvas.requestPaint()
            onValueChanged: canvas.requestPaint()
        }
    }
}
