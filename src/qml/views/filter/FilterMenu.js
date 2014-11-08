/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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
 
var visibility = {
    FAVORITE: 0,
    GPU: 1,
    VIDEO: 2,
    AUDIO: 3
};

var ITEM_HEIGHT = 30
 
function isVisible(meta, showType) {
    if(meta.isHidden) return false
    if(meta.needsGPU && !settings.playerGPU) return false
    if(!meta.needsGPU && settings.playerGPU && meta.gpuAlt != "") return false
    if(showType == visibility.FAVORITE && !meta.isFavorite) return false
    if(showType == visibility.GPU && !meta.needsGPU) return false
    if(showType == visibility.AUDIO && !meta.isAudio) return false
    if(showType == visibility.VIDEO && meta.isAudio ) return false
    if(showType == visibility.VIDEO && meta.needsGPU ) return false
    return true
}

function maxMenuHeight(showType, pad) {
    // Calculate the max possible height of the menu
    var i = 0
    var visibleItems = 0;
    for( i = 0; i < metadatamodel.rowCount(); i++ ) {
        var meta = metadatamodel.get(i)
        if(Logic.isVisible(meta, showType)) {
            visibleItems++
        }
    }
    return (visibleItems * ITEM_HEIGHT) + pad
}

function calcMenuRect(triggerItem, showType, pad) {
    var result = Qt.rect(0, 0, 0, 0)
    var itemPos = triggerItem.mapToItem(null,0,0)
    var triggerPos = Qt.point(itemPos.x + view.pos.x, itemPos.y + view.pos.y)
    var mainWinRect = application.mainWinRect
    
    result.height = Math.min(maxMenuHeight(showType, pad), mainWinRect.height)
    
    // Calculate the y position
    result.y = triggerPos.y - result.height / 2 // Ideal position is centered
    if( result.y < mainWinRect.y ) {
        // Window would be higher than the application window. Move it down
        result.y = mainWinRect.y
    } else if( result.y + result.height > mainWinRect.y + mainWinRect.height ) {
        // Window would be lower than the application window. Move it up
        result.y =  mainWinRect.y + mainWinRect.height - result.height
    }
    
    // Calculate the x position
    result.x = triggerPos.x
    
    return result
}