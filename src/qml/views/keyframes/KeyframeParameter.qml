/*
 * Copyright (c) 2018-2026 Meltytech, LLC
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
    property bool isLoop: false
    property real viewportStart: 0
    property real viewportEnd: width

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
    onIsLoopChanged: canvas.requestPaint()
    onViewportStartChanged: {
        if (isLoop)
            canvas.requestPaint();
    }
    onViewportEndChanged: {
        if (isLoop)
            canvas.requestPaint();
    }

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

        function drawCatmullRom(ctx, pts, i, alpha, tension) {
            var g = i - 2 >= 0 ? i - 2 : i - 1;
            var h = i - 1 >= 0 ? i - 1 : i;
            var j = i + 1 < pts.length ? i + 1 : i;
            var p = [
                {
                    "x": pts[g].x,
                    "y": pts[g].y
                },
                {
                    "x": pts[h].x,
                    "y": pts[h].y
                },
                {
                    "x": pts[i].x,
                    "y": pts[i].y
                },
                {
                    "x": pts[j].x,
                    "y": pts[j].y
                }
            ];
            if (p[0].x == p[1].x) {
                p[0].x -= 10000;
            }
            if (p[3].x == p[2].x) {
                p[3].x += 10000;
            }
            var step = 1 / keyframesRepeater.itemAt(0).width;
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
                    m2 = Math.abs(tension) * (p[2].y - p[1].y + t12 * ((p[3].y - p[2].y) / t23 - (p[3].y - p[1].y) / (t12 + t23)));
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

        function drawSinusoidal(ctx, pts, i, ease) {
            var x1 = pts[i - 1].x;
            var y1 = pts[i - 1].y;
            var x2 = pts[i].x;
            var y2 = pts[i].y;
            var xd = x2 - x1;
            var yd = y2 - y1;
            var step = 1 / keyframesRepeater.itemAt(0).width;
            for (var t = 0.0; t < 1.0; t += step) {
                var factor = 0;
                if (ease === 1) {
                    // In
                    factor = 1.0 - Math.cos((t * Math.PI) / 2);
                } else if (ease === -1) {
                    // Out
                    factor = Math.sin((t * Math.PI) / 2);
                } else {
                    // In/Out
                    factor = 0.5 * (1.0 - Math.cos(t * Math.PI));
                }
                context.lineTo(x1 + xd * t, y1 + yd * factor);
            }
        }

        function drawPow(ctx, pts, i, order, ease) {
            var x1 = pts[i - 1].x;
            var y1 = pts[i - 1].y;
            var x2 = pts[i].x;
            var y2 = pts[i].y;
            var xd = x2 - x1;
            var yd = y2 - y1;
            var step = 1 / keyframesRepeater.itemAt(0).width;
            for (var t = 0.0; t < 1.0; t += step) {
                var factor = 0;
                if (ease === 1) {
                    // In
                    factor = factor = Math.pow(t, order);
                } else if (ease === -1) {
                    // Out
                    factor = 1.0 - Math.pow(1.0 - t, order);
                } else {
                    // In/Out
                    if (t < 0.5) {
                        factor = Math.pow(2.0, order) * Math.pow(t, order) / 2.0;
                    } else {
                        factor = 1.0 - Math.pow(-2.0 * t + 2.0, order) / 2.0;
                    }
                }
                context.lineTo(x1 + xd * t, y1 + yd * factor);
            }
        }

        function drawExponential(ctx, pts, i, ease) {
            var x1 = pts[i - 1].x;
            var y1 = pts[i - 1].y;
            var x2 = pts[i].x;
            var y2 = pts[i].y;
            var xd = x2 - x1;
            var yd = y2 - y1;
            var step = 1 / keyframesRepeater.itemAt(0).width;
            for (var t = 0.0; t < 1.0; t += step) {
                var factor = 0;
                if (t === 0.0) {
                    factor = 0;
                } else if (t === 1.0) {
                    factor = 1.0;
                } else if (ease === 1) {
                    // In
                    factor = Math.pow(2.0, 10 * t - 10);
                } else if (ease === -1) {
                    // Out
                    factor = factor = 1.0 - Math.pow(2.0, -10.0 * t);
                } else {
                    // In/Out
                    if (t < 0.5) {
                        factor = Math.pow(2.0, 20.0 * t - 10.0) / 2.0;
                    } else {
                        factor = (2.0 - Math.pow(2.0, -20.0 * t + 10.0)) / 2.0;
                    }
                }
                context.lineTo(x1 + xd * t, y1 + yd * factor);
            }
        }

        function drawCircular(ctx, pts, i, ease) {
            var x1 = pts[i - 1].x;
            var y1 = pts[i - 1].y;
            var x2 = pts[i].x;
            var y2 = pts[i].y;
            var xd = x2 - x1;
            var yd = y2 - y1;
            var step = 1 / keyframesRepeater.itemAt(0).width;
            for (var t = 0.0; t < 1.0; t += step) {
                var factor = 0;
                if (ease === 1) {
                    // In
                    factor = 1.0 - Math.sqrt(1.0 - Math.pow(t, 2.0));
                } else if (ease === -1) {
                    // Out
                    factor = Math.sqrt(1.0 - Math.pow(t - 1.0, 2.0));
                } else {
                    // In/Out
                    if (t < 0.5) {
                        factor = 0.5 * (1 - Math.sqrt(1 - 4.0 * (t * t)));
                    } else {
                        factor = 0.5 * (Math.sqrt(-((2.0 * t) - 3.0) * ((2.0 * t) - 1.0)) + 1.0);
                    }
                }
                context.lineTo(x1 + xd * t, y1 + yd * factor);
            }
        }

        function drawBack(ctx, pts, i, ease) {
            var x1 = pts[i - 1].x;
            var y1 = pts[i - 1].y;
            var x2 = pts[i].x;
            var y2 = pts[i].y;
            var xd = x2 - x1;
            var yd = y2 - y1;
            var step = 1 / keyframesRepeater.itemAt(0).width;
            for (var t = 0.0; t < 1.0; t += step) {
                var factor = 0;
                if (ease === 1) {
                    // In
                    factor = t * t * t - t * Math.sin(t * Math.PI);
                } else if (ease === -1) {
                    // Out
                    var f = (1.0 - t);
                    factor = 1.0 - (f * f * f - f * Math.sin(f * Math.PI));
                } else {
                    // In/Out
                    if (t < 0.5) {
                        var f = 2.0 * t;
                        factor = 0.5 * (f * f * f - f * Math.sin(f * Math.PI));
                    } else {
                        var f = (1.0 - (2.0 * t - 1.0));
                        factor = 0.5 * (1.0 - (f * f * f - f * Math.sin(f * Math.PI))) + 0.5;
                    }
                }
                context.lineTo(x1 + xd * t, y1 + yd * factor);
            }
        }

        function drawElastic(ctx, pts, i, ease) {
            var x1 = pts[i - 1].x;
            var y1 = pts[i - 1].y;
            var x2 = pts[i].x;
            var y2 = pts[i].y;
            var xd = x2 - x1;
            var yd = y2 - y1;
            var step = 1.0 / xd;
            for (var t = 0.0; t < 1.0; t += step) {
                var factor = 0;
                if (ease === 1) {
                    // In
                    factor = Math.sin(13.0 * Math.PI * t / 2.0) * Math.pow(2.0, 10.0 * (t - 1.0));
                } else if (ease === -1) {
                    // Out
                    factor = Math.sin(-13.0 * Math.PI * (t + 1.0) / 2.0) * Math.pow(2.0, -10.0 * t) + 1.0;
                } else {
                    // In/Out
                    if (t < 0.5) {
                        factor = 0.5 * Math.sin(13.0 * Math.PI * (2.0 * t) / 2) * Math.pow(2.0, 10.0 * ((2.0 * t) - 1.0));
                    } else {
                        factor = 0.5 * (Math.sin(-13.0 * Math.PI * ((2.0 * t - 1.0) + 1.0) / 2.0) * Math.pow(2.0, -10.0 * (2.0 * t - 1.0)) + 2.0);
                    }
                }
                context.lineTo(x1 + xd * t, y1 + yd * factor);
            }
        }

        function bounce_factor(t, ease) {
            if (ease === 1) {
                // In
                return 1.0 - bounce_factor(1.0 - t, -1);
            } else if (ease === -1) {
                // Out
                if (t < 4.0 / 11.0) {
                    return (121 * t * t) / 16.0;
                } else if (t < 8.0 / 11.0) {
                    return (363.0 / 40.0 * t * t) - (99.0 / 10.0 * t) + 17.0 / 5.0;
                } else if (t < 9.0 / 10.0) {
                    return (4356.0 / 361.0 * t * t) - (35442.0 / 1805.0 * t) + 16061.0 / 1805.0;
                } else {
                    return (54.0 / 5.0 * t * t) - (513.0 / 25.0 * t) + 268.0 / 25.0;
                }
            } else {
                // In/Out
                if (t < 0.5) {
                    return 0.5 * bounce_factor(t * 2.0, 1);
                } else {
                    return 0.5 * bounce_factor(2.0 * t - 1.0, -1) + 0.5;
                }
            }
            return 0;
        }

        function drawBounce(ctx, pts, i, ease) {
            var x1 = pts[i - 1].x;
            var y1 = pts[i - 1].y;
            var x2 = pts[i].x;
            var y2 = pts[i].y;
            var xd = x2 - x1;
            var yd = y2 - y1;
            var step = 1.0 / xd;
            for (var t = 0.0; t < 1.0; t += step) {
                var factor = bounce_factor(t, ease);
                context.lineTo(x1 + xd * t, y1 + yd * factor);
            }
        }

        function drawSegment(ctx, pts, i) {
            switch (pts[i - 1].interpolation) {
            case KeyframesModel.DiscreteInterpolation:
                ctx.lineTo(pts[i].x, pts[i - 1].y);
                break;
            default:
            case KeyframesModel.LinearInterpolation:
                ctx.lineTo(pts[i].x, pts[i].y);
                break;
            case KeyframesModel.SmoothLooseInterpolation:
                drawCatmullRom(ctx, pts, i, 0.0, 1.0);
                break;
            case KeyframesModel.SmoothNaturalInterpolation:
                drawCatmullRom(ctx, pts, i, 0.5, -1.0);
                break;
            case KeyframesModel.SmoothTightInterpolation:
                drawCatmullRom(ctx, pts, i, 0.5, 0.0);
                break;
            case KeyframesModel.EaseInSinusoidal:
                drawSinusoidal(ctx, pts, i, 1);
                break;
            case KeyframesModel.EaseOutSinusoidal:
                drawSinusoidal(ctx, pts, i, -1);
                break;
            case KeyframesModel.EaseInOutSinusoidal:
                drawSinusoidal(ctx, pts, i, 0);
                break;
            case KeyframesModel.EaseInQuadratic:
                drawPow(ctx, pts, i, 2, 1);
                break;
            case KeyframesModel.EaseOutQuadratic:
                drawPow(ctx, pts, i, 2, -1);
                break;
            case KeyframesModel.EaseInOutQuadratic:
                drawPow(ctx, pts, i, 2, 0);
                break;
            case KeyframesModel.EaseInCubic:
                drawPow(ctx, pts, i, 3, 1);
                break;
            case KeyframesModel.EaseOutCubic:
                drawPow(ctx, pts, i, 3, -1);
                break;
            case KeyframesModel.EaseInOutCubic:
                drawPow(ctx, pts, i, 3, 0);
                break;
            case KeyframesModel.EaseInQuartic:
                drawPow(ctx, pts, i, 4, 1);
                break;
            case KeyframesModel.EaseOutQuartic:
                drawPow(ctx, pts, i, 4, -1);
                break;
            case KeyframesModel.EaseInOutQuartic:
                drawPow(ctx, pts, i, 4, 0);
                break;
            case KeyframesModel.EaseInQuintic:
                drawPow(ctx, pts, i, 5, 1);
                break;
            case KeyframesModel.EaseOutQuintic:
                drawPow(ctx, pts, i, 5, -1);
                break;
            case KeyframesModel.EaseInOutQuintic:
                drawPow(ctx, pts, i, 5, 0);
                break;
            case KeyframesModel.EaseInExponential:
                drawExponential(ctx, pts, i, 1);
                break;
            case KeyframesModel.EaseOutExponential:
                drawExponential(ctx, pts, i, -1);
                break;
            case KeyframesModel.EaseInOutExponential:
                drawExponential(ctx, pts, i, 0);
                break;
            case KeyframesModel.EaseInCircular:
                drawCircular(ctx, pts, i, 1);
                break;
            case KeyframesModel.EaseOutCircular:
                drawCircular(ctx, pts, i, -1);
                break;
            case KeyframesModel.EaseInOutCircular:
                drawCircular(ctx, pts, i, 0);
                break;
            case KeyframesModel.EaseInBack:
                drawBack(ctx, pts, i, 1);
                break;
            case KeyframesModel.EaseOutBack:
                drawBack(ctx, pts, i, -1);
                break;
            case KeyframesModel.EaseInOutBack:
                drawBack(ctx, pts, i, 0);
                break;
            case KeyframesModel.EaseInElastic:
                drawElastic(ctx, pts, i, 1);
                break;
            case KeyframesModel.EaseOutElastic:
                drawElastic(ctx, pts, i, -1);
                break;
            case KeyframesModel.EaseInOutElastic:
                drawElastic(ctx, pts, i, 0);
                break;
            case KeyframesModel.EaseInBounce:
                drawBounce(ctx, pts, i, 1);
                break;
            case KeyframesModel.EaseOutBounce:
                drawBounce(ctx, pts, i, -1);
                break;
            case KeyframesModel.EaseInOutBounce:
                drawBounce(ctx, pts, i, 0);
                break;
            }
            if (pts[i - 1].interpolation !== KeyframesModel.DiscreteInterpolation)
                ctx.lineTo(pts[i].x, pts[i].y);
            ctx.moveTo(pts[i].x, pts[i].y);
        }

        function keyframePoints() {
            const pts = [];
            const widthOffset = keyframesRepeater.itemAt(0).width / 2;
            const heightOffset = keyframesRepeater.itemAt(0).height / 2;
            for (let i = 0; i < keyframesRepeater.count; i++) {
                const item = keyframesRepeater.itemAt(i);
                pts.push({
                    "x": item.x + widthOffset,
                    "y": item.y + heightOffset,
                    "interpolation": item.interpolation
                });
            }
            return pts;
        }

        function cyclePoints(pts, offset, seamless) {
            const n = pts.length;
            const period = pts[n - 1].x - pts[0].x;
            const result = [];
            if (seamless)
                result.push({
                    "x": pts[n - 2].x + offset - period,
                    "y": pts[n - 2].y,
                    "interpolation": pts[n - 2].interpolation
                });
            for (let i = 0; i < n; i++)
                result.push({
                    "x": pts[i].x + offset,
                    "y": pts[i].y,
                    "interpolation": pts[i].interpolation
                });
            if (seamless)
                result.push({
                    "x": pts[1].x + offset + period,
                    "y": pts[1].y,
                    "interpolation": pts[1].interpolation
                });
            return result;
        }

        function drawGhostMarker(ctx, p) {
            const size = keyframesRepeater.itemAt(0).width / 2;
            ctx.beginPath();
            ctx.moveTo(p.x, p.y - size);
            ctx.lineTo(p.x + size, p.y);
            ctx.lineTo(p.x, p.y + size);
            ctx.lineTo(p.x - size, p.y);
            ctx.closePath();
            ctx.stroke();
        }

        anchors.fill: parent
        onWidthChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d");
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            if (!keyframesRepeater.count)
                return;
            ctx.strokeStyle = activePalette.buttonText;
            ctx.lineWidth = 1;
            const pts = keyframePoints();
            const n = pts.length;
            const loop = isLoop && n >= 2 && pts[n - 1].x > pts[0].x;
            const period = loop ? pts[n - 1].x - pts[0].x : 0;
            const seamless = loop && pts[0].y === pts[n - 1].y;
            const cycle = loop ? cyclePoints(pts, 0, seamless) : pts;
            const first = seamless ? 1 : 0;
            const last = first + n - 1;
            if (isCurve) {
                ctx.beginPath();
                ctx.moveTo(0, cycle[first].y);
                ctx.lineTo(cycle[first].x, cycle[first].y);
                for (let i = first + 1; i <= last; i++)
                    drawSegment(ctx, cycle, i);
                if (loop)
                    ctx.lineTo(cycle[last].x, cycle[first].y);
                else
                    ctx.lineTo(width, cycle[last].y);
                ctx.stroke();
            }
            if (loop) {
                ctx.globalAlpha = 0.4;
                if (isCurve && !seamless)
                    drawGhostMarker(ctx, {
                        "x": pts[n - 1].x,
                        "y": pts[0].y
                    });
                ctx.setLineDash([4, 3]);
                const markerRadius = keyframesRepeater.itemAt(0).width / 2;
                const firstGhostCycle = Math.max(1, Math.ceil((parameterRoot.viewportStart - markerRadius - pts[n - 1].x) / period));
                const maxGhostCycles = Math.max(1, Math.floor(10000 / n));
                for (let c = firstGhostCycle; pts[0].x + c * period < parameterRoot.viewportEnd && c - firstGhostCycle < maxGhostCycles; c++) {
                    const ghost = cyclePoints(pts, c * period, seamless);
                    if (isCurve) {
                        ctx.beginPath();
                        ctx.moveTo(ghost[first].x, ghost[first].y);
                        for (let i = first + 1; i <= last; i++)
                            drawSegment(ctx, ghost, i);
                        ctx.lineTo(ghost[last].x, ghost[first].y);
                        ctx.stroke();
                    }
                    ctx.setLineDash([]);
                    for (let i = first + 1; i <= last && ghost[i].x <= width; i++)
                        drawGhostMarker(ctx, ghost[i]);
                    if (isCurve && !seamless && ghost[last].x <= width)
                        drawGhostMarker(ctx, {
                            "x": ghost[last].x,
                            "y": ghost[first].y
                        });
                    ctx.setLineDash([4, 3]);
                }
                ctx.setLineDash([]);
                ctx.globalAlpha = 1;
            }
        }
    }

    DelegateModel {
        id: keyframeDelegateModel

        model: parameters

        Keyframe {
            property int frame: model.frame ? model.frame : 0

            interpolation: model.interpolation ? model.interpolation : 0
            prevInterpolation: model.prevInterpolation ? model.prevInterpolation : 0
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
