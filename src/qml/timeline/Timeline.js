/*
 * Copyright (c) 2013-2018 Meltytech, LLC
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

function scrollIfNeeded() {
    var x = timeline.position * multitrack.scaleFactor;
    if (!scrollView) return;
    if (x > scrollView.flickableItem.contentX + scrollView.width - 50)
        scrollView.flickableItem.contentX = x - scrollView.width + 50;
    else if (x < 50)
        scrollView.flickableItem.contentX = 0;
    else if (x < scrollView.flickableItem.contentX + 50)
        scrollView.flickableItem.contentX = x - 50;
}

function dragging(pos, duration) {
    if (tracksRepeater.count > 0) {
        var headerHeight = ruler.height + toolbar.height
        dropTarget.x = pos.x
        dropTarget.width = duration * multitrack.scaleFactor

        for (var i = 0; i < tracksRepeater.count; i++) {
            var trackY = tracksRepeater.itemAt(i).y + headerHeight - scrollView.flickableItem.contentY
            var trackH = tracksRepeater.itemAt(i).height
            if (pos.y >= trackY && pos.y < trackY + trackH) {
                currentTrack = i
                if (pos.x > headerWidth) {
                    dropTarget.height = trackH
                    dropTarget.y = trackY
                    if (dropTarget.y < headerHeight) {
                        dropTarget.height -= headerHeight - dropTarget.y
                        dropTarget.y = headerHeight
                    }
                    dropTarget.visible = true
                }
                break
            }
        }
        if (i === tracksRepeater.count || pos.x <= headerWidth)
            dropTarget.visible = false

        // Scroll tracks if at edges.
        if (pos.x > headerWidth + scrollView.width - 50) {
            // Right edge
            scrollTimer.backwards = false
            scrollTimer.start()
        } else if (pos.x >= headerWidth && pos.x < headerWidth + 50) {
            // Left edge
            if (scrollView.flickableItem.contentX < 50) {
                scrollTimer.stop()
                scrollView.flickableItem.contentX = 0;
            } else {
                scrollTimer.backwards = true
                scrollTimer.start()
            }
        } else {
            scrollTimer.stop()
        }

        if (toolbar.scrub) {
            timeline.position = Math.round(
                (pos.x + scrollView.flickableItem.contentX - headerWidth) / multitrack.scaleFactor)
        }
        if (settings.timelineSnap) {
            for (i = 0; i < tracksRepeater.count; i++)
                tracksRepeater.itemAt(i).snapDrop(pos)
        }
    }
}

function dropped() {
    dropTarget.visible = false
    scrollTimer.running = false
}

function acceptDrop(xml) {
    var position = Math.round((dropTarget.x + scrollView.flickableItem.contentX - headerWidth) / multitrack.scaleFactor)
    if (settings.timelineRipple)
        timeline.insert(currentTrack, position, xml)
    else
        timeline.overwrite(currentTrack, position, xml)
}

function trackHeight(isAudio) {
    return isAudio? Math.max(40, multitrack.trackHeight) : multitrack.trackHeight * 2
}

function clamp(x, minimum, maximum) {
    return Math.min(Math.max(x, minimum), maximum)
}

function onMouseWheel(wheel) {
    if ((wheel.modifiers & Qt.ControlModifier) || (wheel.modifiers & Qt.ShiftModifier)) {
        // Zoom
        if (wheel.modifiers & Qt.ControlModifier) {
            adjustZoom(wheel.angleDelta.y / 720)
        }
        if (wheel.modifiers & Qt.ShiftModifier) {
            multitrack.trackHeight = Math.max(30, multitrack.trackHeight + wheel.angleDelta.y / 5)
        }
    } else {
        // Scroll
        var maxWidth = Math.max(scrollView.flickableItem.contentWidth - scrollView.width + 14, 0)
        var maxHeight = Math.max(scrollView.flickableItem.contentHeight - scrollView.height + 14, 0)
        if (wheel.pixelDelta.x || wheel.pixelDelta.y) {
            // Track pads provide both horizontal and vertical.
            var x = wheel.pixelDelta.x
            var y = wheel.pixelDelta.y
            if (!y || Math.abs(x) > 2)
                scrollView.flickableItem.contentX = clamp(scrollView.flickableItem.contentX - x, 0, maxWidth)
            scrollView.flickableItem.contentY = clamp(scrollView.flickableItem.contentY - y, 0, maxHeight)
        } else {
            // Vertical only mouse wheel requires modifier for vertical scroll.
            var y = Math.round(wheel.angleDelta.y / 5)
            if ((wheel.modifiers & Qt.AltModifier) || (wheel.modifiers & Qt.MetaModifier))
                scrollView.flickableItem.contentX = clamp(scrollView.flickableItem.contentX - y, 0, maxWidth)
            else
                scrollView.flickableItem.contentY = clamp(scrollView.flickableItem.contentY - y, 0, maxHeight)
        }
    }
}
