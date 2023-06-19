/*
 * Copyright (c) 2017-2021 Meltytech, LLC
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

function trackHeight(isCurves) {
    return isCurves? (multitrack.trackHeight * 2) : (multitrack.trackHeight < 30)? 20 : 48
}

function scrollIfNeeded(center, continuous) {
    var x = producer.position * timeScale;
    if (!tracksFlickable) return;
    if (settings.timelineCenterPlayhead || center) {
        if (x > tracksFlickable.contentX + tracksFlickable.width * 0.5)
            tracksFlickable.contentX = x - tracksFlickable.width * 0.5;
        else if (x < tracksFlickable.width * 0.5)
            tracksFlickable.contentX = 0;
        else if (x < tracksFlickable.contentX + tracksFlickable.width * 0.5)
            tracksFlickable.contentX = x - tracksFlickable.width * 0.5;
    } else if (tracksContainer.width > tracksFlickable.width) {
        if (continuous) {
            if (x > tracksFlickable.contentX + tracksFlickable.width - 50)
                tracksFlickable.contentX = x - tracksFlickable.width + 50;
            else if (x < 50)
                tracksFlickable.contentX = 0;
            else if (x < tracksFlickable.contentX + 50)
                tracksFlickable.contentX = x - 50;
        } else {
            // paginated
            var leftLimit = tracksFlickable.contentX + 50
            var pageCount = Math.floor((x - leftLimit)/(tracksFlickable.width - 100))
            tracksFlickable.contentX = Math.max(tracksFlickable.contentX + pageCount*(tracksFlickable.width - 100), 0);
        }
    }
}

function seekPreviousSimple() {
    var position = producer.position + producer.in
    if (position > filter.out)
        position = filter.out
    else if (position > filter.out - filter.animateOut)
        position = filter.out - filter.animateOut
    else if (position > filter.in + filter.animateIn)
        position = filter.in + filter.animateIn
    else if (position > filter.in)
        position = filter.in
    else
        position = 0
    producer.position = position - producer.in
}

function seekNextSimple() {
    var position = producer.position + producer.in
    if (position < filter.in)
        position = filter.in
    else if (position < filter.in + filter.animateIn)
        position = filter.in + filter.animateIn
    else if (position < filter.out - filter.animateOut)
        position = filter.out - filter.animateOut
    else if (position < filter.out)
        position = filter.out
    else
        position = producer.out
    producer.position = Math.min(position - producer.in, producer.duration - 1)
}

function clamp(x, minimum, maximum) {
    return Math.min(Math.max(x, minimum), maximum)
}

function scrollMax() {
    var maxWidth = Math.max(tracksFlickable.contentWidth - tracksFlickable.width + 14, 0)
    var maxHeight = Math.max(tracksFlickable.contentHeight - tracksFlickable.height + 14, 0)
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
            var x = wheel.pixelDelta.x
            var y = wheel.pixelDelta.y
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
