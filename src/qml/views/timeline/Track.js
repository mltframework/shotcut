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

function framesOf(item) {
    if (typeof item.clipDuration === 'number' && item.clipDuration > 0)
        return item.clipDuration
    return Math.round(item.width / timeScale)
}

// Exclusive end frame of an item — never prefer a value that would leave a
// 1-frame overlap (that becomes a transition on drop).
function exclusiveEndFrame(item) {
    var itemLeft = item.x
    var itemRight = itemLeft + item.width
    var byDuration = Math.round(itemLeft / timeScale) + framesOf(item)
    var byWidth = Math.round(itemRight / timeScale)
    return Math.max(byDuration, byWidth)
}

// Gaps must not be magnet targets. Snapping to a blank's edge that coincides
// with the dragged clip's current edge holds the clip still until the cursor
// leaves the SNAP radius. At scaleFactor≈10 that escape point is exactly
// halfway across a 2-frame gap (bubble help shows ~0.0167s, half of ~0.033s).
function isGapItem(item) {
    if (!item)
        return true
    // Read every known spelling; QML/JS bridging can make one of these fail.
    if (item.isBlank === true || item.isBlank === 1)
        return true
    if (item.blank === true || item.blank === 1)
        return true
    var svc = item.mltService
    if (svc === 'blank' || svc === 'colour' || svc === 'color') {
        // colour/color producers are sometimes used as spacers; only treat as a
        // gap when the clip has no real resource name.
        if (svc === 'blank')
            return true
    }
    if (typeof item.isBlank === 'string' && item.isBlank.toLowerCase() === 'true')
        return true
    return false
}


// Propose a snap and keep the closest edge (by pixel distance).
function considerSnap(state, edgePos, targetPos, snapFrame, why) {
    var dist = Math.abs(edgePos - targetPos)
    if (!(dist < SNAP))
        return
    // Ignore no-op snaps (e.g. blank edge under the clip's current edge).
    if (state.curFrame !== undefined && snapFrame === state.curFrame)
        return
    if (state.snapFrame === null || dist < state.bestDist) {
        state.snapFrame = snapFrame
        state.bestDist = dist
        state.why = why
    }
}

// True when a same-track join is nearby — used to suppress origin snap while
// closing a small gap (origin would otherwise re-grab until halfway).
function hasNearbyJoin(clip, repeater, left, right) {
    var reach = SNAP + 2 * timeScale
    for (var i = 0; i < repeater.count; i++) {
        if (i === clip.DelegateModel.itemsIndex && clip.originalTrackIndex === repeater.itemAt(i).originalTrackIndex)
            continue
        var item = repeater.itemAt(i)
        if (isGapItem(item))
            continue
        var itemLeft = item.x
        var itemRight = itemLeft + item.width
        if (Math.abs(right - itemLeft) < reach)
            return true
        if (Math.abs(left - itemRight) < reach)
            return true
    }
    return false
}

function snapClip(clip, repeater) {
    var left = clip.x
    var right = clip.x + clip.width
    var curFrame = Math.round(left / timeScale)
    var state = { snapFrame: null, bestDist: SNAP, why: null, curFrame: curFrame }
    var dur = framesOf(clip)

    // Neighbors first (before origin). Near timeline start, origin SNAP would
    // otherwise steal joins that should abut a nearby clip.
    for (var i = 0; i < repeater.count; i++) {
        // Do not snap to self.
        if (i === clip.DelegateModel.itemsIndex && clip.originalTrackIndex === repeater.itemAt(i).originalTrackIndex)
            continue
        var item = repeater.itemAt(i)
        if (isGapItem(item))
            continue
        var itemLeft = item.x
        var itemRight = itemLeft + item.width
        var otherStart = Math.round(itemLeft / timeScale)
        var otherEnd = exclusiveEndFrame(item)
        // Whole-frame abutment so we never overlap (overlap → transition).
        considerSnap(state, right, itemLeft, otherStart - dur, 'R-L:' + i)
        considerSnap(state, left, itemRight, otherEnd, 'L-R:' + i)
        considerSnap(state, right, itemRight, otherEnd - dur, 'R-R:' + i)
        considerSnap(state, left, itemLeft, otherStart, 'L-L:' + i)
    }
    if (state.snapFrame === null) {
        // Snap to markers
        var leftFrame = Math.round(left / timeScale)
        var rightFrame = Math.round(right / timeScale)
        var prevMarker = markers.prevMarkerPosition(leftFrame)
        var nextMarker = markers.nextMarkerPosition(leftFrame)
        if (prevMarker >= 0 && left < prevMarker * timeScale + SNAP) {
            state.snapFrame = prevMarker
            state.why = 'marker'
        } else if (nextMarker > 0 && left > nextMarker * timeScale - SNAP) {
            state.snapFrame = nextMarker
            state.why = 'marker'
        } else {
            prevMarker = markers.prevMarkerPosition(rightFrame)
            nextMarker = markers.nextMarkerPosition(rightFrame)
            if (prevMarker >= 0 && right < prevMarker * timeScale + SNAP) {
                state.snapFrame = prevMarker - framesOf(clip)
                state.why = 'marker'
            } else if (nextMarker > 0 && right > nextMarker * timeScale - SNAP) {
                state.snapFrame = nextMarker - framesOf(clip)
                state.why = 'marker'
            }
        }
    }
    // Playhead/origin magnets: skip while a same-track join is nearby.
    // Otherwise a playhead sitting in a small gap (or at frame 0/1) becomes a
    // second magnet exactly halfway to the real join (~0.0167s of a 0.033s gap).
    var joinNearby = hasNearbyJoin(clip, repeater, left, right)
    if (state.snapFrame === null && !settings.timelineDragScrub && !joinNearby) {
        var cursorX = tracksFlickable.contentX + cursor.x
        var cursorFrame = Math.round(cursorX / timeScale)
        if (left > cursorX - SNAP && left < cursorX + SNAP && cursorFrame !== curFrame) {
            // Snap around cursor/playhead.
            state.snapFrame = cursorFrame
            state.why = 'cursor'
        }
        if (right > cursorX - SNAP && right < cursorX + SNAP) {
            var toFrame = cursorFrame - framesOf(clip)
            if (toFrame !== curFrame) {
                state.snapFrame = toFrame
                state.why = 'cursor'
            }
        }
    }
    if (state.snapFrame === null && clip.x > -SNAP && clip.x < SNAP && !joinNearby) {
        state.snapFrame = 0
        state.why = 'origin'
    }
    if (state.snapFrame !== null && state.snapFrame !== curFrame) {
        clip.x = state.snapFrame * timeScale
        return true
    }
    return false
}

function snapTrimIn(clip, delta, timeline, trackIndex) {
    var x = clip.x + delta * timeScale
    var cursorX = tracksFlickable.contentX + cursor.x
    if (delta < 0) {
        // Snap to other clips on the same track.
        for (var i = 0; i < repeater.count; i++) {
            if (i === clip.DelegateModel.itemsIndex || isGapItem(repeater.itemAt(i)) || repeater.itemAt(i).isTransition)
                continue
            var itemLeft = repeater.itemAt(i).x
            var itemRight = itemLeft + repeater.itemAt(i).width
            if (x > itemLeft - SNAP_TRIM && x < itemLeft + SNAP_TRIM)
                return Math.round((itemLeft - clip.x) / timeScale)
            else if (x > itemRight - SNAP_TRIM && x < itemRight + SNAP_TRIM)
                return Math.round((itemRight - clip.x) / timeScale)
        }
        // Snap to clips on other tracks.
        for (var j = 0; j < timeline.trackCount; j++) {
            if (j === trackIndex)
                continue
            var track = timeline.trackAt(j)
            for (var i = 0; i < track.clipCount; i++) {
                var item = track.clipAt(i)
                if (isGapItem(item) || item.isTransition)
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
        // Snap to other clips on the same track.
        for (var i = 0; i < repeater.count; i++) {
            if (i === clip.DelegateModel.itemsIndex || isGapItem(repeater.itemAt(i)) || repeater.itemAt(i).isTransition)
                continue
            var itemLeft = repeater.itemAt(i).x
            var itemRight = itemLeft + repeater.itemAt(i).width
            if (x > itemLeft - SNAP_TRIM && x < itemLeft + SNAP_TRIM)
                return Math.round((rightEdge - itemLeft) / timeScale)
            else if (x > itemRight - SNAP_TRIM && x < itemRight + SNAP_TRIM)
                return Math.round((rightEdge - itemRight) / timeScale)
        }
        // Snap to clips on other tracks.
        for (var j = 0; j < timeline.trackCount; j++) {
            if (j === trackIndex)
                continue
            var track = timeline.trackAt(j)
            for (var i = 0; i < track.clipCount; i++) {
                var item = track.clipAt(i)
                if (isGapItem(item) || item.isTransition)
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
    var state = { snapFrame: null, bestDist: SNAP, why: null, curFrame: Math.round(left / timeScale) }
    var dropDur = framesOf(dropTarget)

    // Neighbors before origin (same reason as snapClip).
    for (var i = 0; i < repeater.count; i++) {
        var item = repeater.itemAt(i)
        if (isGapItem(item))
            continue
        var itemLeft = item.x
        var itemRight = itemLeft + item.width
        var otherStart = Math.round(itemLeft / timeScale)
        var otherEnd = exclusiveEndFrame(item)
        considerSnap(state, right, itemLeft, otherStart - dropDur, 'drop-R-L')
        considerSnap(state, left, itemRight, otherEnd, 'drop-L-R')
    }
    if (state.snapFrame === null && !settings.timelineDragScrub) {
        var cursorX = tracksFlickable.contentX + cursor.x
        var cursorFrame = Math.round(cursorX / timeScale)
        if (left > cursorX - SNAP && left < cursorX + SNAP)
            // Snap around cursor/playhead.
            state.snapFrame = cursorFrame
        else if (right > cursorX - SNAP && right < cursorX + SNAP)
            state.snapFrame = cursorFrame - dropDur
    }
    if (state.snapFrame === null && left > -SNAP && left < SNAP)
        state.snapFrame = 0
    if (state.snapFrame !== null)
        dropTarget.x = state.snapFrame * timeScale + headerWidth - tracksFlickable.contentX
}

function selectionContains(selection, trackIndex, clipIndex) {
    for (var i = 0; i < selection.length; i++)
        if (selection[i].x === clipIndex && selection[i].y === trackIndex)
            return true
    return false
}
