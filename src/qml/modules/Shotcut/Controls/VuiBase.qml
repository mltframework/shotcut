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
                var gridSizeX = rectW / video.grid
                var gridSizeY = rectH / video.grid
                ctx.lineWidth = 2
                ctx.strokeStyle = "#777777"
                ctx.beginPath()
                // Outline
                ctx.rect(rectX + 1, rectY + 1 , rectW - 2, rectH - 2)
                // vertical grid lines
                for(var x = 1; x * gridSizeX < rectW; x++)
                {
                    context.moveTo(rectX + x * gridSizeX, rectY)
                    context.lineTo(rectX + x * gridSizeX, rectY + rectH)
                }
                // horizontal grid lines
                for(var y = 1; y * gridSizeY < rectH; y++)
                {
                    context.moveTo(rectX, rectY + y * gridSizeY)
                    context.lineTo(rectX + rectW, rectY + y * gridSizeY)
                }
                // draw and close
                context.stroke()
                context.closePath()
            }
        }
    }

    Connections {
        target: video
        onRectChanged: {
            grid.requestPaint()
        }
        onGridChanged: {
            grid.requestPaint()
        }
        onOffsetChanged: {
            grid.requestPaint()
        }
        onZoomChanged: {
            grid.requestPaint()
        }
    }
}
