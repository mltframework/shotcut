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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SHOTCUT_API_APIJOBS_H
#define SHOTCUT_API_APIJOBS_H

#include "api/apiobject.h"

class AbstractJob;

namespace Shotcut {
namespace Api {

class ApiJobs : public ApiObject
{
    Q_OBJECT
    Q_CLASSINFO("doc",
                "Shotcut Jobs queue and timeline exports. Jobs are not undoable. IDs last until "
                "jobs are removed or Shotcut exits.")
    Q_CLASSINFO("doc.count", "Number of jobs in the Jobs panel.")
    Q_PROPERTY(int count READ count NOTIFY changed)
public:
    explicit ApiJobs(QObject *parent = nullptr);
    ~ApiJobs();
    int count() const;

    Q_CLASSINFO("mcp.list", "read")
    Q_CLASSINFO("doc.list",
                "Lists jobs in queue order with id, label, target, type, status, progress (0-100), "
                "elapsedMs, canCancel, and failure message when available. States: pending, "
                "starting, running, paused, canceling, canceled, succeeded, failed.")
    Q_INVOKABLE QVariantList list() const;

    Q_CLASSINFO("mcp.job", "read")
    Q_CLASSINFO("doc.job", "Returns one job snapshot; fails if it has been removed.")
    Q_CLASSINFO("arg.job.id", "Session job ID.")
    Q_INVOKABLE QVariant job(const QString &id) const;

    Q_CLASSINFO("mcp.log", "read")
    Q_CLASSINFO("doc.log",
                "Returns the tail of a job log with id, text and truncated. May include process "
                "diagnostics.")
    Q_CLASSINFO("arg.log.id", "Session job ID.")
    Q_CLASSINFO("arg.log.maxCharacters", "Maximum tail length, 1-64000. Defaults to 8000.")
    Q_INVOKABLE QVariant log(const QString &id, int maxCharacters = 8000) const;

    Q_CLASSINFO("mcp.cancel", "action")
    Q_CLASSINFO("doc.cancel",
                "Cancels a pending or active job and returns its snapshot. Active cancellation "
                "completes asynchronously. Terminal jobs are unchanged. Does not delete output "
                "files. Not undoable; cannot run inside an edit transaction.")
    Q_CLASSINFO("arg.cancel.id", "Session job ID.")
    Q_INVOKABLE QVariant cancel(const QString &id);

    Q_CLASSINFO("mcp.exportPresets", "read")
    Q_CLASSINFO("doc.exportPresets",
                "Lists installed stock and custom Export presets with id, name, category, custom "
                "and extension hint. These are stored MLT consumer properties, not the values "
                "normalized by Export controls. IDs are not file paths.")
    Q_INVOKABLE QVariantList exportPresets();

    Q_CLASSINFO("mcp.startExport", "action")
    Q_CLASSINFO(
        "doc.startExport",
        "Queues a snapshot of the full timeline using an explicit preset and absolute local output "
        "path. Preset properties are passed directly to the consumer; no Export UI conversion, "
        "hardware encoder substitution, or saved encoder preferences are applied. Missing "
        "properties use MLT/FFmpeg defaults except the explicit software default preset. "
        "meta.preset.* and underscore-prefixed metadata are stripped. v2pass=true or pass=1/2 "
        "requests a two-job encode; conflicting or invalid values fail. Returns {jobIds, jobs, "
        "target}. Jobs follow queue scheduling; filters requiring analysis must be analyzed "
        "beforehand. No source/bin, streaming, image sequences or range exports. Not undoable; "
        "cannot run inside an edit transaction. Poll jobs.job or jobs.list for completion.")
    Q_CLASSINFO("arg.startExport.target",
                "Absolute local output filename with extension. Parent folder must exist. Must not "
                "be project media or the project file.")
    Q_CLASSINFO("arg.startExport.presetId", "ID from exportPresets, or default.")
    Q_CLASSINFO("arg.startExport.overwrite",
                "boolean|Allow replacing an existing output file. Defaults to false. Cannot "
                "replace a file used in the project or by an active job.")
    Q_INVOKABLE QVariant startExport(const QString &target,
                                     const QString &presetId,
                                     const QVariantMap &options = {});
signals:
    void changed();

private:
    AbstractJob *find(const QString &id) const;
    QVariantMap snapshot(AbstractJob *job) const;
    void observe(AbstractJob *job);
    bool m_launching = false;
};

} // namespace Api
} // namespace Shotcut
#endif
