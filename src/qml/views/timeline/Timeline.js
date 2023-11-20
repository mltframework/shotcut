/*
 * Copyright (c) 2013-2023 Meltytech, LLC
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

function scrollIfNeeded(center, continouous) {
    let x = timeline.position * multitrack.scaleFactor;
    if (!tracksFlickable) return;
    if (center) {
        if (x > tracksFlickable.contentX + tracksFlickable.width * 0.5)
            tracksFlickable.contentX = x - tracksFlickable.width * 0.5;
        else if (x < tracksFlickable.width * 0.5)
            tracksFlickable.contentX = 0;
        else if (x < tracksFlickable.contentX + tracksFlickable.width * 0.5)
            tracksFlickable.contentX = x - tracksFlickable.width * 0.5;
    } else if (tracksContainer.width > tracksFlickable.width) {
        if (continouous) {
            if (x > tracksFlickable.contentX + tracksFlickable.width - 50)
                tracksFlickable.contentX = x - tracksFlickable.width + 50;
            else if (x < 50)
                tracksFlickable.contentX = 0;
            else if (x < tracksFlickable.contentX + 50)
                tracksFlickable.contentX = x - 50;
        } else {
            // paginated
            let leftLimit = tracksFlickable.contentX + 50
            let pageCount = Math.floor((x - leftLimit)/(tracksFlickable.width - 100))
            tracksFlickable.contentX = Math.max(tracksFlickable.contentX + pageCount*(tracksFlickable.width - 100), 0);
        }
    }
}

function dragging(pos, duration) {
    if (tracksRepeater.count > 0) {
        let headerHeight = ruler.height
        let i = 0;
        dropTarget.x = pos.x
        dropTarget.width = duration * multitrack.scaleFactor

        for (i = 0; i < tracksRepeater.count; i++) {
            let trackY = tracksRepeater.itemAt(i).y + headerHeight - tracksFlickable.contentY
            let trackH = tracksRepeater.itemAt(i).height
            if (pos.y >= trackY && pos.y < trackY + trackH) {
                timeline.currentTrack = i
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
        if (pos.x > headerWidth + tracksFlickable.width - 50) {
            // Right edge
            scrollTimer.backwards = false
            scrollTimer.start()
        } else if (pos.x >= headerWidth && pos.x < headerWidth + 50) {
            // Left edge
            if (tracksFlickable.contentX < 50) {
                scrollTimer.stop()
                tracksFlickable.contentX = 0;
            } else {
                scrollTimer.backwards = true
                scrollTimer.start()
            }
        } else {
            scrollTimer.stop()
        }

        if (settings.timelineDragScrub) {
            timeline.position = Math.round(
                (pos.x + tracksFlickable.contentX - headerWidth) / multitrack.scaleFactor)
        }
        if (settings.timelineSnap) {
            for (i = 0; i < tracksRepeater.count; i++)
                tracksRepeater.itemAt(i).snapDrop(pos)
        }
    } else {
        timeline.currentTrack = 0
    }
}

function dropped() {
    dropTarget.visible = false
    scrollTimer.running = false
}

function acceptDrop(xml) {
    let position = Math.round((dropTarget.x + tracksFlickable.contentX - headerWidth) / multitrack.scaleFactor)
    timeline.handleDrop(timeline.currentTrack, position, xml)
}

function trackHeight(isAudio) {
    return isAudio? Math.max(20, multitrack.trackHeight) : multitrack.trackHeight * 2
}

function clamp(x, minimum, maximum) {
    return Math.min(Math.max(x, minimum), maximum)
}

function scrollMax() {
    let maxWidth = Math.max(tracksFlickable.contentWidth - tracksFlickable.width + 14, 0)
    let maxHeight = Math.max(tracksFlickable.contentHeight - tracksFlickable.height + 14, 0)
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
            n = (application.OS === 'macOS')? wheel.angleDelta.x : wheel.angleDelta.y
            multitrack.trackHeight = Math.max(10, multitrack.trackHeight + n / 25)
        }
    } else {
        // Scroll
        if ((wheel.pixelDelta.x || wheel.pixelDelta.y) && wheel.modifiers === Qt.NoModifier) {
            let x = wheel.pixelDelta.x
            let y = wheel.pixelDelta.y
            if (application.OS !== 'Windows' && !x && y) {
                x = y;
                y = 0;
            }
            // Track pads provide both horizontal and vertical.
            if (!y || Math.abs(x) > 2)
                tracksFlickable.contentX = clamp(tracksFlickable.contentX - x, 0, scrollMax().x)
            tracksFlickable.contentY = clamp(tracksFlickable.contentY - y, 0, scrollMax().y)
        } else {
            // Vertical only mouse wheel requires modifier for vertical scroll.
            if (wheel.modifiers === Qt.AltModifier) {
                n = Math.round((application.OS === 'macOS'? wheel.angleDelta.y : wheel.angleDelta.x) / 2)
                tracksFlickable.contentY = clamp(tracksFlickable.contentY - n, 0, scrollMax().y)
            } else {
                n = Math.round(wheel.angleDelta.y / 2)
                tracksFlickable.contentX = clamp(tracksFlickable.contentX - n, 0, scrollMax().x)
            }
        }
    }
}

function toggleSelection(trackIndex, clipIndex) {
    let result = []
    let skip = false
    timeline.selection.forEach(function(el) {
        if (tracksRepeater.itemAt(el.y).clipAt(el.x).isBlank)
            // Do not support multiselect for blank clips
            return
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
    let result = [timeline.selection.length? timeline.selection[0] : Qt.point(clipIndex, trackIndex)]
    // this only works on a single track for now
    if (timeline.selection.length && trackIndex === result[0].y) {
        if (clipIndex > result[0].x) {
            for (let x = result[0].x + 1; x <= clipIndex; x++)
                if (!tracksRepeater.itemAt(trackIndex).clipAt(x).isBlank)
                    result.push(Qt.point(x, trackIndex))
        } else {
            for (let x = clipIndex; x < result[0].x; x++)
                if (!tracksRepeater.itemAt(trackIndex).clipAt(x).isBlank)
                    result.push(Qt.point(x, trackIndex))
        }
    }
    return result
}

function selectionContains(trackIndex, clipIndex) {
    let selection = timeline.selection
    for (let i = 0; i < selection.length; i++) {
        if (selection[i].x === clipIndex && selection[i].y === trackIndex)
            return true
    }
    return false
}

function selectClips() {
    let result = [];
    let rectA = Qt.rect(selectionBox.x, selectionBox.y, selectionBox.width, selectionBox.height);
    for (let trackIndex = 0; trackIndex < tracksRepeater.count; trackIndex++) {
        let track = tracksRepeater.itemAt(trackIndex);
        let rectB = Qt.rect(track.x, track.y, track.width, track.height);
        if (application.intersects(rectA, rectB)) {
            for (let clipIndex = 0; clipIndex < track.clipCount; clipIndex++) {
                let clip = track.clipAt(clipIndex);
                if (!clip.isBlank) {
                    rectB = selectionBox.parent.mapFromItem(track, clip.x, clip.y, clip.width, clip.height)
                    if (application.intersects(rectA, rectB))
                        result.push(Qt.point(clipIndex, trackIndex));
                }
            }
        }
    }
    if (result.length > 0)
        timeline.selection = result;
}
