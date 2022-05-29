import QtQuick 2.0

DropArea {
    Canvas {
        id: grid
        anchors.fill : parent
        opacity: 0.5

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            if(video.grid > 0)
            {
                var zoom = (video.zoom > 0) ? video.zoom : 1.0
                var rectW = video.rect.width * zoom
                var rectH = video.rect.height * zoom
                var zoomOffsetX = ( video.rect.width - rectW ) / 2
                var zoomOffsetY = ( video.rect.height - rectH ) / 2
                var rectX = video.rect.x - video.offset.x + zoomOffsetX
                var rectY = video.rect.y - video.offset.y + zoomOffsetY
                var gridSizeX = (video.grid > 10000) ? rectW / (profile.width / (video.grid - 10000)) : rectW / video.grid
                var gridSizeY = (video.grid > 10000) ? rectH / (profile.height / (video.grid - 10000)) : rectH / video.grid
                var gridOffsetX = rectX % gridSizeX
                var gridOffsetY = rectY % gridSizeY

                if (video.grid <= 10000) {
                    // Black shadow grid (offset by 1)
                    ctx.lineWidth = 1
                    ctx.strokeStyle = "#000000"
                    ctx.beginPath()
                    if (video.grid === 8090) {
                        // 80/90% Safe Areas
                        ctx.rect( 0.1 * rectW + rectX + 2,  0.1 * rectH + rectY + 2, 0.8 * rectW - 1, 0.8 * rectH - 1)
                        ctx.rect(0.05 * rectW + rectX + 2, 0.05 * rectH + rectY + 2, 0.9 * rectW - 1, 0.9 * rectH - 1)
                    } else if (video.grid == 95) {
                        // EBU R95 Safe Areas
                        ctx.rect( 0.05 * rectW + rectX + 2,  0.05 * rectH + rectY + 2,  0.9 * rectW - 1,  0.9 * rectH - 1)
                        ctx.rect(0.035 * rectW + rectX + 2, 0.035 * rectH + rectY + 2, 0.93 * rectW - 1, 0.93 * rectH - 1)
                    } else {
                        // vertical grid lines
                        for(var x = 0; x * gridSizeX < parent.width + gridSizeX; x++)
                        {
                            context.moveTo(gridOffsetX + x * gridSizeX + 1, 0)
                            context.lineTo(gridOffsetX + x * gridSizeX + 1, parent.height)
                        }
                        // horizontal grid lines
                        for(var y = 0; y * gridSizeY < parent.height + gridSizeY; y++)
                        {
                            context.moveTo(0, gridOffsetY + y * gridSizeY + 1)
                            context.lineTo(parent.width, gridOffsetY + y * gridSizeY + 1)
                        }
                    }
                    // draw and close
                    context.stroke()
                    context.closePath()
                }

                // White line grid
                ctx.lineWidth = 1
                ctx.strokeStyle = "#ffffff"
                ctx.beginPath()
                if (video.grid === 8090) {
                    // 80/90% Safe Areas
                    ctx.rect( 0.1 * rectW + rectX + 1,  0.1 * rectH + rectY + 1, 0.8 * rectW - 2, 0.8 * rectH - 2)
                    ctx.rect(0.05 * rectW + rectX + 1, 0.05 * rectH + rectY + 2, 0.9 * rectW - 2, 0.9 * rectH - 2)
                } else if (video.grid == 95) {
                    // EBU R95 Safe Areas
                    ctx.rect( 0.05 * rectW + rectX + 1,  0.05 * rectH + rectY + 1,  0.9 * rectW - 2,  0.9 * rectH - 2)
                    ctx.rect(0.035 * rectW + rectX + 1, 0.035 * rectH + rectY + 1, 0.93 * rectW - 2, 0.93 * rectH - 2)
                } else {
                    // vertical grid lines
                    for(var x = 0; x * gridSizeX < parent.width + gridSizeX; x++)
                    {
                        context.moveTo(gridOffsetX + x * gridSizeX, 0)
                        context.lineTo(gridOffsetX + x * gridSizeX, parent.height)
                    }
                    // horizontal grid lines
                    for(var y = 0; y * gridSizeY < parent.height + gridSizeY; y++)
                    {
                        context.moveTo(0, gridOffsetY + y * gridSizeY)
                        context.lineTo(parent.width, gridOffsetY + y * gridSizeY)
                    }
                }
                // draw and close
                context.stroke()
                context.closePath()
            }
        }
    }

    Connections {
        target: video
        function onRectChanged() {
            grid.requestPaint()
        }
        function onGridChanged() {
            grid.requestPaint()
        }
        function onOffsetChanged() {
            grid.requestPaint()
        }
        function onZoomChanged() {
            grid.requestPaint()
        }
    }
}
