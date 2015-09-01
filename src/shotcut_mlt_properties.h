/*
 * Copyright (c) 2013-2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef SHOTCUT_MLT_PROPERTIES_H
#define SHOTCUT_MLT_PROPERTIES_H

/* This file contains all of the Shotcut-specific MLT properties.
 * See also http://www.shotcut.org/bin/view/Shotcut/MLTXMLAnnotations
 *
 * A property should be prefaced with an underscore if it will not be saved
 * in the XML even if it never has a chance of getting into there. This makes
 * it more clear which is also an XML annotation or purely internal use.
 */

/* MLT XML annotations */

#define kAudioTrackProperty "shotcut:audio"
#define kCommentProperty "shotcut:comment"
#define kShotcutFilterProperty "shotcut:filter"
#define kShotcutPlaylistProperty "shotcut:playlist"
#define kShotcutTransitionProperty "shotcut:transition"
#define kShotcutVirtualClip "shotcut:virtual"
#define kTimelineScaleProperty "shotcut:scaleFactor"
#define kTrackHeightProperty "shotcut:trackHeight"
#define kTrackNameProperty "shotcut:name"
#define kTrackLockProperty "shotcut:lock"
#define kVideoTrackProperty "shotcut:video"

/* Ideally all shotcut properties should begin with "shotcut:", but these
 * do not and kept for legacy reasons? */

#define kAspectRatioNumerator "shotcut_aspect_num"
#define kAspectRatioDenominator "shotcut_aspect_den"
#define kShotcutResourceProperty "shotcut_resource"
#define kShotcutSequenceProperty "shotcut_sequence"

/* Special object Ids expected by Shotcut and used in XML */

#define kBackgroundTrackId "background"
#define kPlaylistTrackId "main bin"

/* Internal only */

#define kAudioLevelsProperty "_shotcut:audio-levels"
#define kBackgroundCaptureProperty "_shotcut:bgcapture"
#define kPlaylistIndexProperty "_shotcut:playlistIndex"
#define kFilterInProperty "_shotcut:filter_in"
#define kFilterOutProperty "_shotcut:filter_out"
#define kThumbnailInProperty "_shotcut:thumbnail-in"
#define kThumbnailOutProperty "_shotcut:thumbnail-out"
#define kUndoIdProperty "_shotcut:undo_id"
#define kUuidProperty "_shotcut:uuid"

#endif // SHOTCUT_MLT_PROPERTIES_H
