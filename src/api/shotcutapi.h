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

#ifndef SHOTCUT_API_SHOTCUTAPI_H
#define SHOTCUT_API_SHOTCUTAPI_H

#include "api/apifilters.h"
#include "api/apijobs.h"
#include "api/apimarkers.h"
#include "api/apiobject.h"
#include "api/apiplayer.h"
#include "api/apiplaylist.h"
#include "api/apiproject.h"
#include "api/apitimeline.h"
#include "api/apiundo.h"

namespace Shotcut {
namespace Api {

// Root of the scripting API, exposed to QML as "shotcut" and reflected into MCP tools.
class ShotcutApi : public ApiObject
{
    Q_OBJECT
    Q_CLASSINFO("doc",
                "Root of the Shotcut scripting API. All times are integer frames at the project "
                "frame rate (project.profile.fps).")
    Q_CLASSINFO("doc.version", "Shotcut version.")
    Q_CLASSINFO("doc.apiVersion", "Scripting API version.")
    Q_CLASSINFO("doc.revision",
                "Increases whenever the project may have changed; compare values to detect "
                "stale results.")
    Q_CLASSINFO("doc.project", "The open project file and video mode.")
    Q_CLASSINFO("doc.player", "Preview state and playback controls.")
    Q_CLASSINFO("doc.timeline", "Timeline tracks, clips, selection, and edits.")
    Q_CLASSINFO("doc.playlist", "The project source bin.")
    Q_CLASSINFO("doc.markers", "Timeline point and range markers.")
    Q_CLASSINFO("doc.filters", "Filter metadata and static edits on clips, tracks, and output.")
    Q_CLASSINFO("doc.undo", "Explicit synchronous script undo grouping.")
    Q_CLASSINFO("doc.jobs", "Job status, cancellation, export presets and timeline export launch.")
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString apiVersion READ apiVersion CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(Shotcut::Api::ApiProject *project READ project CONSTANT)
    Q_PROPERTY(Shotcut::Api::ApiPlayer *player READ player CONSTANT)
    Q_PROPERTY(Shotcut::Api::ApiTimeline *timeline READ timeline CONSTANT)
    Q_PROPERTY(Shotcut::Api::ApiPlaylist *playlist READ playlist CONSTANT)
    Q_PROPERTY(Shotcut::Api::ApiMarkers *markers READ markers CONSTANT)
    Q_PROPERTY(Shotcut::Api::ApiFilters *filters READ filters CONSTANT)
    Q_PROPERTY(Shotcut::Api::ApiUndo *undo READ undo CONSTANT)
    Q_PROPERTY(Shotcut::Api::ApiJobs *jobs READ jobs CONSTANT)

public:
    static ShotcutApi &singleton();

    QString version() const;
    QString apiVersion() const;
    int revision() const { return m_revision; }
    ApiProject *project() const { return m_project; }
    ApiPlayer *player() const { return m_player; }
    ApiTimeline *timeline() const { return m_timeline; }
    ApiPlaylist *playlist() const { return m_playlist; }
    ApiMarkers *markers() const { return m_markers; }
    ApiFilters *filters() const { return m_filters; }
    ApiUndo *undo() const { return m_undo; }
    ApiJobs *jobs() const { return m_jobs; }

    Q_CLASSINFO("doc.timecode", "Formats frames as a timecode string.")
    Q_CLASSINFO("arg.timecode.frames", "Frame count.")
    Q_CLASSINFO("arg.timecode.format",
                "smpte, smpte_ndf, clock, or frames; defaults to the user's time format setting.")
    Q_INVOKABLE QString timecode(int frames, const QString &format = QString()) const;
    Q_CLASSINFO("doc.frames", "Parses a timecode string into frames.")
    Q_CLASSINFO("arg.frames.timecode", "Timecode such as 00:00:04:00 or 00:00:04.000.")
    Q_INVOKABLE int frames(const QString &timecode) const;

signals:
    void revisionChanged();

private:
    explicit ShotcutApi(QObject *parent = nullptr);
    void bumpRevision();

    ApiProject *m_project;
    ApiPlayer *m_player;
    ApiTimeline *m_timeline;
    ApiPlaylist *m_playlist;
    ApiMarkers *m_markers;
    ApiFilters *m_filters;
    ApiUndo *m_undo;
    ApiJobs *m_jobs;
    int m_revision = 0;
    bool m_revisionNotifyPending = false;
};

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_SHOTCUTAPI_H
