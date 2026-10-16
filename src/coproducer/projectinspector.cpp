/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "coproducer/projectinspector.h"

#include "docks/timelinedock.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <Mlt.h>
#include <QJsonArray>
#include <QJsonDocument>

namespace {

QString formatMltTimeMs(Mlt::Producer *producer, int value)
{
    if (!producer)
        return QStringLiteral("00:00:00.000");
    return QString::fromLatin1(producer->frames_to_time(value, mlt_time_clock));
}

QString clipDisplayName(Mlt::Producer *producer)
{
    if (!producer || !producer->is_valid())
        return QString();

    if (producer->is_cut()) {
        Mlt::Producer parent = producer->parent();
        if (parent.is_valid()) {
            const QString parentName = Util::clipDisplayName(&parent);
            if (!parentName.isEmpty() && parentName != QStringLiteral("Producer"))
                return parentName;
        }
    }

    return Util::clipDisplayName(producer);
}

QJsonObject summarizeProducer(Mlt::Producer *producer)
{
    QJsonObject object;
    if (!producer || !producer->is_valid()) {
        object["valid"] = false;
        return object;
    }

    const int length = producer->get_length();
    const int in = producer->get_in();
    const int out = producer->get_out();
    const int position = producer->position();

    const QString resource = QString::fromUtf8(producer->get("resource"));
    const QString service = QString::fromUtf8(producer->get("mlt_service"));
    const QString type = QString::fromUtf8(producer->get("mlt_type"));

    object["valid"] = true;
    object["name"] = clipDisplayName(producer);
    object["service"] = service;
    object["resource"] = resource;
    object["type"] = type;
    object["length_frames"] = length;
    object["length"] = formatMltTimeMs(producer, length);
    object["position_frames"] = position;
    object["position"] = formatMltTimeMs(producer, position);
    object["in_frames"] = in;
    object["in"] = formatMltTimeMs(producer, in);
    object["out_frames"] = out;
    object["out"] = formatMltTimeMs(producer, out);
    return object;
}

QJsonArray summarizePlaylistItems(Mlt::Playlist *playlist, int limit = 5)
{
    QJsonArray items;
    if (!playlist || !playlist->is_valid())
        return items;

    const int count = qMin(playlist->count(), limit);
    for (int i = 0; i < count; ++i) {
        auto *clip = playlist->get_clip(i);
        if (!clip)
            continue;
        items.append(summarizeProducer(clip));
    }
    return items;
}

QJsonObject summarizeClipEntry(Mlt::Producer *producer, int index = -1, int trackIndex = -1)
{
    QJsonObject entry;
    if (!producer || !producer->is_valid()) {
        entry["valid"] = false;
        return entry;
    }

    const QString resource = QString::fromUtf8(producer->get("resource"));
    const QString service = QString::fromUtf8(producer->get("mlt_service"));

    entry["valid"] = true;
    entry["index"] = index;
    entry["track"] = trackIndex;
    entry["name"] = clipDisplayName(producer);
    entry["resource"] = resource;
    entry["service"] = service;
    entry["length_frames"] = producer->get_length();
    entry["length"] = formatMltTimeMs(producer, producer->get_length());
    entry["in_frames"] = producer->get_in();
    entry["in"] = formatMltTimeMs(producer, producer->get_in());
    entry["out_frames"] = producer->get_out();
    entry["out"] = formatMltTimeMs(producer, producer->get_out());
    return entry;
}

QJsonArray summarizePlaylistClips(Mlt::Playlist *playlist, int trackIndex = -1)
{
    QJsonArray clips;
    if (!playlist || !playlist->is_valid())
        return clips;

    for (int i = 0; i < playlist->count(); ++i) {
        auto *clip = playlist->get_clip(i);
        if (!clip)
            continue;
        auto entry = summarizeClipEntry(clip, i, trackIndex);
        clips.append(entry);
    }
    return clips;
}

QJsonArray summarizeTimelineClips()
{
    QJsonArray clips;
    auto *mainWindow = &MainWindow::singleton();
    if (!mainWindow->timelineDock() || !mainWindow->timelineDock()->model())
        return clips;

    const auto &trackList = mainWindow->timelineDock()->model()->trackList();
    for (int trackIndex = 0; trackIndex < trackList.size(); ++trackIndex) {
        QScopedPointer<Mlt::Producer> track(mainWindow->timelineDock()->model()->tractor()->track(
            trackList.at(trackIndex).mlt_index));
        if (!track)
            continue;

        Mlt::Playlist playlist(*track);
        if (!playlist.is_valid())
            continue;

        const auto trackName = mainWindow->timelineDock()->model()->getTrackName(trackIndex);
        for (int clipIndex = 0; clipIndex < playlist.count(); ++clipIndex) {
            auto *clip = playlist.get_clip(clipIndex);
            if (!clip)
                continue;
            auto entry = summarizeClipEntry(clip, clipIndex, trackIndex);
            entry["track_name"] = trackName;
            clips.append(entry);
        }
    }
    return clips;
}

} // namespace

namespace Coproducer {

ToolDefinition projectInspectorTool()
{
    ToolDefinition tool;
    tool.name = QStringLiteral("shotcut_project_inspect");
    tool.description = QStringLiteral(
        "Inspect the currently open Shotcut clip, playlist, or timeline and the UI playback state. "
        "Return a compact JSON object with read-only metadata. Do not modify project content.");
    tool.parameters = QJsonObject{{"type", "object"},
                                  {"properties", QJsonObject{}},
                                  {"required", QJsonArray{}}};
    return tool;
}

QJsonObject inspectProject()
{
    auto &controller = Mlt::Controller::singleton();
    QJsonObject snapshot;

    const bool hasOpenClip = controller.isClip();
    const bool hasPlaylist = controller.isPlaylist();
    const bool hasTimeline = controller.isMultitrack();

    snapshot["has_open_clip"] = hasOpenClip;
    snapshot["has_playlist"] = hasPlaylist;
    snapshot["has_timeline"] = hasTimeline;
    snapshot["active_context"] = hasTimeline   ? QStringLiteral("timeline")
                                 : hasPlaylist ? QStringLiteral("playlist")
                                 : hasOpenClip ? QStringLiteral("clip")
                                               : QStringLiteral("none");

    if (controller.producer() && controller.producer()->is_valid()) {
        snapshot["current_producer"] = summarizeProducer(controller.producer());
    } else {
        snapshot["current_producer"] = QJsonObject{{"valid", false}};
    }

    if (hasOpenClip && controller.producer()) {
        snapshot["open_clip"] = summarizeProducer(controller.producer());
    }

    if (hasPlaylist && controller.producer()) {
        Mlt::Playlist playlist(*controller.producer());
        QJsonObject info = summarizeProducer(controller.producer());
        info["clip_count"] = playlist.is_valid() ? playlist.count() : 0;
        info["current_clip_index"] = playlist.is_valid() ? playlist.position() : -1;
        info["items"] = summarizePlaylistItems(&playlist);
        snapshot["playlist"] = info;
    }

    QJsonObject uiState;

    if (hasTimeline && controller.producer()) {
        QJsonObject info = summarizeProducer(controller.producer());
        info["duration_frames"] = controller.producer()->get_length();
        auto *mainWindow = &MainWindow::singleton();
        if (mainWindow->timelineDock()) {
            info["track_count"] = mainWindow->timelineDock()->model()->trackList().count();
            info["current_track"] = mainWindow->timelineDock()->currentTrack();
            info["selected_track"] = mainWindow->timelineDock()->selectedTrack();

            QJsonArray tracks;
            const auto &trackList = mainWindow->timelineDock()->model()->trackList();
            for (int i = 0; i < trackList.size(); ++i) {
                const auto &track = trackList.at(i);
                QJsonObject trackInfo;
                trackInfo["index"] = i;
                trackInfo["mlt_index"] = track.mlt_index;
                trackInfo["number"] = track.number;
                trackInfo["name"] = mainWindow->timelineDock()->model()->getTrackName(i);
                trackInfo["type"] = track.type == AudioTrackType   ? QStringLiteral("audio")
                                    : track.type == VideoTrackType ? QStringLiteral("video")
                                                                   : QStringLiteral("other");
                tracks.append(trackInfo);
            }
            info["tracks"] = tracks;

            const int currentTrackIndex = mainWindow->timelineDock()->currentTrack();
            if (currentTrackIndex >= 0 && currentTrackIndex < trackList.size()) {
                uiState["current_track_name"] = mainWindow->timelineDock()->model()->getTrackName(
                    currentTrackIndex);
            }
        }
        snapshot["timeline"] = info;
    }
    uiState["paused"] = controller.isPaused();
    uiState["position_frames"] = controller.producer() && controller.producer()->is_valid()
                                     ? controller.producer()->position()
                                     : 0;
    uiState["position"] = controller.producer() && controller.producer()->is_valid()
                              ? formatMltTimeMs(controller.producer(),
                                                controller.producer()->position())
                              : QStringLiteral("00:00:00.000");
    uiState["duration_frames"] = controller.producer() && controller.producer()->is_valid()
                                     ? controller.producer()->get_length()
                                     : 0;
    uiState["duration"] = controller.producer() && controller.producer()->is_valid()
                              ? formatMltTimeMs(controller.producer(),
                                                controller.producer()->get_length())
                              : QStringLiteral("00:00:00.000");
    uiState["fps"] = controller.profile().fps();
    uiState["resource"] = controller.resource();
    uiState["project_folder"] = controller.projectFolder();

    auto *mainWindow = &MainWindow::singleton();
    if (mainWindow->timelineDock()) {
        uiState["current_track"] = mainWindow->timelineDock()->currentTrack();
        uiState["selected_track"] = mainWindow->timelineDock()->selectedTrack();

        QJsonArray selection;
        const auto selectedClips = mainWindow->timelineDock()->selection();
        for (const auto &point : selectedClips) {
            QJsonObject clipSelection;
            clipSelection["track"] = point.y();
            clipSelection["clip"] = point.x();
            selection.append(clipSelection);
        }
        uiState["selected_clips"] = selection;
    }

    if (hasPlaylist && controller.producer()) {
        Mlt::Playlist playlist(*controller.producer());
        QJsonArray allClips = summarizePlaylistClips(&playlist);
        snapshot["clip_catalog"] = allClips;
        QJsonArray summary;
        for (int i = 0; i < qMin(20, allClips.size()); ++i)
            summary.append(allClips.at(i));
        snapshot["clip_summary"] = summary;
    } else if (hasTimeline && controller.producer()) {
        QJsonArray allClips = summarizeTimelineClips();
        snapshot["clip_catalog"] = allClips;
        QJsonArray summary;
        for (int i = 0; i < qMin(20, allClips.size()); ++i)
            summary.append(allClips.at(i));
        snapshot["clip_summary"] = summary;
    }

    snapshot["ui_state"] = uiState;

    return snapshot;
}

} // namespace Coproducer
