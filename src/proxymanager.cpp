/*
 * Copyright (c) 2020-2023 Meltytech, LLC
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

#include "proxymanager.h"
#include "mltcontroller.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "jobqueue.h"
#include "jobs/ffmpegjob.h"
#include "jobs/qimagejob.h"
#include "util.h"

#include <QObject>
#include <QVector>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFile>
#include <QImageReader>
#include <Logger.h>
#include <utime.h>

static const char *kProxySubfolder = "proxies";
static const char *kProxyVideoExtension = ".mp4";
static const char *kGoProProxyVideoExtension = ".LRV";
static const char *kProxyPendingVideoExtension = ".pending.mp4";
static const char *kProxyImageExtension = ".jpg";
static const char *kProxyPendingImageExtension = ".pending.jpg";
static const float kProxyResolutionRatio = 1.3f;
static const int   kFallbackProxyResolution = 540;
static const QStringList kPixFmtsWithAlpha = {"pal8", "argb", "rgba", "abgr",
                                              "bgra", "yuva420p", "yuva422p", "yuva444p", "yuva420p9be", "yuva420p9le",
                                              "yuva422p9be", "yuva422p9le", "yuva444p9be", "yuva444p9le", "yuva420p10be",
                                              "yuva420p10le", "yuva422p10be", "yuva422p10le", "yuva444p10be", "yuva444p10le",
                                              "yuva420p16be", "yuva420p16le", "yuva422p16be", "yuva422p16le", "yuva444p16be",
                                              "yuva444p16le", "rgba64be", "rgba64le", "bgra64be", "bgra64le", "ya8",
                                              "ya16le", "ya16be", "gbrap", "gbrap16le", "gbrap16be", "ayuv64le", "ayuv64be",
                                              "gbrap12le", "gbrap12be", "gbrap10le", "gbrap10be", "gbrapf32be",
                                              "gbrapf32le", "yuva422p12be", "yuva422p12le", "yuva444p12be", "yuva444p12le"
                                             };

QDir ProxyManager::dir()
{
    // Use project folder + "/proxies" if using project folder and enabled
    QDir dir(MLT.projectFolder());
    if (!MLT.projectFolder().isEmpty() && dir.exists() && Settings.proxyUseProjectFolder()) {
        if (!dir.cd(kProxySubfolder)) {
            if (dir.mkdir(kProxySubfolder))
                dir.cd(kProxySubfolder);
        }
    } else {
        // Otherwise, use app setting
        dir = QDir(Settings.proxyFolder());
    }
    return dir;
}

QString ProxyManager::resource(Mlt::Service &producer)
{
    QString resource = QString::fromUtf8(producer.get("resource"));
    if (producer.get_int(kIsProxyProperty) && producer.get(kOriginalResourceProperty)) {
        resource = QString::fromUtf8(producer.get(kOriginalResourceProperty));
    } else if (!::qstrcmp(producer.get("mlt_service"), "timewarp")) {
        resource = QString::fromUtf8(producer.get("warp_resource"));
    }
    return resource;
}

void ProxyManager::generateVideoProxy(Mlt::Producer &producer, bool fullRange, ScanMode scanMode,
                                      const QPoint &aspectRatio, bool replace)
{
    // Always regenerate per preview scaling or 540 if not specified
    QString resource = ProxyManager::resource(producer);
    QStringList args;
    QString hash = Util::getHash(producer);
    QString fileName = ProxyManager::dir().filePath(hash + kProxyPendingVideoExtension);
    QString filters;
    auto hwCodecs = Settings.encodeHardware();
    QString hwFilters;

    // Touch file to make it in progress
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    file.resize(0);
    file.close();

    args << "-loglevel" << "verbose";
    args << "-i" << resource;
    args << "-max_muxing_queue_size" << "9999";
    // transcode all streams except data, subtitles, and attachments
    auto audioIndex = producer.property_exists(kDefaultAudioIndexProperty) ? producer.get_int(
                          kDefaultAudioIndexProperty) : producer.get_int("audio_index");
    if (producer.get_int("video_index") < audioIndex) {
        args << "-map" << "0:V?" << "-map" << "0:a?";
    } else {
        args << "-map" << "0:a?" << "-map" << "0:V?";
    }
    args << "-map_metadata" << "0" << "-ignore_unknown";
    args << "-vf";

    if (scanMode == Automatic) {
        filters = QString("yadif=deint=interlaced,");
    } else if (scanMode != Progressive) {
        filters = QString("yadif=parity=%1,").arg(scanMode == InterlacedTopFieldFirst ? "tff" : "bff");
    }
    filters += QString("scale=width=-2:height=%1").arg(resolution());
    if (Settings.proxyUseHardware() && (hwCodecs.contains("hevc_vaapi")
                                        || hwCodecs.contains("h264_vaapi"))) {
        hwFilters = ",format=nv12,hwupload";
    }
    if (fullRange) {
        args << filters + ":in_range=full:out_range=full" + hwFilters;
        args << "-color_range" << "jpeg";
    } else {
        args << filters + ":in_range=mpeg:out_range=mpeg" + hwFilters;
        args << "-color_range" << "mpeg";
    }
    switch (producer.get_int("meta.media.colorspace")) {
    case 601:
        if (producer.get_int("meta.media.height") == 576) {
            args << "-color_primaries" << "bt470bg";
            args << "-color_trc" << "smpte170m";
            args << "-colorspace" << "bt470bg";
        } else {
            args << "-color_primaries" << "smpte170m";
            args << "-color_trc" << "smpte170m";
            args << "-colorspace" << "smpte170m";
        }
        break;
    case 170:
        args << "-color_primaries" << "smpte170m";
        args << "-color_trc" << "smpte170m";
        args << "-colorspace" << "smpte170m";
        break;
    case 240:
        args << "-color_primaries" << "smpte240m";
        args << "-color_trc" << "smpte240m";
        args << "-colorspace" << "smpte240m";
        break;
    case 470:
        args << "-color_primaries" << "bt470bg";
        args << "-color_trc" << "bt470bg";
        args << "-colorspace" << "bt470bg";
        break;
    default:
        args << "-color_primaries" << "bt709";
        args << "-color_trc" << "bt709";
        args << "-colorspace" << "bt709";
        break;
    }
    if (!aspectRatio.isNull()) {
        args << "-aspect" << QString("%1:%2").arg(aspectRatio.x()).arg(aspectRatio.y());
    }
    args << "-f" << "mp4" << "-codec:a" << "ac3" << "-b:a" << "256k";
    args << "-pix_fmt" << "yuv420p";
    if (Settings.proxyUseHardware()) {
        if (hwCodecs.contains("hevc_nvenc")) {
            args << "-codec:v" << "hevc_nvenc";
            args << "-rc" << "constqp";
            args << "-qp:v" << "37";
        } else if (hwCodecs.contains("hevc_qsv")) {
            args << "-load_plugin" << "hevc_hw";
            args << "-codec:v" << "hevc_qsv";
            args << "-q:v" << "36";
        } else if (hwCodecs.contains("hevc_amf")) {
            args << "-codec:v" << "hevc_amf";
            args << "-rc" << "1";
            args << "-qp_i" << "32" << "-qp_p" << "32";
        } else if (hwCodecs.contains("hevc_vaapi")) {
            args << "-init_hw_device" << "vaapi=vaapi0:" << "-filter_hw_device" << "vaapi0";
            args << "-codec:v" << "hevc_vaapi";
            args << "-qp" << "37";
        } else if (hwCodecs.contains("h264_nvenc")) {
            args << "-codec:v" << "h264_nvenc";
            args << "-rc" << "constqp";
            args << "-qp:v" << "37";
        } else if (hwCodecs.contains("h264_vaapi")) {
            args << "-init_hw_device" << "vaapi=vaapi0:" << "-filter_hw_device" << "vaapi0";
            args << "-codec:v" << "h264_vaapi";
            args << "-qp" << "30";
        } else if (hwCodecs.contains("hevc_videotoolbox")) {
            args << "-codec:v" << "hevc_videotoolbox";
            args << "-b:v" << "2M";
        } else if (hwCodecs.contains("h264_videotoolbox")) {
            args << "-codec:v" << "h264_videotoolbox";
            args << "-b:v" << "2M";
        } else if (hwCodecs.contains("h264_qsv")) {
            args << "-codec:v" << "h264_qsv";
            args << "-q:v" << "36";
        } else if (hwCodecs.contains("h264_amf")) {
            args << "-codec:v" << "h264_amf";
            args << "-rc" << "1";
            args << "-qp_i" << "32" << "-qp_p" << "32";
        }
    }
    if (!args.contains("-codec:v")) {
        args << "-codec:v" << "libx264";
        args << "-preset" << "veryfast";
        args << "-crf" << "23";
    }
    args << "-g" << "1" << "-bf" << "0";
    args << "-y" << fileName;

    FfmpegJob *job = new FfmpegJob(fileName, args, true);
    job->setLabel(QObject::tr("Make proxy for %1").arg(Util::baseName(resource)));
    if (replace) {
        job->setPostJobAction(new ProxyReplacePostJobAction(resource, fileName, hash));
    } else {
        job->setPostJobAction(new ProxyFinalizePostJobAction(resource, fileName));
    }
    JOBS.add(job);
}

void ProxyManager::generateImageProxy(Mlt::Producer &producer, bool replace)
{
    // Always regenerate per preview scaling or 540 if not specified
    QString resource = ProxyManager::resource(producer);
    QString hash = Util::getHash(producer);
    QString fileName = ProxyManager::dir().filePath(hash + kProxyPendingImageExtension);

    // Touch file to make it in progress
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    file.resize(0);
    file.close();

    AbstractJob *job = new QImageJob(fileName, resource, resolution());
    if (replace) {
        job->setPostJobAction(new ProxyReplacePostJobAction(resource, fileName, hash));
    } else {
        job->setPostJobAction(new ProxyFinalizePostJobAction(resource, fileName));
    }
    JOBS.add(job);
}

typedef QPair<QString, QString> MltProperty;

static void processProperties(QXmlStreamWriter &newXml, QVector<MltProperty> &properties,
                              const QString &root)
{
    // Determine if this is a proxy resource
    bool isProxy = false;
    QString newResource;
    QString service;
    QString speed = "1";
    for (const auto &p : properties) {
        if (p.first == kIsProxyProperty) {
            isProxy = true;
        } else if (p.first == kOriginalResourceProperty) {
            newResource = p.second;
        } else if (newResource.isEmpty() && p.first == "resource") {
            newResource = p.second;
        } else if (p.first == "mlt_service") {
            service = p.second;
        } else if (p.first == "warp_speed") {
            speed = p.second;
        }
    }
    QVector<MltProperty> newProperties;
    QVector<MltProperty> &propertiesRef = properties;
    if (isProxy) {
        // Filter the properties
        for (const auto &p : properties) {
            // Replace the resource property if proxy
            if (p.first == "resource") {
                // Convert to relative
                if (!root.isEmpty() && newResource.startsWith(root)) {
                    newResource = newResource.mid(root.size());
                }
                if (service == "timewarp") {
                    newProperties << MltProperty(p.first, QString("%1:%2").arg(speed, newResource));
                } else {
                    newProperties << MltProperty(p.first, newResource);
                }
            } else if (p.first == "warp_resource") {
                newProperties << MltProperty(p.first, newResource);
            } else if (p.first != kIsProxyProperty && p.first != kOriginalResourceProperty) {
                // Remove special proxy and original resource properties
                newProperties << MltProperty(p.first, p.second);
            }
        }
        propertiesRef = newProperties;
    }
    // Write all of the property elements
    for (const auto &p : propertiesRef) {
        newXml.writeStartElement("property");
        newXml.writeAttribute("name", p.first);
        newXml.writeCharacters(p.second);
        newXml.writeEndElement();
    }
    // Reset the saved properties
    properties.clear();
}

bool ProxyManager::filterXML(QString &xmlString, QString root)
{
    QString output;
    QXmlStreamReader xml(xmlString);
    QXmlStreamWriter newXml(&output);
    bool isPropertyElement = false;
    QVector<MltProperty> properties;

    // This prevents processProperties() from mis-matching a resource path that begins with root
    // when it is converting to relative paths.
    if (!root.isEmpty() && root.endsWith('/')) {
        root.append('/');
    }

    newXml.setAutoFormatting(true);
    newXml.setAutoFormattingIndent(2);

    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::Characters:
            if (!isPropertyElement)
                newXml.writeCharacters(xml.text().toString());
            break;
        case QXmlStreamReader::Comment:
            newXml.writeComment(xml.text().toString());
            break;
        case QXmlStreamReader::DTD:
            newXml.writeDTD(xml.text().toString());
            break;
        case QXmlStreamReader::EntityReference:
            newXml.writeEntityReference(xml.name().toString());
            break;
        case QXmlStreamReader::ProcessingInstruction:
            newXml.writeProcessingInstruction(xml.processingInstructionTarget().toString(),
                                              xml.processingInstructionData().toString());
            break;
        case QXmlStreamReader::StartDocument:
            newXml.writeStartDocument(xml.documentVersion().toString(), xml.isStandaloneDocument());
            break;
        case QXmlStreamReader::EndDocument:
            newXml.writeEndDocument();
            break;
        case QXmlStreamReader::StartElement: {
            const QString element = xml.name().toString();
            if (element == "property") {
                // Save each property element but do not output yet
                const QString name = xml.attributes().value("name").toString();
                properties << MltProperty(name, xml.readElementText());
                isPropertyElement = true;
            } else {
                // At the start of a non-property element
                isPropertyElement = false;
                processProperties(newXml, properties, root);
                // Write the new start element
                newXml.writeStartElement(xml.namespaceUri().toString(), element);
                for (const auto &a : xml.attributes()) {
                    newXml.writeAttribute(a);
                }
            }
            break;
        }
        case QXmlStreamReader::EndElement:
            // At the end of a non-property element
            if (xml.name().toString() != "property") {
                processProperties(newXml, properties, root);
                newXml.writeEndElement();
            }
            break;
        default:
            break;
        }
    }

    // Useful for debugging
//    LOG_DEBUG() << output;

    if (!xml.hasError()) {
        xmlString = output;
        return true;
    }
    return false;
}

bool ProxyManager::fileExists(Mlt::Producer &producer)
{
    QDir proxyDir(Settings.proxyFolder());
    QDir projectDir(MLT.projectFolder());
    QString service = QString::fromLatin1(producer.get("mlt_service"));
    QString fileName;
    if (service.startsWith("avformat")) {
        if (QFile::exists(GoProProxyFilePath(producer.get("resource")))) {
            return true;
        }
        fileName = Util::getHash(producer) + kProxyVideoExtension;
    } else if (isValidImage(producer)) {
        fileName = Util::getHash(producer) + kProxyImageExtension;
    } else {
        return false;
    }
    return (projectDir.cd(kProxySubfolder) && projectDir.exists(fileName)) || proxyDir.exists(fileName);
}

bool ProxyManager::filePending(Mlt::Producer &producer)
{
    QDir proxyDir(Settings.proxyFolder());
    QDir projectDir(MLT.projectFolder());
    QString service = QString::fromLatin1(producer.get("mlt_service"));
    QString fileName;
    if (service.startsWith("avformat")) {
        fileName = Util::getHash(producer) + kProxyPendingVideoExtension;
    } else if (isValidImage(producer)) {
        fileName = Util::getHash(producer) + kProxyPendingImageExtension;
    } else {
        return false;
    }
    return (projectDir.cd(kProxySubfolder) && projectDir.exists(fileName)) || proxyDir.exists(fileName);
}

bool ProxyManager::isValidImage(Mlt::Producer &producer)
{
    QString service = QString::fromLatin1(producer.get("mlt_service"));
    if ((service == "qimage" || service == "pixbuf") && !producer.get_int(kShotcutSequenceProperty)) {
        QImageReader reader;
        reader.setDecideFormatFromContent(true);
        reader.setFileName(ProxyManager::resource(producer));
        return reader.imageCount() == 1 && !reader.read().hasAlphaChannel();
    }
    return false;
}

bool ProxyManager::isValidVideo(Mlt::Producer producer)
{
    QString service = QString::fromLatin1(producer.get("mlt_service"));
    int video_index = producer.get_int("video_index");
    // video_index -1 means no video
    if (video_index < 0)
        return false;
    if (service == "avformat-novalidate") {
        producer = Mlt::Producer(MLT.profile(), resource(producer).toUtf8().constData());
        service = QString::fromLatin1(producer.get("mlt_service"));
        producer.set("video_index", video_index);
    }
    if (service == "avformat") {
        QString key = QString("meta.media.%1.codec.pix_fmt").arg(video_index);
        QString pix_fmt = QString::fromLatin1(producer.get(key.toLatin1().constData()));
        // Cover art is usually 90000 fps and should not be proxied
        key = QString("meta.media.%1.codec.frame_rate").arg(video_index);
        QString frame_rate = producer.get(key.toLatin1().constData());
        key = QString("meta.media.%1.codec.name").arg(video_index);
        QString codec_name = producer.get(key.toLatin1().constData());
        bool coverArt = codec_name == "mjpeg" && frame_rate == "90000";
        key = QString("meta.attr.%1.stream.alpha_mode.markup").arg(video_index);
        bool alpha_mode = producer.get_int(key.toLatin1().constData());
        LOG_DEBUG() << "pix_fmt =" << pix_fmt << " codec.frame_rate =" << frame_rate << " alpha_mode =" <<
                    alpha_mode;
        return !kPixFmtsWithAlpha.contains(pix_fmt) && !alpha_mode && !coverArt;
    }
    return false;
}

// Returns true if the producer exists and was updated with proxy info
bool ProxyManager::generateIfNotExists(Mlt::Producer &producer, bool replace)
{
    if (Settings.proxyEnabled() && producer.is_valid() && !producer.get_int(kDisableProxyProperty)
            && !producer.get_int(kIsProxyProperty)) {
        if (ProxyManager::fileExists(producer)) {
            QString service = QString::fromLatin1(producer.get("mlt_service"));
            QDir projectDir(MLT.projectFolder());
            QString fileName;
            if (service.startsWith("avformat")) {
                auto gopro = GoProProxyFilePath(producer.get("resource"));
                if (QFile::exists(gopro)) {
                    producer.set(kIsProxyProperty, 1);
                    producer.set(kMetaProxyProperty, 1);
                    producer.set(kOriginalResourceProperty, producer.get("resource"));
                    producer.set("resource", gopro.toUtf8().constData());
                    return true;
                } else {
                    fileName = Util::getHash(producer) + kProxyVideoExtension;
                }
            } else if (isValidImage(producer)) {
                fileName = Util::getHash(producer) + kProxyImageExtension;
            } else {
                return false;
            }
            producer.set(kIsProxyProperty, 1);
            producer.set(kMetaProxyProperty, 1);
            producer.set(kOriginalResourceProperty, producer.get("resource"));
            if (projectDir.exists(fileName)) {
                ::utime(projectDir.filePath(fileName).toUtf8().constData(), nullptr);
                producer.set("resource", projectDir.filePath(fileName).toUtf8().constData());
            } else {
                QDir proxyDir(Settings.proxyFolder());
                ::utime(proxyDir.filePath(fileName).toUtf8().constData(), nullptr);
                producer.set("resource", proxyDir.filePath(fileName).toUtf8().constData());
            }
            return true;
        } else if (!filePending(producer)) {
            if (isValidVideo(producer)) {
                // Tag this producer so we do not try to generate proxy again in this session
                delete producer.get_frame();
                auto threshold = qRound(kProxyResolutionRatio * resolution());
                LOG_DEBUG() << producer.get_int("meta.media.width") << "x" << producer.get_int("meta.media.height")
                            << "threshold" << threshold;
                if (producer.get_int("meta.media.width") > threshold
                        && producer.get_int("meta.media.height") > threshold) {
                    ProxyManager::generateVideoProxy(producer, MLT.fullRange(producer), Automatic, QPoint(), replace);
                }
            } else if (isValidImage(producer)) {
                // Tag this producer so we do not try to generate proxy again in this session
                delete producer.get_frame();
                auto threshold = qRound(kProxyResolutionRatio * resolution());
                LOG_DEBUG() << producer.get_int("meta.media.width") << "x" << producer.get_int("meta.media.height")
                            << "threshold" << threshold;
                if (producer.get_int("meta.media.width") > threshold
                        && producer.get_int("meta.media.height") > threshold) {
                    ProxyManager::generateImageProxy(producer, replace);
                }
            }
        }
    }
    return false;
}

const char *ProxyManager::videoFilenameExtension()
{
    return kProxyVideoExtension;
}

const char *ProxyManager::pendingVideoExtension()
{
    return kProxyPendingVideoExtension;
}

const char *ProxyManager::imageFilenameExtension()
{
    return kProxyImageExtension;
}

const char *ProxyManager::pendingImageExtension()
{
    return kProxyImageExtension;
}

int ProxyManager::resolution()
{
    return Settings.playerPreviewScale() ? Settings.playerPreviewScale() : kFallbackProxyResolution;
}

class FindNonProxyProducersParser : public Mlt::Parser
{
private:
    QString m_hash;
    QList<Mlt::Producer> m_producers;

public:
    FindNonProxyProducersParser() : Mlt::Parser() {}

    QList<Mlt::Producer> &producers()
    {
        return m_producers;
    }

    int on_start_filter(Mlt::Filter *)
    {
        return 0;
    }
    int on_start_producer(Mlt::Producer *producer)
    {
        if (!producer->parent().get_int(kIsProxyProperty))
            m_producers << Mlt::Producer(producer);
        return 0;
    }
    int on_end_producer(Mlt::Producer *)
    {
        return 0;
    }
    int on_start_playlist(Mlt::Playlist *)
    {
        return 0;
    }
    int on_end_playlist(Mlt::Playlist *)
    {
        return 0;
    }
    int on_start_tractor(Mlt::Tractor *)
    {
        return 0;
    }
    int on_end_tractor(Mlt::Tractor *)
    {
        return 0;
    }
    int on_start_multitrack(Mlt::Multitrack *)
    {
        return 0;
    }
    int on_end_multitrack(Mlt::Multitrack *)
    {
        return 0;
    }
    int on_start_track()
    {
        return 0;
    }
    int on_end_track()
    {
        return 0;
    }
    int on_end_filter(Mlt::Filter *)
    {
        return 0;
    }
    int on_start_transition(Mlt::Transition *)
    {
        return 0;
    }
    int on_end_transition(Mlt::Transition *)
    {
        return 0;
    }
    int on_start_chain(Mlt::Chain *chain)
    {
        if (!chain->parent().get_int(kIsProxyProperty))
            m_producers << Mlt::Producer(chain);
        return 0;
    }
    int on_end_chain(Mlt::Chain *)
    {
        return 0;
    }
    int on_start_link(Mlt::Link *)
    {
        return 0;
    }
    int on_end_link(Mlt::Link *)
    {
        return 0;
    }
};

void ProxyManager::generateIfNotExistsAll(Mlt::Producer &producer)
{
    FindNonProxyProducersParser parser;
    parser.start(producer);
    for (auto &clip : parser.producers()) {
        generateIfNotExists(clip, false /* replace */);
    }
}

bool ProxyManager::removePending()
{
    bool foundAny = false;
    QDir dir(MLT.projectFolder());
    if (!MLT.projectFolder().isEmpty() && dir.exists()) {
        dir.cd(kProxySubfolder);
    } else {
        dir = QDir(Settings.proxyFolder());
    }
    if (dir.exists()) {
        dir.setNameFilters(QStringList() << "*.pending.*");
        dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Writable);
        for (const auto &s : dir.entryList()) {
            LOG_INFO() << "removing" << dir.filePath(s);
            foundAny |= QFile::remove(dir.filePath(s));
        }
    }
    //TODO if any pending remove, let user know and offer to regenerate?
    return foundAny;
}

QString ProxyManager::GoProProxyFilePath(const QString &resource)
{
    auto fi = QFileInfo(resource);
    auto base = fi.baseName();
    base = "GL" + base.mid(2);
    auto result = fi.absoluteDir().filePath(base + kGoProProxyVideoExtension);
    LOG_DEBUG() << result;
    return result;
}
