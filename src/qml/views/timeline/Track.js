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

var SNAP = 10
var SNAP_TRIM = 4

function snapClip(clip, repeater) {
    var left = clip.x
    var right = clip.x + clip.width
    if (clip.x > -SNAP && clip.x < SNAP) {
        // Snap around origin.
        clip.x = 0
        return
    } else {
        // Snap to other clips on the same track.
        for (var i = 0; i < repeater.count; i++) {
            // Do not snap to self.
            if (i === clip.DelegateModel.itemsIndex && clip.originalTrackIndex === repeater.itemAt(i).originalTrackIndex)
                continue
            var itemLeft = repeater.itemAt(i).x
            var itemRight = itemLeft + repeater.itemAt(i).width
            // Snap to blank
            if (right > itemLeft - SNAP && right < itemLeft + SNAP) {
                // Snap right edge to left edge.
                clip.x = itemLeft - clip.width
                return
            } else if (left > itemRight - SNAP && left < itemRight + SNAP) {
                // Snap left edge to right edge.
                clip.x = itemRight
                return
            } else if (right > itemRight - SNAP && right < itemRight + SNAP) {
                // Snap right edge to right edge.
                clip.x = itemRight - clip.width
                return
            } else if (left > itemLeft - SNAP && left < itemLeft + SNAP) {
                // Snap left edge to left edge.
                clip.x = itemLeft
                return
            }
        }
    }
    // Snap to markers
    var leftFrame = Math.round(left / timeScale)
    var prevMarkerX = Math.round(markers.prevMarkerPosition(leftFrame) * timeScale)
    if (left < prevMarkerX + SNAP) {
        clip.x = prevMarkerX
        return
    }
    var nextMarkerX = Math.round(markers.nextMarkerPosition(leftFrame) * timeScale)
    if (nextMarkerX > 0 && left > nextMarkerX - SNAP) {
        clip.x = nextMarkerX
        return
    }
    var rightFrame = Math.round(right / timeScale)
    var prevMarkerX = Math.round(markers.prevMarkerPosition(rightFrame) * timeScale)
    if (right < prevMarkerX + SNAP) {
        clip.x = prevMarkerX - clip.width
        return
    }
    var nextMarkerX = Math.round(markers.nextMarkerPosition(rightFrame) * timeScale)
    if (nextMarkerX > 0 && right > nextMarkerX - SNAP) {
        clip.x = nextMarkerX - clip.width
        return
    }
    if (!toolbar.scrub) {
        var cursorX = tracksFlickable.contentX + cursor.x
        if (left > cursorX - SNAP && left < cursorX + SNAP)
            // Snap around cursor/playhead.
            clip.x = cursorX
        if (right > cursorX - SNAP && right < cursorX + SNAP)
            clip.x = cursorX - clip.width
    }
}

function snapTrimIn(clip, delta, timeline, trackIndex) {
    var x = clip.x + delta * timeScale
    var cursorX = tracksFlickable.contentX + cursor.x
    if (delta < 0) {
        // Snap to other clips on the same track.
        for (var i = 0; i < repeater.count; i++) {
            if (i === clip.DelegateModel.itemsIndex || repeater.itemAt(i).isBlank || repeater.itemAt(i).isTransition)
                continue
            var itemLeft = repeater.itemAt(i).x
            var itemRight = itemLeft + repeater.itemAt(i).width
            if (x > itemLeft - SNAP_TRIM && x < itemLeft + SNAP_TRIM)
                return Math.round((itemLeft - clip.x) / timeScale)
            else if (x > itemRight - SNAP_TRIM && x < itemRight + SNAP_TRIM)
                return Math.round((itemRight - clip.x) / timeScale)
        }
    }
    // Snap to clips on other tracks.
    for (var j = 0; j < timeline.trackCount; j++) {
        if (j !== trackIndex) {
            var track = timeline.trackAt(j)
            for (i = 0; i < track.clipCount; i++) {
                var item = track.clipAt(i)
                if (item.isBlank)
                    continue
                itemLeft = item.x
                itemRight = itemLeft + item.width
                if (x > itemLeft - SNAP_TRIM && x < itemLeft + SNAP_TRIM)
                    return Math.round((itemLeft - clip.x) / timeScale)
                else if (x > itemRight - SNAP_TRIM && x < itemRight + SNAP_TRIM)
                    return Math.round((itemRight - clip.x) / timeScale)
            }
        }
    }
    // Snap to markers
    var frame = Math.round(x / timeScale)
    var prevMarkerX = Math.round(markers.prevMarkerPosition(frame) * timeScale)
    if (x < prevMarkerX + SNAP_TRIM) {
        return Math.round((prevMarkerX - clip.x) / timeScale)
    }
    var nextMarkerX = Math.round(markers.nextMarkerPosition(frame) * timeScale)
    if (nextMarkerX > 0 && x > nextMarkerX - SNAP_TRIM) {
        return Math.round((nextMarkerX - clip.x) / timeScale)
    }
    if (x > -SNAP_TRIM && x < SNAP_TRIM) {
        // Snap around origin.
        return Math.round(-clip.x / timeScale)
    } else if (x > cursorX - SNAP_TRIM && x < cursorX + SNAP_TRIM) {
        // Snap around cursor/playhead.
        return Math.round((cursorX - clip.x) / timeScale)
    }
    return delta
}

function snapTrimOut(clip, delta, timeline, trackIndex) {
    var rightEdge = clip.x + clip.width
    var x = rightEdge - delta * timeScale
    var cursorX = tracksFlickable.contentX + cursor.x
    if (delta < 0) {
        // Snap to other clips.
        for (var i = 0; i < repeater.count; i++) {
            if (i === clip.DelegateModel.itemsIndex || repeater.itemAt(i).isBlank || repeater.itemAt(i).isTransition)
                continue
            var itemLeft = repeater.itemAt(i).x
            var itemRight = itemLeft + repeater.itemAt(i).width
            if (x > itemLeft - SNAP_TRIM && x < itemLeft + SNAP_TRIM)
                return Math.round((rightEdge - itemLeft) / timeScale)
            else if (x > itemRight - SNAP_TRIM && x < itemRight + SNAP_TRIM)
                return Math.round((rightEdge - itemRight) / timeScale)
        }
    }
    // Snap to clips on other tracks.
    for (var j = 0; j < timeline.trackCount; j++) {
        if (j !== trackIndex) {
            var track = timeline.trackAt(j)
            for (i = 0; i < track.clipCount; i++) {
                var item = track.clipAt(i)
                if (item.isBlank)
                    continue
                itemLeft = item.x
                itemRight = itemLeft + item.width
                if (x > itemLeft - SNAP_TRIM && x < itemLeft + SNAP_TRIM)
                    return Math.round((rightEdge - itemLeft) / timeScale)
                else if (x > itemRight - SNAP_TRIM && x < itemRight + SNAP_TRIM)
                    return Math.round((rightEdge - itemRight) / timeScale)
            }
        }
    }
    // Snap to markers
    var frame = Math.round(x / timeScale)
    var prevMarkerX = Math.round(markers.prevMarkerPosition(frame) * timeScale)
    if (x < prevMarkerX + SNAP_TRIM) {
        return Math.round((rightEdge - prevMarkerX) / timeScale)
    }
    var nextMarkerX = Math.round(markers.nextMarkerPosition(frame) * timeScale)
    if (nextMarkerX > 0 && x > nextMarkerX - SNAP_TRIM) {
        return Math.round((rightEdge - nextMarkerX) / timeScale)
    }
    if (x > cursorX - SNAP_TRIM && x < cursorX + SNAP_TRIM) {
        // Snap around cursor/playhead.
        return Math.round((rightEdge - cursorX) / timeScale)
    }
    return delta
}

function snapDrop(pos, repeater) {
    var left = tracksFlickable.contentX + pos.x - headerWidth
    var right = left + dropTarget.width
    if (left > -SNAP && left < SNAP) {
        // Snap around origin.
        dropTarget.x = headerWidth
        return
    } else {
        // Snap to other clips.
        for (var i = 0; i < repeater.count; i++) {
            var itemLeft = repeater.itemAt(i).x
            var itemRight = itemLeft + repeater.itemAt(i).width
            if (right > itemLeft - SNAP && right < itemLeft + SNAP) {
                dropTarget.x = itemLeft - dropTarget.width + headerWidth - tracksFlickable.contentX
                return
            } else if (left > itemRight - SNAP && left < itemRight + SNAP) {
                dropTarget.x = itemRight + headerWidth - tracksFlickable.contentX
                return
            }
        }
    }
    if (!toolbar.scrub) {
        var cursorX = tracksFlickable.contentX + cursor.x
        if (left > cursorX - SNAP && left < cursorX + SNAP)
            // Snap around cursor/playhead.
            dropTarget.x = cursorX + headerWidth - tracksFlickable.contentX
        if (right > cursorX - SNAP && right < cursorX + SNAP)
            dropTarget.x = cursorX - dropTarget.width + headerWidth - tracksFlickable.contentX
    }
}

function selectionContains(selection, trackIndex, clipIndex) {
    for (var i = 0; i < selection.length; i++) {
        if (selection[i].x === clipIndex && selection[i].y === trackIndex)
            return true
    }
    return false
}
