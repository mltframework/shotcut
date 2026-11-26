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

#include "apifilters.h"

#include "Logger.h"
#include "api/apicatalog.h"
#include "api/filteranimation.h"
#include "api/transaction.h"
#include "commands/filtercommands.h"
#include "controllers/filtercontroller.h"
#include "docks/timelinedock.h"
#include "jobqueue.h"
#include "jobs/meltjob.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/multitrackmodel.h"
#include "qmltypes/qmlfilter.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <MltFilter.h>
#include <MltPlaylist.h>
#include <memory>
#include <QDomDocument>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QUndoStack>

#include <cmath>
#include <limits>
#include <memory>

namespace Shotcut {
namespace Api {

namespace {

bool reservedProperty(const QString &name)
{
    return name.isEmpty() || name.contains(QChar::Null) || name.startsWith('_')
           || name.startsWith("shotcut:") || name.startsWith("mlt_") || name.startsWith("meta.")
           || name == "id" || name == "in" || name == "out" || name == "length"
           || name == "disable";
}

bool animatedValue(const QString &value)
{
    static const QRegularExpression expression(
        QStringLiteral("(^|;)\\s*-?\\d+(?:[.:]\\d+)*\\s*[|~!]?="));
    return expression.match(value).hasMatch();
}

QVariantMap scalarMetadata(Mlt::Properties &properties)
{
    QVariantMap result;
    if (properties.is_valid()) {
        for (int i = 0; i < properties.count(); ++i) {
            const char *name = properties.get_name(i);
            if (name && name[0] != '_' && !properties.get_data(name)) {
                if (const char *value = properties.get(name))
                    result.insert(QString::fromUtf8(name), QString::fromUtf8(value));
            }
        }
    }
    return result;
}

QVariantMap nativeMetadata(const QString &service)
{
    std::unique_ptr<Mlt::Properties> metadata(
        MLT.repository()->metadata(mlt_service_filter_type, service.toUtf8().constData()));
    if (!metadata || !metadata->is_valid())
        return {};
    auto result = scalarMetadata(*metadata);
    for (const char *name : {"tags", "image_formats"}) {
        Mlt::Properties sequence(metadata->get_data(name));
        if (sequence.is_valid()) {
            QVariantList values;
            for (int i = 0; i < sequence.count(); ++i)
                values.append(QString::fromUtf8(sequence.get(i)));
            result.insert(QString::fromLatin1(name), values);
        }
    }
    Mlt::Properties parameters(metadata->get_data("parameters"));
    if (parameters.is_valid()) {
        QVariantList items;
        for (int i = 0; i < parameters.count(); ++i) {
            Mlt::Properties parameter(parameters.get_data(parameters.get_name(i)));
            auto item = scalarMetadata(parameter);
            if (!item.contains("identifier"))
                item.insert("identifier", QString::fromUtf8(parameters.get_name(i)));
            if (parameter.is_valid()) {
                Mlt::Properties values(parameter.get_data("values"));
                if (values.is_valid()) {
                    QVariantList options;
                    for (int j = 0; j < values.count(); ++j)
                        options.append(QString::fromUtf8(values.get(j)));
                    item.insert("values", options);
                }
            }
            items.append(item);
        }
        result.insert("parameters", items);
    }
    return result;
}

QVariantMap parameterMetadata(Mlt::Service &service)
{
    QVariantMap result;
    const auto native = nativeMetadata(QString::fromUtf8(service.get("mlt_service")));
    for (const auto &value : native.value("parameters").toList()) {
        const auto parameter = value.toMap();
        result.insert(parameter.value("identifier").toString(), parameter);
    }
    return result;
}

bool declaredTrue(const QVariant &value)
{
    const auto text = value.toString().toLower();
    return text == "1" || text == "true" || text == "yes";
}

bool supportsAnalysis(const QString &service)
{
    return service == "loudness" || service == "vidstab" || service == "opencv.tracker";
}

MeltJob *createAnalysisJob(
    Mlt::Producer &producer, Mlt::Filter &filter, int in, int out, QString &error)
{
    error.clear();
    int rawIndex = -1;
    for (int i = 0; i < producer.filter_count(); ++i) {
        std::unique_ptr<Mlt::Filter> candidate(producer.filter(i));
        if (candidate->get_service() == filter.get_service()) {
            rawIndex = i;
            break;
        }
    }
    if (rawIndex < 0 || !supportsAnalysis(QString::fromUtf8(filter.get("mlt_service")))
        || out < in) {
        error = QObject::tr("The filter cannot be analyzed on this producer.");
        return nullptr;
    }
    Mlt::Producer copy(MLT.profile(), "xml-string", MLT.XML(&producer).toUtf8().constData());
    if (!copy.is_valid() || rawIndex >= copy.filter_count()) {
        error = QObject::tr("Cannot copy the analysis producer.");
        return nullptr;
    }
    std::unique_ptr<Mlt::Filter> analysis(copy.filter(rawIndex));
    if (QString::fromUtf8(analysis->get("mlt_service"))
        != QString::fromUtf8(filter.get("mlt_service"))) {
        error = QObject::tr("The analysis filter changed while copying the producer.");
        return nullptr;
    }
    const QString uuid = QUuid::createUuid().toString();
    analysis->set(kShotcutHashProperty, uuid.toUtf8().constData());
    analysis->clear("results");
    analysis->set("disable", 0);
    analysis->set_in_and_out(in, out);
    const bool audio = QString::fromUtf8(filter.get("mlt_service")) == "loudness";
    if (!audio)
        analysis->set("analyze", 1);
    if (QString::fromUtf8(filter.get("mlt_service")) == "opencv.tracker")
        analysis->set("_reset", 1);
    Mlt::Playlist render(MLT.profile());
    render.append(copy, in, out);
    QDomDocument dom;
    const auto parsed = dom.setContent(MLT.XML(&render, true));
    if (!parsed) {
        error = QObject::tr("Invalid analysis XML: %1").arg(parsed.errorMessage);
        return nullptr;
    }
    auto output = std::make_unique<QTemporaryFile>(Util::writableTemporaryFile({}));
    if (!output->open()) {
        error = QObject::tr("Cannot create analysis output: %1").arg(output->errorString());
        return nullptr;
    }
    output->close();
    auto consumer = dom.createElement("consumer");
    consumer.setAttribute("mlt_service", "xml");
    consumer.setAttribute("all", 1);
    consumer.setAttribute(audio ? "video_off" : "audio_off", 1);
    consumer.setAttribute("resource", output->fileName());
    const auto profiles = dom.elementsByTagName("profile");
    dom.documentElement().insertAfter(consumer, profiles.isEmpty() ? QDomNode() : profiles.at(0));
    auto *job = new MeltJob(output->fileName(),
                            dom.toString(2),
                            MLT.profile().frame_rate_num(),
                            MLT.profile().frame_rate_den(),
                            Settings.jobPriority(),
                            false);
    auto environment = QProcessEnvironment::systemEnvironment();
    // Keep analyzed filters in result XML instead of a reference to the input XML file.
    environment.insert("MLT_XML_DEEP", "1");
    job->setProcessEnvironment(environment);
    job->setProperty("analysisUuid", uuid);
    job->setProperty("analysisState", "running");
    job->setProperty("jobType", "analysis");
    output.release()->setParent(job);
    return job;
}

} // namespace

ApiFilters::ApiFilters(QObject *parent)
    : ApiObject(parent)
{
    connect(MAIN.filterController()->attachedModel(),
            &AttachedFiltersModel::changed,
            this,
            &ApiFilters::changed);
}

QmlMetadata *ApiFilters::filterMetadata(const QString &id) const
{
    auto meta = MAIN.filterController()->metadata(id);
    if (!meta || meta->type() != QmlMetadata::Filter) {
        fail(ErrorCode::NotFound, tr("Shotcut filter metadata ID not found: %1.").arg(id));
        return nullptr;
    }
    return meta;
}

QVariantMap ApiFilters::metadataSnapshot(QmlMetadata *meta) const
{
    QVariantList keyframeParameters;
    for (int i = 0; i < meta->keyframes()->parameterCount(); ++i) {
        const auto *parameter = meta->keyframes()->parameter(i);
        if (meta->keyframes()->property("enabled").toBool() && !parameter->isColor()
            && !reservedProperty(parameter->property()))
            keyframeParameters.append(QVariantMap{{"property", parameter->property()},
                                                  {"rectangle", parameter->isRectangle()}});
    }
    return {{"metadataId", meta->uniqueId()},
            {"keyframeParameters", keyframeParameters},
            {"name", meta->name()},
            {"service", meta->mlt_service()},
            {"supportsAnalysis", supportsAnalysis(meta->mlt_service())},
            {"isAudio", meta->isAudio()},
            {"needsGPU", meta->needsGPU()},
            {"isGpuCompatible", meta->isGpuCompatible()},
            {"isHidden", meta->isHidden()},
            {"isDeprecated", meta->isDeprecated()},
            {"allowMultiple", meta->allowMultiple()},
            {"isClipOnly", meta->isClipOnly()},
            {"isTrackOnly", meta->isTrackOnly()},
            {"isOutputOnly", meta->isOutputOnly()}};
}

QVariantList ApiFilters::available() const
{
    QVariantList result;
    auto model = MAIN.filterController()->metadataModel();
    for (int i = 0; i < model->sourceRowCount(); ++i) {
        auto meta = model->getFromSource(i);
        if (meta->type() == QmlMetadata::Filter)
            result.append(metadataSnapshot(meta));
    }
    return result;
}

QVariant ApiFilters::metadata(const QString &metadataId) const
{
    auto meta = filterMetadata(metadataId);
    if (!meta)
        return {};
    auto result = metadataSnapshot(meta);
    const auto native = nativeMetadata(meta->mlt_service());
    result.insert("mlt", native);
    result.insert("metadataAvailable", !native.isEmpty());
    return result;
}

bool ApiFilters::resolveTarget(const QVariantMap &target, Target &result, bool writable) const
{
    if (target.size() != 1) {
        fail(ErrorCode::InvalidArgument,
             tr("Give exactly one clipId, trackIndex, or output: true."));
        return false;
    }
    auto dock = MAIN.timelineDock();
    auto model = dock->model();
    if (!model->tractor()) {
        fail(ErrorCode::Unavailable, tr("A timeline is required for filter targets."));
        return false;
    }
    int trackIndex = -1;
    if (target.contains("clipId") && matchesJsonType(target.value("clipId"), "string")) {
        const auto id = QUuid::fromString(target.value("clipId").toString());
        if (id.isNull()) {
            fail(ErrorCode::InvalidArgument, tr("Invalid clip ID."));
            return false;
        }
        int clipIndex = -1;
        auto info = model->findClipByUuid(id, trackIndex, clipIndex);
        if (!info) {
            fail(ErrorCode::NotFound, tr("The target clip does not exist."));
            return false;
        }
        const auto index = model->makeIndex(trackIndex, clipIndex);
        if (info->cut->is_blank()
            || model->data(index, MultitrackModel::IsTransitionRole).toBool()) {
            fail(ErrorCode::Unavailable, tr("Blanks and transitions cannot be filter targets."));
            return false;
        }
        result.producer = Mlt::Producer(info->producer);
        const auto range = model->getClipFilterRange(trackIndex, clipIndex, *info);
        result.in = range.first;
        result.out = range.second;
        result.kind = "clip";
    } else if (target.contains("trackIndex")
               && matchesJsonType(target.value("trackIndex"), "integer")) {
        const double value = target.value("trackIndex").toDouble();
        if (value < 0 || value > std::numeric_limits<int>::max()) {
            fail(ErrorCode::InvalidArgument, tr("Track index is out of range."));
            return false;
        }
        trackIndex = target.value("trackIndex").toInt();
        if (trackIndex < 0 || trackIndex >= model->trackList().size()) {
            fail(ErrorCode::NotFound, tr("The target track does not exist."));
            return false;
        }
        std::unique_ptr<Mlt::Producer> track(
            model->tractor()->track(model->trackList()[trackIndex].mlt_index));
        result.producer = Mlt::Producer(track.get());
        result.in = 0;
        result.out = result.producer.get_playtime() - 1;
        result.kind = "track";
    } else if (target.contains("output") && matchesJsonType(target.value("output"), "boolean")
               && target.value("output").toBool()) {
        result.producer = Mlt::Producer(model->tractor());
        result.in = 0;
        result.out = result.producer.get_playtime() - 1;
        result.kind = "output";
    } else {
        fail(ErrorCode::InvalidArgument,
             tr("Target must be clipId: string, trackIndex: integer, or output: true."));
        return false;
    }
    if (!result.producer.is_valid()) {
        fail(ErrorCode::Unavailable, tr("The filter target cannot be read."));
        return false;
    }
    if (writable && trackIndex >= 0 && dock->isTrackLocked(trackIndex)) {
        fail(ErrorCode::Unavailable, tr("The target track is locked."));
        return false;
    }
    return true;
}

bool ApiFilters::checkFilter(Mlt::Producer &producer,
                             int index,
                             Mlt::Service &service,
                             bool writable) const
{
    auto model = MAIN.filterController()->attachedModel();
    if (index < 0 || index >= model->serviceCount(producer)) {
        fail(ErrorCode::NotFound, tr("Filter row %1 does not exist.").arg(index));
        return false;
    }
    service = model->doGetService(producer, index);
    if (!service.is_valid() || service.type() != mlt_service_filter_type) {
        fail(ErrorCode::Unavailable, tr("This row is not a regular filter."));
        return false;
    }
    if (writable && service.get(kShotcutTrackVolumeProperty)) {
        fail(ErrorCode::Unavailable,
             tr("Use timeline track gain controls for the track volume filter."));
        return false;
    }
    return true;
}

QVariantMap ApiFilters::filterSnapshot(Mlt::Producer &producer, int index) const
{
    auto service = MAIN.filterController()->attachedModel()->doGetService(producer, index);
    const auto descriptors = parameterMetadata(service);
    QVariantMap parameters;
    for (int i = 0; i < service.count(); ++i) {
        const auto name = QString::fromUtf8(service.get_name(i));
        const auto key = name.toUtf8();
        if (reservedProperty(name) || service.get_data(key.constData())
            || !service.get(key.constData()))
            continue;
        const auto type = descriptors.value(name).toMap().value("type").toString();
        QVariant value = QString::fromUtf8(service.get(key.constData()));
        if (!animatedValue(value.toString()) && !service.is_anim(key.constData())) {
            if (type == "integer")
                value = service.get_int(key.constData());
            else if (type == "float")
                value = service.get_double(key.constData());
            else if (type == "boolean")
                value = service.get_int(key.constData()) != 0;
            else if (type == "rect") {
                const auto rect = service.get_rect(key.constData());
                value = QVariantMap{{"x", rect.x},
                                    {"y", rect.y},
                                    {"width", rect.w},
                                    {"height", rect.h},
                                    {"opacity", rect.o}};
            }
        }
        parameters.insert(name, value);
    }
    auto meta = MAIN.filterController()->metadataForService(&service);
    return {{"index", index},
            {"metadataId",
             meta ? meta->uniqueId() : QString::fromUtf8(service.get(kShotcutFilterProperty))},
            {"name", meta ? meta->name() : QString::fromUtf8(service.get("mlt_service"))},
            {"service", QString::fromUtf8(service.get("mlt_service"))},
            {"enabled", service.get_int("disable") == 0},
            {"in", service.get_int("in")},
            {"out", service.get_int("out")},
            {"readOnly", service.get(kShotcutTrackVolumeProperty) != nullptr},
            {"parameters", parameters}};
}

QVariantList ApiFilters::list(const QVariantMap &target) const
{
    Target resolved;
    if (!resolveTarget(target, resolved, false))
        return {};
    QVariantList result;
    auto model = MAIN.filterController()->attachedModel();
    for (int i = 0; i < model->serviceCount(resolved.producer); ++i) {
        auto service = model->doGetService(resolved.producer, i);
        if (service.type() == mlt_service_filter_type)
            result.append(filterSnapshot(resolved.producer, i));
    }
    return result;
}

bool ApiFilters::prepareParameters(Mlt::Service &service,
                                   const QVariantMap &parameters,
                                   Mlt::Properties &values) const
{
    const auto descriptors = parameterMetadata(service);
    for (auto it = parameters.cbegin(); it != parameters.cend(); ++it) {
        const auto name = it.key();
        const auto key = name.toUtf8();
        const auto descriptor = descriptors.value(name).toMap();
        const auto type = descriptor.value("type").toString();
        if (name == "analyze" && supportsAnalysis(QString::fromUtf8(service.get("mlt_service")))) {
            fail(ErrorCode::InvalidArgument, tr("Use filters.analyze() to start filter analysis."));
            return false;
        }
        if (reservedProperty(name) || declaredTrue(descriptor.value("readonly"))) {
            fail(ErrorCode::InvalidArgument,
                 tr("Filter property %1 is reserved or read-only.").arg(name));
            return false;
        }
        if (service.is_anim(key.constData())
            || animatedValue(QString::fromUtf8(service.get(key.constData())))
            || (it.value().metaType().id() == QMetaType::QString
                && animatedValue(it.value().toString()))) {
            fail(ErrorCode::Unavailable,
                 tr("Animated parameter %1 is not supported by static edits.").arg(name));
            return false;
        }
        const auto value = it.value();
        const bool boolean = value.metaType().id() == QMetaType::Bool;
        const bool number = matchesJsonType(value, "number") && !boolean;
        const bool string = value.metaType().id() == QMetaType::QString;
        const bool rectangle = value.metaType().id() == QMetaType::QVariantMap;
        if ((!type.isEmpty()
             && ((type == "integer" && !boolean && !matchesJsonType(value, "integer"))
                 || (type == "float" && !number) || (type == "boolean" && !boolean)
                 || ((type == "string" || type == "color") && !string)
                 || (type == "rect" && !rectangle && !string)))
            || (!boolean && !number && !string && !rectangle)
            || (number
                && (!std::isfinite(value.toDouble())
                    || (type == "integer"
                        && (value.toDouble() < std::numeric_limits<int>::min()
                            || value.toDouble() > std::numeric_limits<int>::max()))))
            || (string && value.toString().contains(QChar::Null))) {
            fail(ErrorCode::InvalidArgument,
                 tr("Invalid static value or MLT type for parameter %1 (%2).").arg(name, type));
            return false;
        }
        if (rectangle) {
            const auto rect = value.toMap();
            const QStringList keys{"x", "y", "width", "height", "opacity"};
            bool valid = rect.size() == keys.size();
            for (const auto &field : keys)
                valid = valid && rect.contains(field)
                        && matchesJsonType(rect.value(field), "number")
                        && std::isfinite(rect.value(field).toDouble());
            if (!valid) {
                fail(ErrorCode::InvalidArgument,
                     tr("Rectangle %1 requires finite x, y, width, height, and opacity.").arg(name));
                return false;
            }
            values.set(key.constData(),
                       rect.value("x").toDouble(),
                       rect.value("y").toDouble(),
                       rect.value("width").toDouble(),
                       rect.value("height").toDouble(),
                       rect.value("opacity").toDouble());
        } else if (boolean) {
            values.set(key.constData(), value.toBool() ? 1 : 0);
        } else if (number) {
            values.set(key.constData(), value.toDouble());
        } else {
            values.set_string(key.constData(), value.toString().toUtf8().constData());
        }
    }
    return true;
}

QVariant ApiFilters::editFailed() const
{
    const auto message = tr(
        "The filter edit could not be applied as requested; the edits are rolled back.");
    LOG_ERROR() << message;
    Transaction::singleton().fail(message);
    return fail(ErrorCode::Unavailable, message);
}

QVariant ApiFilters::add(const QVariantMap &target,
                         const QString &metadataId,
                         const QVariantMap &parameters)
{
    ApiCallGuard callGuard;
    Target resolved;
    if (!resolveTarget(target, resolved, true))
        return {};
    auto meta = filterMetadata(metadataId);
    if (!meta)
        return {};
    if ((meta->isClipOnly() && resolved.kind != "clip")
        || (meta->isTrackOnly() && resolved.kind != "track")
        || (meta->isOutputOnly() && resolved.kind != "output")
        || (meta->needsGPU() && !Settings.playerGPU())
        || (!meta->isGpuCompatible() && Settings.playerGPU()) || meta->isHidden()) {
        return fail(ErrorCode::Unavailable,
                    tr("This filter is not available for the target or current GPU mode."));
    }
    auto model = MAIN.filterController()->attachedModel();
    for (int i = 0; !meta->allowMultiple() && i < model->serviceCount(resolved.producer); ++i) {
        auto service = model->doGetService(resolved.producer, i);
        auto attachedMeta = MAIN.filterController()->metadataForService(&service);
        if (attachedMeta && attachedMeta->uniqueId() == metadataId)
            return fail(ErrorCode::Unavailable,
                        tr("Only one %1 filter is allowed on this target.").arg(meta->name()));
    }
    Mlt::Filter filter(MLT.profile(), meta->mlt_service().toUtf8().constData());
    if (!filter.is_valid())
        return fail(ErrorCode::Unavailable, tr("MLT could not create this filter."));
    Mlt::Properties values;
    if (!prepareParameters(filter, parameters, values))
        return {};
    filter.inherit(values);
    if (supportsAnalysis(meta->mlt_service()) && !meta->isAudio())
        filter.set("analyze", 0);
    if (!meta->objectName().isEmpty())
        filter.set(kShotcutFilterProperty, meta->objectName().toUtf8().constData());
    filter.set_in_and_out(resolved.in, resolved.out);
    const int count = model->serviceCount(resolved.producer);
    const int index = model->findInsertRow(resolved.producer, meta);
    Transaction::singleton().touch();
    MAIN.undoStack()->push(
        new Filter::AddCommand(*model, resolved.producer, meta->name(), filter, index));
    if (model->serviceCount(resolved.producer) != count + 1)
        return editFailed();
    const auto result = filterSnapshot(resolved.producer, index);
    if (result.value("metadataId") != metadataId)
        return editFailed();
    return result;
}

QVariant ApiFilters::remove(const QVariantMap &target, int index)
{
    ApiCallGuard callGuard;
    Target resolved;
    Mlt::Service service;
    if (!resolveTarget(target, resolved, true)
        || !checkFilter(resolved.producer, index, service, true))
        return {};
    const auto result = filterSnapshot(resolved.producer, index);
    auto model = MAIN.filterController()->attachedModel();
    const int count = model->serviceCount(resolved.producer);
    Transaction::singleton().touch();
    MAIN.undoStack()->push(new Filter::RemoveCommand(*model,
                                                     resolved.producer,
                                                     result.value("name").toString(),
                                                     service,
                                                     index));
    if (model->serviceCount(resolved.producer) != count - 1)
        return editFailed();
    return result;
}

QVariant ApiFilters::move(const QVariantMap &target, int index, int toIndex)
{
    ApiCallGuard callGuard;
    Target resolved;
    Mlt::Service service, destination;
    if (!resolveTarget(target, resolved, true)
        || !checkFilter(resolved.producer, index, service, true)
        || !checkFilter(resolved.producer, toIndex, destination, true))
        return {};
    const auto result = filterSnapshot(resolved.producer, index);
    if (index != toIndex) {
        auto model = MAIN.filterController()->attachedModel();
        Transaction::singleton().touch();
        MAIN.undoStack()->push(new Filter::MoveCommand(*model,
                                                       resolved.producer,
                                                       result.value("name").toString(),
                                                       index,
                                                       toIndex));
        auto moved = model->doGetService(resolved.producer, toIndex);
        if (moved.get_service() != service.get_service())
            return editFailed();
    }
    return filterSnapshot(resolved.producer, toIndex);
}

QVariant ApiFilters::setEnabled(const QVariantMap &target, int index, bool enabled)
{
    ApiCallGuard callGuard;
    Target resolved;
    Mlt::Service service;
    if (!resolveTarget(target, resolved, true)
        || !checkFilter(resolved.producer, index, service, true))
        return {};
    if ((service.get_int("disable") == 0) != enabled) {
        Transaction::singleton().touch();
        MAIN.undoStack()->push(new Filter::DisableCommand(*MAIN.filterController()->attachedModel(),
                                                          resolved.producer,
                                                          filterSnapshot(resolved.producer, index)
                                                              .value("name")
                                                              .toString(),
                                                          index,
                                                          !enabled));
        if ((service.get_int("disable") == 0) != enabled)
            return editFailed();
    }
    return filterSnapshot(resolved.producer, index);
}

QVariant ApiFilters::setParameters(const QVariantMap &target,
                                   int index,
                                   const QVariantMap &parameters)
{
    ApiCallGuard callGuard;
    Target resolved;
    Mlt::Service service;
    if (!resolveTarget(target, resolved, true)
        || !checkFilter(resolved.producer, index, service, true))
        return {};
    Mlt::Properties values;
    if (!prepareParameters(service, parameters, values))
        return {};
    return applyParameters(resolved, index, values);
}

QVariant ApiFilters::applyParameters(Target &resolved, int index, Mlt::Properties &values)
{
    auto service = MAIN.filterController()->attachedModel()->doGetService(resolved.producer, index);
    bool changed = false;
    for (int i = 0; i < values.count(); ++i) {
        const char *name = values.get_name(i);
        changed = changed || !service.property_exists(name)
                  || QString::fromUtf8(service.get(name)) != QString::fromUtf8(values.get(name));
    }
    if (changed) {
        Transaction::singleton().touch();
        MAIN.undoStack()->push(new Filter::SetParametersCommand(
            *MAIN.filterController()->attachedModel(),
            resolved.producer,
            filterSnapshot(resolved.producer, index).value("name").toString(),
            index,
            values));
        for (int i = 0; i < values.count(); ++i) {
            const char *name = values.get_name(i);
            if (QString::fromUtf8(service.get(name)) != QString::fromUtf8(values.get(name)))
                return editFailed();
        }
    }
    return filterSnapshot(resolved.producer, index);
}

bool ApiFilters::animationParameter(Mlt::Service &service,
                                    const QString &parameter,
                                    bool &rectangle) const
{
    const auto *meta = MAIN.filterController()->metadataForService(&service);
    const auto *spec = meta && meta->keyframes()->property("enabled").toBool()
                           ? meta->keyframes()->parameter(parameter)
                           : nullptr;
    const auto descriptor = parameterMetadata(service).value(parameter).toMap();
    if (reservedProperty(parameter) || !spec || spec->isColor()
        || declaredTrue(descriptor.value("readonly"))) {
        fail(ErrorCode::Unavailable,
             tr("Not a supported numeric or rectangle keyframe parameter: %1.").arg(parameter));
        return false;
    }
    rectangle = spec->isRectangle();
    return true;
}

QVariant ApiFilters::keyframes(const QVariantMap &target, int index, const QString &parameter) const
{
    Target resolved;
    Mlt::Service service;
    bool rectangle = false;
    if (!resolveTarget(target, resolved, false)
        || !checkFilter(resolved.producer, index, service, false)
        || !animationParameter(service, parameter, rectangle))
        return {};
    const int duration = service.get_int("out") > 0 || service.get_int("in") > 0
                             ? service.get_int("out") - service.get_int("in") + 1
                             : resolved.out - resolved.in + 1;
    Mlt::Properties copy;
    copy.inherit(service);
    return QVariantMap{{"parameter", parameter},
                       {"rectangle", rectangle},
                       {"duration", duration},
                       {"keys", FilterAnimation::read(copy, parameter, rectangle, duration)}};
}

QVariant ApiFilters::setKeyframes(const QVariantMap &target,
                                  int index,
                                  const QString &parameter,
                                  const QVariantList &keys)
{
    ApiCallGuard guard;
    Target resolved;
    Mlt::Service service;
    bool rectangle = false;
    if (!resolveTarget(target, resolved, true)
        || !checkFilter(resolved.producer, index, service, true)
        || !animationParameter(service, parameter, rectangle))
        return {};
    const int duration = service.get_int("out") > 0 || service.get_int("in") > 0
                             ? service.get_int("out") - service.get_int("in") + 1
                             : resolved.out - resolved.in + 1;
    Mlt::Properties values;
    QString error;
    if (!FilterAnimation::write(values, parameter, rectangle, duration, keys, error))
        return fail(ErrorCode::InvalidArgument, error);
    values.set(kShotcutAnimInProperty, 0);
    values.set(kShotcutAnimOutProperty, 0);
    if (!applyParameters(resolved, index, values).isValid())
        return {};
    return keyframes(target, index, parameter);
}

QVariant ApiFilters::removeKeyframes(const QVariantMap &target,
                                     int index,
                                     const QString &parameter,
                                     const QVariantList &frames)
{
    ApiCallGuard guard;
    const auto snapshot = keyframes(target, index, parameter);
    if (!snapshot.isValid())
        return {};
    auto keys = snapshot.toMap().value("keys").toList();
    QSet<int> requested;
    for (const auto &frame : frames) {
        if (!matchesJsonType(frame, "integer") || frame.toDouble() < 0
            || frame.toDouble() >= snapshot.toMap().value("duration").toInt()
            || requested.contains(frame.toInt()))
            return fail(ErrorCode::InvalidArgument,
                        tr("Give unique existing integer keyframe frames."));
        requested.insert(frame.toInt());
    }
    if (requested.isEmpty())
        return fail(ErrorCode::InvalidArgument, tr("Give at least one keyframe frame."));
    QVariant lastValue;
    for (int i = keys.size() - 1; i >= 0; --i) {
        if (requested.remove(keys[i].toMap().value("frame").toInt())) {
            lastValue = keys[i].toMap().value("value");
            keys.removeAt(i);
        }
    }
    if (!requested.isEmpty())
        return fail(ErrorCode::NotFound, tr("A requested keyframe does not exist."));
    if (!keys.isEmpty())
        return setKeyframes(target, index, parameter, keys);
    Target resolved;
    Mlt::Service service;
    if (!resolveTarget(target, resolved, true)
        || !checkFilter(resolved.producer, index, service, true))
        return {};
    Mlt::Properties values;
    const auto key = parameter.toUtf8();
    if (snapshot.toMap().value("rectangle").toBool()) {
        const auto rect = lastValue.toMap();
        values.set(key.constData(),
                   rect.value("x").toDouble(),
                   rect.value("y").toDouble(),
                   rect.value("width").toDouble(),
                   rect.value("height").toDouble(),
                   rect.value("opacity").toDouble());
    } else {
        values.set(key.constData(), lastValue.toDouble());
    }
    values.set(kShotcutAnimInProperty, 0);
    values.set(kShotcutAnimOutProperty, 0);
    if (!applyParameters(resolved, index, values).isValid())
        return {};
    return keyframes(target, index, parameter);
}

QVariant ApiFilters::analyze(const QVariantMap &target, int index)
{
    if (Transaction::singleton().isOpen())
        return fail(ErrorCode::Busy, tr("Start analysis outside an edit transaction."));
    Target resolved;
    Mlt::Service service;
    if (!resolveTarget(target, resolved, true)
        || !checkFilter(resolved.producer, index, service, true))
        return {};
    const QString native = QString::fromUtf8(service.get("mlt_service"));
    if (!supportsAnalysis(native))
        return fail(ErrorCode::Unavailable,
                    tr("This filter does not use Shotcut's analysis pipeline."));
    for (auto *job : JOBS.jobs()) {
        const auto state = job->property("analysisState").toString();
        if (state == "running"
            && job->property("analysisService").value<quintptr>()
                   == reinterpret_cast<quintptr>(service.get_service()))
            return fail(ErrorCode::Busy, tr("Analysis is already running for this filter."));
    }
    if (native == "vidstab") {
        const QFileInfo filename(QString::fromUtf8(service.get("filename")));
        if (!filename.isAbsolute() || filename.exists() || !filename.dir().exists())
            return fail(ErrorCode::InvalidArgument,
                        tr("Set filename to an absolute, nonexistent stabilization output file."));
        QTemporaryFile probe(filename.dir().filePath(".shotcut-analysis-XXXXXX"));
        if (!probe.open())
            return fail(ErrorCode::Unavailable,
                        tr("Analysis output folder is not writable: %1").arg(probe.errorString()));
    }
    auto filter = std::make_shared<Mlt::Filter>(service);
    const int in = service.get_int("in") || service.get_int("out") ? service.get_int("in")
                                                                   : resolved.in;
    const int out = service.get_int("in") || service.get_int("out") ? service.get_int("out")
                                                                    : resolved.out;
    QString error;
    auto *job = createAnalysisJob(resolved.producer, *filter, in, out, error);
    if (!job)
        return fail(ErrorCode::Unavailable, error);
    if (native == "vidstab") {
        const QString filename = QString::fromUtf8(service.get("filename"));
        QFile reservation(filename);
        if (!reservation.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
            delete job;
            return fail(ErrorCode::Unavailable,
                        tr("Cannot reserve stabilization output: %1").arg(reservation.errorString()));
        }
        job->setProperty("analysisFilename", filename);
    }
    filter->set(kShotcutHashProperty, job->property("analysisUuid").toString().toUtf8().constData());
    job->setProperty("analysisService",
                     QVariant::fromValue(reinterpret_cast<quintptr>(service.get_service())));
    job->setLabel(
        tr("Analyze %1").arg(filterSnapshot(resolved.producer, index).value("name").toString()));
    auto *delegate = new AnalyzeDelegate(*filter);
    delegate->setParent(job);
    connect(
        job,
        &AbstractJob::finished,
        this,
        [this, delegate, filter](AbstractJob *job, bool isSuccess) {
            delegate->onAnalyzeFinished(job, isSuccess);
            if (job->property("analysisState").toString() == "applied")
                MAIN.filterController()->onFilterEdited(*filter, {QStringLiteral("results")});
            if (QString::fromUtf8(filter->get(kShotcutHashProperty))
                == job->property("analysisUuid").toString())
                filter->clear(kShotcutHashProperty);
            emit analysisChanged();
        },
        Qt::QueuedConnection);
    JOBS.add(job);
    return QVariantMap{{"jobId", job->id()}};
}

} // namespace Api
} // namespace Shotcut
