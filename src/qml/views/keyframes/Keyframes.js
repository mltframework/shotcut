/*
 * Copyright (c) 2017-2020 Meltytech, LLC
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
    return isCurves? (multitrack.trackHeight * 2) : (multitrack.trackHeight < 30)? 20 : 36
}

function scrollIfNeeded(center) {
    var x = producer.position * timeScale;
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
