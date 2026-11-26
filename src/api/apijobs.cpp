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
#include "api/apijobs.h"

#include "api/transaction.h"
#include "exportpreset.h"
#include "jobqueue.h"
#include "jobs/encodejob.h"
#include "jobs/exportjobbuilder.h"
#include "mainwindow.h"
#include "settings.h"

#include <MltTractor.h>
#include <QDir>
#include <QFileInfo>
#include <QScopedValueRollback>
#include <QtMath>

namespace Shotcut {
namespace Api {

static bool usesDifferentProfile(Mlt::Properties &properties)
{
    if (properties.get("width") && properties.get_int("width") != MLT.profile().width())
        return true;
    if (properties.get("height") && properties.get_int("height") != MLT.profile().height())
        return true;
    if (properties.get("aspect")
        && !qFuzzyCompare(properties.get_double("aspect") + 1.0, MLT.profile().dar() + 1.0))
        return true;
    if (properties.get("progressive")
        && properties.get_int("progressive") != MLT.profile().progressive())
        return true;
    if (properties.get("frame_rate_num") && properties.get("frame_rate_den")) {
        return properties.get_int("frame_rate_num") != MLT.profile().frame_rate_num()
               || properties.get_int("frame_rate_den") != MLT.profile().frame_rate_den();
    }
    if (properties.get("r"))
        return !qFuzzyCompare(properties.get_double("r") + 1.0, MLT.profile().fps() + 1.0);
    return false;
}

ApiJobs::ApiJobs(QObject *parent)
    : ApiObject(parent)
{
    for (auto *job : JOBS.jobs())
        observe(job);
    connect(&JOBS, &JobQueue::jobAdded, this, [this]() {
        observe(JOBS.jobs().last());
        emit changed();
    });
    connect(&JOBS, &QAbstractItemModel::rowsRemoved, this, &ApiJobs::changed, Qt::QueuedConnection);
}

ApiJobs::~ApiJobs() = default;

void ApiJobs::observe(AbstractJob *job)
{
    connect(job, &AbstractJob::progressUpdated, this, &ApiJobs::changed);
    connect(job, &AbstractJob::finished, this, &ApiJobs::changed);
    connect(job, &QProcess::stateChanged, this, &ApiJobs::changed);
}

int ApiJobs::count() const
{
    return JOBS.jobs().size();
}

AbstractJob *ApiJobs::find(const QString &id) const
{
    for (auto *job : JOBS.jobs())
        if (job->id() == id)
            return job;
    fail(ErrorCode::NotFound, tr("Job does not exist: %1").arg(id));
    return nullptr;
}

QVariantMap ApiJobs::snapshot(AbstractJob *job) const
{
    const auto status = job->status();
    QVariantMap result{{"id", job->id()},
                       {"label", job->label()},
                       {"target", job->target()},
                       {"type",
                        job->property("jobType").isValid() ? job->property("jobType").toString()
                        : qobject_cast<EncodeJob *>(job)   ? "export"
                                                           : "job"},
                       {"status", status},
                       {"progress", job->progress()},
                       {"elapsedMs", job->elapsedMs()},
                       {"canCancel",
                        status == "pending" || status == "starting" || status == "running"
                            || status == "paused"}};
    if (status == "failed")
        result.insert("error", job->failureMessage());
    if (job->property("analysisState").isValid()) {
        result.insert("analysisState", job->property("analysisState"));
        if (job->property("analysisError").isValid())
            result.insert("analysisError", job->property("analysisError"));
    }
    return result;
}

QVariantList ApiJobs::list() const
{
    QVariantList result;
    for (auto *job : JOBS.jobs())
        result.append(snapshot(job));
    return result;
}

QVariant ApiJobs::job(const QString &id) const
{
    auto *item = find(id);
    return item ? QVariant(snapshot(item)) : QVariant();
}

QVariant ApiJobs::log(const QString &id, int maxCharacters) const
{
    if (maxCharacters < 1 || maxCharacters > 64000)
        return fail(ErrorCode::InvalidArgument, tr("Log length must be between 1 and 64000."));
    auto *item = find(id);
    if (!item)
        return {};
    const auto text = item->log();
    return QVariantMap{{"id", id},
                       {"text", text.right(maxCharacters)},
                       {"truncated", text.size() > maxCharacters}};
}

QVariant ApiJobs::cancel(const QString &id)
{
    if (Transaction::singleton().isOpen())
        return fail(ErrorCode::Busy, tr("Cancel jobs outside an edit transaction."));
    auto *item = find(id);
    if (!item)
        return {};
    item->cancel();
    emit changed();
    return snapshot(item);
}

QVariantList ApiJobs::exportPresets()
{
    return ExportPreset::list(QDir(Settings.appDataLocation()).filePath("presets/encode"));
}

QVariant ApiJobs::startExport(const QString &target,
                              const QString &presetId,
                              const QVariantMap &options)
{
    ApiCallGuard guard;
    if (Transaction::singleton().isOpen())
        return fail(ErrorCode::Busy, tr("Start exports outside an edit transaction."));
    if (m_launching)
        return fail(ErrorCode::Busy, tr("An export request is already being prepared."));
    QScopedValueRollback<bool> launching(m_launching, true);
    if (!checkOptions("startExport", options))
        return {};
    if (!MAIN.isMultitrackValid() || MAIN.multitrack()->get_playtime() <= 0)
        return fail(ErrorCode::Unavailable, tr("A nonempty timeline is required."));
    const QFileInfo file(target);
    if (target.contains(QChar::Null) || !file.isAbsolute() || file.suffix().isEmpty()
        || file.isDir())
        return fail(ErrorCode::InvalidArgument,
                    tr("Give an absolute local output filename with extension."));
    if (!file.dir().exists())
        return fail(ErrorCode::NotFound, tr("Output folder does not exist."));
    ExportPreset::ResolvedPreset preset;
    QString error;
    if (!ExportPreset::resolve(presetId,
                               preset,
                               error,
                               QDir(Settings.appDataLocation()).filePath("presets/encode"),
                               MLT.audioChannels()))
        return fail(ErrorCode::InvalidArgument, error);
    if (preset.imageSequence)
        return fail(ErrorCode::Unavailable,
                    tr("Image sequence exports are not supported by the API."));
    const QString path = file.exists()
                             ? file.canonicalFilePath()
                             : QDir::cleanPath(file.dir().canonicalPath() + '/' + file.fileName());
    const QFileInfo project(MAIN.fileName());
    if (!MAIN.fileName().isEmpty()
        && (path == project.absoluteFilePath()
            || (file.exists() && file.canonicalFilePath() == project.canonicalFilePath())))
        return fail(ErrorCode::InvalidArgument, tr("Cannot export over the project file."));
    if (file.exists() && !options.value("overwrite", false).toBool())
        return fail(ErrorCode::Unavailable,
                    tr("Output already exists; use overwrite: true to replace it."));
    for (auto *job : JOBS.jobs()) {
        const QFileInfo jobTarget(job->target());
        const QString activePath = jobTarget.exists()
                                       ? jobTarget.canonicalFilePath()
                                       : QDir::cleanPath(jobTarget.dir().canonicalPath() + '/'
                                                         + jobTarget.fileName());
        const auto status = job->status();
        if (status != "succeeded" && status != "failed" && status != "canceled"
            && activePath == path)
            return fail(ErrorCode::Busy, tr("A job already uses this output path."));
    }
    ExportJobOptions jobOptions;
    jobOptions.priority = Settings.jobPriority();
    jobOptions.includeSubtitles = true;
    jobOptions.useMultiConsumer = usesDifferentProfile(preset.properties);
    jobOptions.useMultiConsumerForReframe = jobOptions.useMultiConsumer;
    jobOptions.validateProjectXml = true;
    jobOptions.preservePresetProperties = true;
    jobOptions.useHardwareDecoder = false;
    jobOptions.pass = preset.twoPass ? 1 : 0;
    auto *first
        = ExportJobBuilder::create(MAIN.multitrack(), path, preset.properties, jobOptions, error);
    if (!first)
        return fail(ErrorCode::Unavailable, error);
    QList<MeltJob *> jobs{first};
    if (preset.twoPass) {
        jobOptions.pass = 2;
        auto *second = ExportJobBuilder::create(MAIN.multitrack(),
                                                path,
                                                preset.properties,
                                                jobOptions,
                                                error);
        if (!second) {
            delete first;
            return fail(ErrorCode::Unavailable, error);
        }
        jobs.append(second);
    }
    QStringList ids;
    for (auto *job : jobs) {
        ids.append(job->id());
        JOBS.add(job);
    }
    QVariantList snapshots;
    for (auto *job : jobs)
        snapshots.append(snapshot(job));
    return QVariantMap{{"jobIds", ids}, {"jobs", snapshots}, {"target", path}};
}

} // namespace Api
} // namespace Shotcut
