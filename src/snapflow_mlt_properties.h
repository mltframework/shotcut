/*
 * Copyright (c) 2013-2026 Meltytech, LLC
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

#ifndef SNAPFLOW_MLT_PROPERTIES_H
#define SNAPFLOW_MLT_PROPERTIES_H

/* This file contains all of the Snapflow-specific MLT properties.
 * See also https://www.snapflow.org/notes/mltxml-annotations/
 *
 * A property should be prefaced with an underscore if it will not be saved
 * in the XML even if it never has a chance of getting into there. This makes
 * it more clear which is also an XML annotation or purely internal use.
 */

/* MLT XML annotations */

#define kSnapflowXmlProperty "snapflow"
#define kAudioTrackProperty "snapflow:audio"
#define kCommentProperty "snapflow:comment"
#define kSnapflowFilterProperty "snapflow:filter"
#define kSnapflowPlaylistProperty "snapflow:playlist"
#define kSnapflowTransitionProperty "snapflow:transition"
#define kSnapflowProducerProperty "snapflow:producer"
#define kSnapflowVirtualClip "snapflow:virtual"
#define kTimelineScaleProperty "snapflow:scaleFactor"
#define kTrackHeightProperty "snapflow:trackHeight"
#define kTrackHeaderWidthProperty "snapflow:trackHeaderWidth"
#define kTrackNameProperty "snapflow:name"
#define kTrackLockProperty "snapflow:lock"
#define kVideoTrackProperty "snapflow:video"
#define kSnapflowCaptionProperty "snapflow:caption"
#define kSnapflowDetailProperty "snapflow:detail"
#define kSnapflowHashProperty "snapflow:hash"
#define kSnapflowHiddenProperty "snapflow:hidden"
#define kSnapflowSkipConvertProperty "snapflow:skipConvert"
#define kSnapflowAnimInProperty "snapflow:animIn"
#define kSnapflowAnimOutProperty "snapflow:animOut"
#define kSnapflowMarkersProperty "snapflow:markers"
#define kSnapflowGroupProperty "snapflow:group"
// Snapflow's VUI (video user interface) components set this so that glwidget can
// hide the VUI when the play head is not over the clip with the current filter.
#define kSnapflowVuiMetaProperty "meta.snapflow.vui"
#define kDefaultAudioIndexProperty "snapflow:defaultAudioIndex"
#define kOriginalResourceProperty "snapflow:resource"
#define kOriginalInProperty "snapflow:originalIn"
#define kOriginalOutProperty "snapflow:originalOut"
#define kDisableProxyProperty "snapflow:disableProxy"
#define kBackupProperty "snapflow:backup"
// "snapflow:proxy" is internal only because older versions do not know to hide it.
// "snapflow:metaProxy" indicates whether the "meta." properties reflect source or proxy.
#define kMetaProxyProperty "snapflow:proxy.meta"
#define kSnapflowBinsProperty "snapflow:bins"

/* Project specific properties */
#define kSnapflowProjectAudioChannels "snapflow:projectAudioChannels"
#define kSnapflowProjectFolder "snapflow:projectFolder"
#define kSnapflowProjectNote "snapflow:projectNote"
#define kSnapflowProjectProcessingMode "snapflow:processingMode"
#define kSnapflowColorTransfer "snapflow:colorTransfer"

/* Ideally all snapflow properties should begin with "snapflow:", but these
 * do not and kept for legacy reasons? */

#define kAspectRatioNumerator "snapflow_aspect_num"
#define kAspectRatioDenominator "snapflow_aspect_den"
#define kSnapflowSequenceProperty "snapflow_sequence"

/* Special object Ids expected by Snapflow and used in XML */

#define kBackgroundTrackId "background"
#define kLegacyPlaylistTrackId "main bin"
#define kPlaylistTrackId "main_bin"

/* Internal only */

#define kAudioLevelsProperty "_snapflow:audio-levels"
#define kBackgroundCaptureProperty "_snapflow:bgcapture"
#define kPlaylistIndexProperty "_snapflow:playlistIndex"
#define kPlaylistStartProperty "_snapflow:playlistStart"
#define kFilterInProperty "_snapflow:filter_in"
#define kFilterOutProperty "_snapflow:filter_out"
#define kThumbnailInProperty "_snapflow:thumbnail-in"
#define kThumbnailOutProperty "_snapflow:thumbnail-out"
#define kUuidProperty "_snapflow:uuid"
#define kMultitrackItemProperty "_snapflow:multitrack-item"
#define kExportFromProperty "_snapflow:exportFromDefault"
#define kTrackIndexProperty "_snapflow:trackIndex"
#define kFilterIndexProperty "_snapflow:filterIndex"
#define kNewFilterProperty "_snapflow:newFilter"
#define kSnapflowFiltersClipboard "snapflow:filtersClipboard"
#define kIsProxyProperty "snapflow:proxy"
#define kPrivateProducerProperty "_snapflow:producer"
#define kNewFrameOutProperty "_snapflow:newFrameOut"

#define kDefaultMltProfile "atsc_1080p_25"

#endif // SNAPFLOW_MLT_PROPERTIES_H
