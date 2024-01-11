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

#ifndef SHOTCUT_MLT_PROPERTIES_H
#define SHOTCUT_MLT_PROPERTIES_H

/* This file contains all of the Shotcut-specific MLT properties.
 * See also https://www.shotcut.org/notes/mltxml-annotations/
 *
 * A property should be prefaced with an underscore if it will not be saved
 * in the XML even if it never has a chance of getting into there. This makes
 * it more clear which is also an XML annotation or purely internal use.
 */

/* MLT XML annotations */

#define kShotcutXmlProperty "shotcut"
#define kAudioTrackProperty "shotcut:audio"
#define kCommentProperty "shotcut:comment"
#define kShotcutFilterProperty "shotcut:filter"
#define kShotcutPlaylistProperty "shotcut:playlist"
#define kShotcutTransitionProperty "shotcut:transition"
#define kShotcutProducerProperty "shotcut:producer"
#define kShotcutVirtualClip "shotcut:virtual"
#define kTimelineScaleProperty "shotcut:scaleFactor"
#define kTrackHeightProperty "shotcut:trackHeight"
#define kTrackNameProperty "shotcut:name"
#define kTrackLockProperty "shotcut:lock"
#define kVideoTrackProperty "shotcut:video"
#define kShotcutCaptionProperty "shotcut:caption"
#define kShotcutDetailProperty "shotcut:detail"
#define kShotcutHashProperty "shotcut:hash"
#define kShotcutSkipConvertProperty "shotcut:skipConvert"
#define kShotcutAnimInProperty "shotcut:animIn"
#define kShotcutAnimOutProperty "shotcut:animOut"
#define kShotcutMarkersProperty "shotcut:markers"
#define kShotcutGroupProperty "shotcut:group"
// Shotcut's VUI (video user interface) components set this so that glwidget can
// hide the VUI when the play head is not over the clip with the current filter.
#define kShotcutVuiMetaProperty "meta.shotcut.vui"
#define kDefaultAudioIndexProperty "shotcut:defaultAudioIndex"
#define kOriginalResourceProperty "shotcut:resource"
#define kOriginalInProperty "shotcut:originalIn"
#define kOriginalOutProperty "shotcut:originalOut"
#define kDisableProxyProperty "shotcut:disableProxy"
#define kBackupProperty "shotcut:backup"
// "shotcut:proxy" is internal only because older versions do not know to hide it.
// "shotcut:metaProxy" indicates whether the "meta." properties reflect source or proxy.
#define kMetaProxyProperty "shotcut:proxy.meta"

/* Project specific properties */
#define kShotcutProjectAudioChannels "shotcut:projectAudioChannels"
#define kShotcutProjectFolder "shotcut:projectFolder"
#define kShotcutProjectNote "shotcut:projectNote"

/* Ideally all shotcut properties should begin with "shotcut:", but these
 * do not and kept for legacy reasons? */

#define kAspectRatioNumerator "shotcut_aspect_num"
#define kAspectRatioDenominator "shotcut_aspect_den"
#define kShotcutSequenceProperty "shotcut_sequence"

/* Special object Ids expected by Shotcut and used in XML */

#define kBackgroundTrackId "background"
#define kLegacyPlaylistTrackId "main bin"
#define kPlaylistTrackId "main_bin"

/* Internal only */

#define kAudioLevelsProperty "_shotcut:audio-levels"
#define kBackgroundCaptureProperty "_shotcut:bgcapture"
#define kPlaylistIndexProperty "_shotcut:playlistIndex"
#define kPlaylistStartProperty "_shotcut:playlistStart"
#define kFilterInProperty "_shotcut:filter_in"
#define kFilterOutProperty "_shotcut:filter_out"
#define kThumbnailInProperty "_shotcut:thumbnail-in"
#define kThumbnailOutProperty "_shotcut:thumbnail-out"
#define kUuidProperty "_shotcut:uuid"
#define kMultitrackItemProperty "_shotcut:multitrack-item"
#define kExportFromProperty "_shotcut:exportFromDefault"
#define kTrackIndexProperty "_shotcut:trackIndex"
#define kClipIndexProperty "_shotcut:clipIndex"
#define kFilterIndexProperty "_shotcut:filterIndex"
#define kGroupProperty "_shotcut:group"
#define kShotcutInProperty "_shotcut:in"
#define kShotcutOutProperty "_shotcut:out"
#define kNewTrackIndexProperty "_shotcut:newTrackIndex"
#define kNewFilterProperty "_shotcut:newFilter"
#define kShotcutFiltersClipboard "shotcut:filtersClipboard"
#define kIsProxyProperty "shotcut:proxy"

#define kDefaultMltProfile "atsc_1080p_25"

#endif // SHOTCUT_MLT_PROPERTIES_H
