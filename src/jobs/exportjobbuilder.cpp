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

#include "exportjobbuilder.h"

#include "Logger.h"
#include "exportpreset.h"
#include "jobqueue.h"
#include "jobs/encodejob.h"
#include "mltcontroller.h"
#include "mltxmlchecker.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <MltFilter.h>
#include <MltProducer.h>
#include <MltProfile.h>
#include <MltService.h>
#include <memory>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>
#include <QTemporaryFile>
#include <QXmlStreamReader>

#ifdef Q_OS_WIN
static const QString kNullTarget = "nul";
#else
static const QString kNullTarget = "/dev/null";
#endif

static QPoint frameRate(Mlt::Properties &properties)
{
    bool ok = false;
    const int num = QString::fromLatin1(properties.get("frame_rate_num")).toInt(&ok);
    if (ok) {
        const int den = QString::fromLatin1(properties.get("frame_rate_den")).toInt();
        if (den > 0)
            return QPoint(num, den);
    }
    const double fps = QString::fromLatin1(properties.get("r")).toDouble(&ok);
    if (ok && fps > 0.0) {
        int numerator = 0;
        int denominator = 0;
        Util::normalizeFrameRate(fps, numerator, denominator);
        if (numerator > 0 && denominator > 0)
            return QPoint(numerator, denominator);
    }
    return QPoint(MLT.profile().frame_rate_num(), MLT.profile().frame_rate_den());
}

static void setSubtitleProperties(QDomElement &node, Mlt::Producer *service)
{
    if (!service)
        return;
    int subIndex = 0;
    for (int i = 0; i < service->filter_count(); ++i) {
        QScopedPointer<Mlt::Filter> filter(service->filter(i));
        if (filter && filter->is_valid()
            && filter->get("mlt_service") == QStringLiteral("subtitle_feed")) {
            node.setAttribute(QStringLiteral("subtitle.%1.feed").arg(subIndex), filter->get("feed"));
            node.setAttribute(QStringLiteral("subtitle.%1.lang").arg(subIndex), filter->get("lang"));
            ++subIndex;
        }
    }
}

static QPoint addConsumerElement(Mlt::Producer *service,
                                 QDomDocument &dom,
                                 const QString &target,
                                 Mlt::Properties &properties,
                                 const ExportJobOptions &options)
{
    QDomElement consumerNode = dom.createElement("consumer");
    const QDomNodeList profiles = dom.elementsByTagName("profile");
    if (profiles.isEmpty())
        dom.documentElement().insertAfter(consumerNode, dom.documentElement());
    else
        dom.documentElement().insertAfter(consumerNode, profiles.at(profiles.length() - 1));
    consumerNode.setAttribute("mlt_service", "avformat");
    consumerNode.setAttribute("target", options.pass == 1 ? kNullTarget : target);
    for (int i = 0; i < properties.count(); ++i) {
        const char *name = properties.get_name(i);
        if (name && *name)
            consumerNode.setAttribute(QString::fromLatin1(name), properties.get(i));
    }
    if (!properties.get("real_time") && options.realtime)
        consumerNode.setAttribute("real_time", options.realtime);

    const QString vcodec = QString::fromLatin1(properties.get("vcodec"));
    ExportPreset::configurePass(consumerNode,
                                target,
                                vcodec,
                                options.pass,
                                options.preservePresetProperties);
    if (options.strictFormatCompatibility && (target.endsWith(".mp4") || target.endsWith(".mov")))
        consumerNode.setAttribute("strict", "experimental");
    if (options.includeSubtitles)
        setSubtitleProperties(consumerNode, service);
    return frameRate(properties);
}

static MeltJob *convertReframe(Mlt::Producer *service,
                               QTemporaryFile *tmp,
                               const QString &target,
                               Mlt::Properties &properties,
                               const ExportJobOptions &options)
{
    for (int i = 0; i < service->filter_count(); ++i) {
        Mlt::Filter filter(service->filter(i));
        if (::qstrcmp("reframe", filter.get(kShotcutFilterProperty)) || filter.get_int("disable"))
            continue;

        auto rect = filter.anim_get_rect("rect", 0);
        LOG_DEBUG() << "Found Reframe" << rect.w << "x" << rect.h << tmp->fileName();
        Mlt::Profile reframeProfile;
        reframeProfile.set_explicit(1);
        reframeProfile.set_colorspace(MLT.profile().colorspace());
        reframeProfile.set_frame_rate(MLT.profile().frame_rate_num(),
                                      MLT.profile().frame_rate_den());
        reframeProfile.set_sample_aspect(1, 1);
        reframeProfile.set_width(rect.w);
        reframeProfile.set_height(rect.h);
        reframeProfile.set_progressive(MLT.profile().progressive());
        const auto gcd = Util::greatestCommonDivisor(rect.w, rect.h);
        reframeProfile.set_display_aspect(rect.w / gcd, rect.h / gcd);
        LOG_DEBUG() << "reframe profile" << reframeProfile.width() << "x"
                    << reframeProfile.height();
        Mlt::Producer producer(reframeProfile,
                               "consumer",
                               (QStringLiteral("xml:%1").arg(tmp->fileName())).toUtf8().constData());
        Mlt::Filter affine;
        auto rectPropertyName("rect");
        if (Settings.playerGPU()) {
            affine = Mlt::Filter(reframeProfile, "movit.rect");
            affine.set("valign", "middle");
            filter.set("halign", "center");
        } else {
            affine = Mlt::Filter(reframeProfile, "affine");
            affine.set("transition.valign", "middle");
            affine.set("transition.halign", "center");
            rectPropertyName = "transition.rect";
        }
        producer.attach(affine);

        auto anim = filter.get_anim("rect");
        if (anim->key_count() > 0) {
            for (int k = 0; k < anim->key_count(); ++k) {
                const auto frameNum = anim->key_get_frame(k);
                if (frameNum >= 0) {
                    rect = filter.anim_get_rect("rect", frameNum);
                    rect.x = -rect.x;
                    rect.y = -rect.y;
                    rect.w = MLT.profile().width();
                    rect.h = MLT.profile().height();
                    affine.anim_set(rectPropertyName, rect, frameNum, 0, anim->key_get_type(k));
                    if (k + 1 == anim->key_count())
                        affine.anim_set(rectPropertyName, rect, service->get_out());
                }
            }
        } else {
            rect.x = -rect.x;
            rect.y = -rect.y;
            rect.w = MLT.profile().width();
            rect.h = MLT.profile().height();
            affine.set(rectPropertyName, rect);
        }

        Mlt::Consumer consumer(reframeProfile, "xml", "string");
        consumer.set("root", "");
        consumer.connect(producer);
        consumer.start();
        QXmlStreamReader xmlReader(consumer.get("string"));
        QDomDocument dom;
        dom.setContent(&xmlReader);
        const auto fps = addConsumerElement(service, dom, target, properties, options);
        auto *job = new EncodeJob(QDir::toNativeSeparators(target),
                                  dom.toString(2),
                                  fps.x(),
                                  fps.y(),
                                  options.priority,
                                  options.useHardwareDecoder);
        job->setUseMultiConsumer(options.useMultiConsumerForReframe);
        tmp->setParent(job);
        if (options.in > -1 || options.out > -1)
            job->setInAndOut(options.in, options.out);
        return job;
    }
    return nullptr;
}

MeltJob *ExportJobBuilder::create(Mlt::Producer *service,
                                  const QString &target,
                                  Mlt::Properties &properties,
                                  const ExportJobOptions &options,
                                  QString &error)
{
    error.clear();
    const auto fail = [&error](const QString &message,
                               Logger::LogLevel level = Logger::Warning) -> MeltJob * {
        error = message;
        if (level == Logger::Error)
            LOG_ERROR() << message;
        else
            LOG_WARNING() << message;
        return nullptr;
    };
    if (!target.isEmpty() && !target.contains("://")) {
        const QFileInfo file(target);
        if (!file.dir().exists())
            return fail(QObject::tr("Output folder does not exist: %1").arg(file.dir().path()));
        if (file.exists() && (!file.isFile() || !file.isWritable()))
            return fail(QObject::tr("Output is not a writable file: %1").arg(target));
        QTemporaryFile probe(file.dir().filePath(".shotcut-export-XXXXXX"));
        if (!probe.open())
            return fail(QObject::tr("Output folder is not writable: %1").arg(probe.errorString()));
    }
    if (JOBS.targetIsInProgress(target))
        return fail(QObject::tr("A job already exists for %1").arg(target));

    QString exportTarget = target;
    if (ExportPreset::isImageSequence(properties)) {
        const QFileInfo file(exportTarget);
        exportTarget = QStringLiteral("%1/%2-%05d.%3")
                           .arg(file.path(), file.baseName(), file.completeSuffix());
    }

    QScopedPointer<Mlt::Producer> tempProducer;
    if (options.adjustClipFilterRanges && MLT.isSeekable(service)
        && service->type() != mlt_service_chain_type) {
        const QString xml = MLT.XML(service);
        tempProducer.reset(new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData()));
        service = tempProducer.data();
        const int producerIn = tempProducer->get_in();
        if (producerIn > 0) {
            for (int i = 0; i < tempProducer->filter_count(); ++i) {
                QScopedPointer<Mlt::Filter> filter(tempProducer->filter(i));
                if (filter->get_in() > 0)
                    filter->set_in_and_out(filter->get_in() - producerIn,
                                           filter->get_out() - producerIn);
            }
        }
    }

    auto tmp = new QTemporaryFile{Util::writableTemporaryFile(target)};
    if (!tmp->open()) {
        delete tmp;
        return fail(QObject::tr("Cannot create temporary export XML."), Logger::Error);
    }
    const QString fileName = tmp->fileName();
    MLT.saveXML(fileName, service, false /* without relative paths */, tmp, options.useProxy);
    tmp->close();

    QFile input(fileName);
    if (!input.open(QIODevice::ReadOnly)) {
        delete tmp;
        return fail(QObject::tr("Cannot read temporary export XML: %1").arg(input.errorString()),
                    Logger::Error);
    }
    QXmlStreamReader xmlReader(&input);
    QDomDocument dom(fileName);
    const auto parsed = dom.setContent(&xmlReader);
    input.close();
    if (!parsed) {
        delete tmp;
        return fail(QObject::tr("Invalid export XML: %1").arg(parsed.errorMessage), Logger::Error);
    }
    if (options.validateProjectXml) {
        MltXmlChecker checker;
        if (checker.check(fileName) != QXmlStreamReader::NoError
            || checker.unlinkedFilesModel().rowCount() > 0) {
            delete tmp;
            return fail(QObject::tr("The project contains missing media or invalid XML."),
                        Logger::Error);
        }
        const QString canonicalTarget = QFileInfo(target).canonicalFilePath();
        const auto nodes = dom.elementsByTagName("property");
        for (int i = 0; i < nodes.size(); ++i) {
            const QFileInfo resource(nodes.at(i).toElement().text());
            if (!canonicalTarget.isEmpty() && resource.exists()
                && resource.canonicalFilePath() == canonicalTarget) {
                delete tmp;
                return fail(QObject::tr("Cannot export over a file used in the project."));
            }
        }
    }

    const QString xml = dom.toString(0);
    if (xml.contains(QDir::fromNativeSeparators(target))) {
        delete tmp;
        return fail(QObject::tr("You cannot write to a file that is in your project.\n"
                                "Try again with a different folder or file name."));
    }

    const QDomNodeList playlists = dom.elementsByTagName("playlist");
    for (int i = 0; i < playlists.length(); ++i)
        playlists.item(i).toElement().setAttribute("autoclose", 1);

    MeltJob *job = convertReframe(service, tmp, exportTarget, properties, options);
    if (!job) {
        const auto fps = addConsumerElement(service, dom, exportTarget, properties, options);
        job = new EncodeJob(QDir::toNativeSeparators(target),
                            dom.toString(2),
                            fps.x(),
                            fps.y(),
                            options.priority,
                            options.useHardwareDecoder);
        job->setUseMultiConsumer(options.useMultiConsumer);
        if (options.in > -1 || options.out > -1)
            job->setInAndOut(options.in, options.out);
        delete tmp;
    }
    return job;
}
