/*
 * Copyright (c) 2026 Meltytech, LLC
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

DropArea {
    Canvas {
        id: grid

        anchors.fill: parent
        opacity: 0.5
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            if (video.grid > 0) {
                var zoom = (video.zoom > 0) ? video.zoom : 1;
                var rectW = video.rect.width * zoom;
                var rectH = video.rect.height * zoom;
                var zoomOffsetX = (video.rect.width - rectW) / 2;
                var zoomOffsetY = (video.rect.height - rectH) / 2;
                var rectX = video.rect.x - video.offset.x + zoomOffsetX;
                var rectY = video.rect.y - video.offset.y + zoomOffsetY;
                var gridSizeX = (video.grid > 10000) ? rectW / (profile.width / (video.grid - 10000)) : rectW / video.grid;
                var gridSizeY = (video.grid > 10000) ? rectH / (profile.height / (video.grid - 10000)) : rectH / video.grid;
                var gridOffsetX = rectX % gridSizeX;
                var gridOffsetY = rectY % gridSizeY;
                if (video.grid <= 10000 || (video.grid >= 20001 && video.grid <= 29999)) {
                    // Black shadow grid (offset by 1)
                    ctx.lineWidth = 1;
                    ctx.strokeStyle = "#000000";
                    ctx.beginPath();
                    if (video.grid === 8090) {
                        // 80/90% Safe Areas
                        ctx.rect(0.1 * rectW + rectX + 2, 0.1 * rectH + rectY + 2, 0.8 * rectW - 1, 0.8 * rectH - 1);
                        ctx.rect(0.05 * rectW + rectX + 2, 0.05 * rectH + rectY + 2, 0.9 * rectW - 1, 0.9 * rectH - 1);
                    } else if (video.grid == 95) {
                        // EBU R95 Safe Areas
                        ctx.rect(0.05 * rectW + rectX + 2, 0.05 * rectH + rectY + 2, 0.9 * rectW - 1, 0.9 * rectH - 1);
                        ctx.rect(0.035 * rectW + rectX + 2, 0.035 * rectH + rectY + 2, 0.93 * rectW - 1, 0.93 * rectH - 1);
                    } else if (video.grid >= 20001 && video.grid <= 29999) {
                        // Aspect ratio frame overlay
                        let ratioW, ratioH;
                        switch (video.grid) {
                        case 20001:
                            ratioW = 1;
                            ratioH = 1;
                            break;
                        case 20169:
                            ratioW = 16;
                            ratioH = 9;
                            break;
                        case 20043:
                            ratioW = 4;
                            ratioH = 3;
                            break;
                        default:
                            ratioW = 9;
                            ratioH = 16;
                            break;
                        }
                        const targetAspect = ratioW / ratioH;
                        const videoAspect = rectW / rectH;
                        let frameW, frameH;
                        if (targetAspect >= videoAspect) {
                            frameW = rectW;
                            frameH = frameW / targetAspect;
                        } else {
                            frameH = rectH;
                            frameW = frameH * targetAspect;
                        }
                        const frameX = rectX + (rectW - frameW) / 2;
                        const frameY = rectY + (rectH - frameH) / 2;
                        ctx.rect(frameX + 2, frameY + 2, frameW - 1, frameH - 1);
                        if (video.grid === 20916) {
                            // Mobile safe guides: 420px from bottom, 50px from right (1080x1920)
                            const hLineY = frameY + frameH * (1500 / 1920) + 2;
                            const vLineX = frameX + frameW * (1030 / 1080) + 2;
                            ctx.moveTo(frameX + 2, hLineY);
                            ctx.lineTo(frameX + frameW - 1, hLineY);
                            ctx.moveTo(vLineX, frameY + 2);
                            ctx.lineTo(vLineX, frameY + frameH - 1);
                        }
                    } else {
                        // vertical grid lines
                        for (var x = 0; x * gridSizeX < parent.width + gridSizeX; x++) {
                            context.moveTo(gridOffsetX + x * gridSizeX + 1, 0);
                            context.lineTo(gridOffsetX + x * gridSizeX + 1, parent.height);
                        }
                        // horizontal grid lines
                        for (var y = 0; y * gridSizeY < parent.height + gridSizeY; y++) {
                            context.moveTo(0, gridOffsetY + y * gridSizeY + 1);
                            context.lineTo(parent.width, gridOffsetY + y * gridSizeY + 1);
                        }
                    }
                    // draw and close
                    context.stroke();
                    context.closePath();
                }
                // White line grid
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#ffffff";
                ctx.beginPath();
                if (video.grid === 8090) {
                    // 80/90% Safe Areas
                    ctx.rect(0.1 * rectW + rectX + 1, 0.1 * rectH + rectY + 1, 0.8 * rectW - 2, 0.8 * rectH - 2);
                    ctx.rect(0.05 * rectW + rectX + 1, 0.05 * rectH + rectY + 2, 0.9 * rectW - 2, 0.9 * rectH - 2);
                } else if (video.grid == 95) {
                    // EBU R95 Safe Areas
                    ctx.rect(0.05 * rectW + rectX + 1, 0.05 * rectH + rectY + 1, 0.9 * rectW - 2, 0.9 * rectH - 2);
                    ctx.rect(0.035 * rectW + rectX + 1, 0.035 * rectH + rectY + 1, 0.93 * rectW - 2, 0.93 * rectH - 2);
                } else if (video.grid >= 20001 && video.grid <= 29999) {
                    // Aspect ratio frame overlay
                    let ratioW, ratioH;
                    switch (video.grid) {
                    case 20001:
                        ratioW = 1;
                        ratioH = 1;
                        break;
                    case 20169:
                        ratioW = 16;
                        ratioH = 9;
                        break;
                    case 20043:
                        ratioW = 4;
                        ratioH = 3;
                        break;
                    default:
                        ratioW = 9;
                        ratioH = 16;
                        break;
                    }
                    const targetAspect = ratioW / ratioH;
                    const videoAspect = rectW / rectH;
                    let frameW, frameH;
                    if (targetAspect >= videoAspect) {
                        frameW = rectW;
                        frameH = frameW / targetAspect;
                    } else {
                        frameH = rectH;
                        frameW = frameH * targetAspect;
                    }
                    const frameX = rectX + (rectW - frameW) / 2;
                    const frameY = rectY + (rectH - frameH) / 2;
                    ctx.rect(frameX + 1, frameY + 1, frameW - 2, frameH - 2);
                    if (video.grid === 20916) {
                        // Mobile safe guides: 420px from bottom, 50px from right (1080x1920)
                        const hLineY = frameY + frameH * (1500 / 1920) + 1;
                        const vLineX = frameX + frameW * (1030 / 1080) + 1;
                        ctx.moveTo(frameX + 1, hLineY);
                        ctx.lineTo(frameX + frameW - 2, hLineY);
                        ctx.moveTo(vLineX, frameY + 1);
                        ctx.lineTo(vLineX, frameY + frameH - 2);
                    }
                } else {
                    // vertical grid lines
                    for (var x = 0; x * gridSizeX < parent.width + gridSizeX; x++) {
                        context.moveTo(gridOffsetX + x * gridSizeX, 0);
                        context.lineTo(gridOffsetX + x * gridSizeX, parent.height);
                    }
                    // horizontal grid lines
                    for (var y = 0; y * gridSizeY < parent.height + gridSizeY; y++) {
                        context.moveTo(0, gridOffsetY + y * gridSizeY);
                        context.lineTo(parent.width, gridOffsetY + y * gridSizeY);
                    }
                }
                // draw and close
                context.stroke();
                context.closePath();
            }
        }
    }

    Connections {
        function onRectChanged() {
            grid.requestPaint();
        }

        function onGridChanged() {
            grid.requestPaint();
        }

        function onOffsetChanged() {
            grid.requestPaint();
        }

        function onZoomChanged() {
            grid.requestPaint();
        }

        target: video
    }
}
