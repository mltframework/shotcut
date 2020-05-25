/*
 * Copyright (c) 2020 Meltytech, LLC
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
#include "util.h"

#include <QObject>

QDir ProxyManager::dir()
{
    // Use project folder + "/proxies" if using project folder and enabled
    QDir dir(MLT.projectFolder());
    if (!MLT.projectFolder().isEmpty() && dir.exists() && Settings.proxyUseProjectFolder()) {
        const char* subfolder = "proxies";
        if (!dir.cd(subfolder)) {
            if (dir.mkdir(subfolder))
                dir.cd(subfolder);
        }
    } else {
        // Otherwise, use app setting
        dir = QDir(Settings.proxyFolder());
    }
    return dir;
}

QString ProxyManager::resource(Mlt::Producer& producer)
{
    QString resource = QString::fromUtf8(producer.get("resource"));
    if (producer.get_int(kIsProxyProperty) && producer.get(kOriginalResourceProperty)) {
        resource = QString::fromUtf8(producer.get(kOriginalResourceProperty));
    } else if (!::qstrcmp(producer.get("mlt_service"), "timewarp")) {
        resource = QString::fromUtf8(producer.get("warp_resource"));
    }
    return resource;
}

void ProxyManager::generateVideoProxy(Mlt::Producer& producer, bool fullRange, ScanMode scanMode, const QPoint& aspectRatio)
{
    // Always regenerate per preview scaling or 540 if not specified
    QString resource = ProxyManager::resource(producer);
    QStringList args;
    QString hash = producer.get(kShotcutHashProperty);
    QString fileName = hash + ".mp4";
    QString filters;
    auto hwCodecs = Settings.encodeHardware();
    QString hwFilters;

    args << "-loglevel" << "verbose";
    args << "-i" << resource;
    args << "-max_muxing_queue_size" << "9999";
    // transcode all streams except data, subtitles, and attachments
    if (producer.get_int("video_index") < producer.get_int("audio_index"))
        args << "-map" << "0:v?" << "-map" << "0:a?";
    else
        args << "-map" << "0:a?" << "-map" << "0:v?";
    args << "-map_metadata" << "0" << "-ignore_unknown";
    args << "-vf";

    if (scanMode == Automatic) {
        filters = QString("yadif=deint=interlaced,");
    } else if (scanMode != Progressive) {
        filters = QString("yadif=parity=%1,").arg(scanMode == InterlacedTopFieldFirst? "tff" : "bff");
    }
    filters += QString("scale=width=-2:height=%1").arg(Settings.playerPreviewScale()? Settings.playerPreviewScale() : 540);
    if (Settings.encodeUseHardware() && (hwCodecs.contains("hevc_vaapi") || hwCodecs.contains("h264_vaapi"))) {
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
    if (Settings.encodeUseHardware()) {
        if (hwCodecs.contains("hevc_nvenc")) {
            args << "-codec:v" << "hevc_nvenc";
            args << "-rc" << "constqp";
            args << "-vglobal_quality" << "30";
        } else if (hwCodecs.contains("hevc_qsv")) {
            args << "-load_plugin" << "hevc_hw";
            args << "-codec:v" << "hevc_qsv";
            args << "-qscale" << "30";
        } else if (hwCodecs.contains("hevc_amf")) {
            args << "-codec:v" << "hevc_amf";
            args << "-rc" << "cqp";
            args << "-qp_i" << "30";
        } else if (hwCodecs.contains("hevc_vaapi")) {
            args << "-init_hw_device" << "vaapi=vaapi0:,connection_type=x11" << "-filter_hw_device" << "vaapi0";
            args << "-codec:v" << "hevc_vaapi";
            args << "-qp" << "30";
        } else if (hwCodecs.contains("h264_vaapi")) {
            args << "-init_hw_device" << "vaapi=vaapi0:,connection_type=x11" << "-filter_hw_device" << "vaapi0";
            args << "-codec:v" << "h264_vaapi";
            args << "-qp" << "30";
        } else if (hwCodecs.contains("hevc_videotoolbox")) {
            args << "-codec:v" << "hevc_videotoolbox";
            args << "-qscale" << "30";
        }
    }
    if (!args.contains("-codec:v")) {
        args << "-codec:v" << "libx264";
        args << "-pix_fmt" << "yuv420p";
        args << "-preset" << "veryfast";
        args << "-crf" << "23";
    }
    args << "-g" << "1" << "-bf" << "0";
    fileName = ProxyManager::dir().filePath(fileName);
    args << "-y" << fileName;

    FfmpegJob* job = new FfmpegJob(fileName, args, false);
    job->setLabel(QObject::tr("Make proxy for %1").arg(Util::baseName(resource)));
    job->setPostJobAction(new ProxyReplacePostJobAction(resource, fileName, hash));
    JOBS.add(job);
}
