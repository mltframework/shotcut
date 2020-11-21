/*
 * Copyright (c) 2013-2020 Meltytech, LLC
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

function scrollIfNeeded(center) {
    var x = timeline.position * multitrack.scaleFactor;
    if (!scrollView) return;
    if (settings.timelineCenterPlayhead || center) {
        if (x > scrollView.flickableItem.contentX + scrollView.width * 0.5)
            scrollView.flickableItem.contentX = x - scrollView.width * 0.5;
        else if (x < scrollView.width * 0.5)
            scrollView.flickableItem.contentX = 0;
        else if (x < scrollView.flickableItem.contentX + scrollView.width * 0.5)
            scrollView.flickableItem.contentX = x - scrollView.width * 0.5;
    } else {
        if (x > scrollView.flickableItem.contentX + scrollView.width - 50)
            scrollView.flickableItem.contentX = x - scrollView.width + 50;
        else if (x < 50)
            scrollView.flickableItem.contentX = 0;
        else if (x < scrollView.flickableItem.contentX + 50)
            scrollView.flickableItem.contentX = x - 50;
    }
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
    } else {
        currentTrack = 0
    }
}

function dropped() {
    dropTarget.visible = false
    scrollTimer.running = false
}

function acceptDrop(xml) {
    var position = Math.round((dropTarget.x + scrollView.flickableItem.contentX - headerWidth) / multitrack.scaleFactor)
    if (settings.timelineRipple)
        timeline.insert(currentTrack, position, xml, false)
    else
        timeline.overwrite(currentTrack, position, xml, false)
}

function trackHeight(isAudio) {
    return isAudio? Math.max(20, multitrack.trackHeight) : multitrack.trackHeight * 2
}

function clamp(x, minimum, maximum) {
    return Math.min(Math.max(x, minimum), maximum)
}

function scrollMax() {
    var maxWidth = Math.max(scrollView.flickableItem.contentWidth - scrollView.width + 14, 0)
    var maxHeight = Math.max(scrollView.flickableItem.contentHeight - scrollView.height + 14, 0)
    return Qt.point(maxWidth, maxHeight)
}

function onMouseWheel(wheel) {
    var n
    if ((wheel.modifiers === Qt.ControlModifier) || (wheel.modifiers === Qt.ShiftModifier)) {
        // Zoom
        if (wheel.modifiers & Qt.ControlModifier) {
            adjustZoom(wheel.angleDelta.y / 2000, wheel.x)
        }
        if (wheel.modifiers & Qt.ShiftModifier) {
            n = (application.OS === 'OS X')? wheel.angleDelta.x : wheel.angleDelta.y
            multitrack.trackHeight = Math.max(10, multitrack.trackHeight + n / 25)
        }
    } else {
        // Scroll
        if ((wheel.pixelDelta.x || wheel.pixelDelta.y) && wheel.modifiers === Qt.NoModifier) {
            var x = wheel.pixelDelta.x
            var y = wheel.pixelDelta.y
            // Track pads provide both horizontal and vertical.
            if (!y || Math.abs(x) > 2)
                scrollView.flickableItem.contentX = clamp(scrollView.flickableItem.contentX - x, 0, scrollMax().x)
            scrollView.flickableItem.contentY = clamp(scrollView.flickableItem.contentY - y, 0, scrollMax().y)
        } else {
            // Vertical only mouse wheel requires modifier for vertical scroll.
            if (wheel.modifiers === Qt.AltModifier) {
                n = Math.round((application.OS === 'OS X'? wheel.angleDelta.y : wheel.angleDelta.x) / 2)
                scrollView.flickableItem.contentY = clamp(scrollView.flickableItem.contentY - n, 0, scrollMax().y)
            } else {
                n = Math.round(wheel.angleDelta.y / 2)
                scrollView.flickableItem.contentX = clamp(scrollView.flickableItem.contentX - n, 0, scrollMax().x)
            }
        }
    }
}

function toggleSelection(trackIndex, clipIndex) {
    var result = []
    var skip = false
    timeline.selection.forEach(function(el) {
        if (el.x !== clipIndex || el.y !== trackIndex)
            result.push(el)
        else
            skip = true
    })
    if (!skip)
        result.push(Qt.point(clipIndex, trackIndex))
    return result
}

function selectRange(trackIndex, clipIndex) {
    var result = [timeline.selection.length? timeline.selection[0] : Qt.point(clipIndex, trackIndex)]
    // this only works on a single track for now
    if (timeline.selection.length && trackIndex === result[0].y) {
        var x
        if (clipIndex > result[0].x) {
            for (x = result[0].x + 1; x <= clipIndex; x++)
                result.push(Qt.point(x, trackIndex))
        } else {
            for (x = clipIndex; x < result[0].x; x++)
                result.push(Qt.point(x, trackIndex))
        }
    }
    return result
}

function selectionContains(trackIndex, clipIndex) {
    var selection = timeline.selection
    for (var i = 0; i < selection.length; i++) {
        if (selection[i].x === clipIndex && selection[i].y === trackIndex)
            return true
    }
    return false
}
