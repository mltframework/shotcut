/*
 * Copyright (c) 2023-2024 Meltytech, LLC
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

#include "transcoder.h"

#include "jobqueue.h"
#include "jobs/ffmpegjob.h"
#include "mainwindow.h"
#include "util.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"

#include <QFileDialog>
#include <QMessageBox>

static const auto kHandleSeconds = 15.0;

void Transcoder::setProducers(QList<Mlt::Producer> &producers)
{
    m_producers = producers;
}

void Transcoder::addProducer(Mlt::Producer &producer)
{
    m_producers.append(producer);
}

void Transcoder::addProducer(Mlt::Producer *producer)
{
    m_producers.append(*producer);
}

void Transcoder::convert(TranscodeDialog &dialog)
{
    int result = dialog.exec();
    if (dialog.isCheckBoxChecked()) {
        Settings.setShowConvertClipDialog(false);
    }
    if (result != QDialog::Accepted) {
        return;
    }

    QString path = Settings.savePath();
    QString suffix = dialog.isSubClip() ? tr("Sub-clip") + ' ' : tr("Converted");
    QString filename;
    QString nameFormat;
    QString nameFilter;

    switch (dialog.format()) {
    case 0:
        nameFormat = "/%1 - %2.mp4";
        nameFilter = tr("MP4 (*.mp4);;All Files (*)");
        break;
    case 1:
        nameFormat = "/%1 - %2.mov";
        nameFilter = tr("MOV (*.mov);;All Files (*)");
        break;
    case 2:
        nameFormat = "/%1 - %2.mkv";
        nameFilter = tr("MKV (*.mkv);;All Files (*)");
        break;
    }

    if (m_producers.length() == 1) {
        QString resource = Util::GetFilenameFromProducer(&m_producers[0]);
        QFileInfo fi(resource);
        filename = path + nameFormat.arg(fi.completeBaseName(), suffix);
        if (dialog.isSubClip()) {
            filename = Util::getNextFile(path);
        }
        filename = QFileDialog::getSaveFileName(MAIN.centralWidget(), dialog.windowTitle(), filename,
                                                nameFilter,
                                                nullptr, Util::getFileDialogOptions());
        if (!filename.isEmpty()) {
            if (filename == QDir::toNativeSeparators(resource)) {
                QMessageBox::warning(MAIN.centralWidget(), dialog.windowTitle(),
                                     QObject::tr("Unable to write file %1\n"
                                                 "Perhaps you do not have permission.\n"
                                                 "Try again with a different folder.")
                                     .arg(fi.fileName()));
                return;
            }
            if (Util::warnIfNotWritable(filename, MAIN.centralWidget(), dialog.windowTitle()))
                return;

            if (Util::warnIfLowDiskSpace(filename)) {
                MAIN.showStatusMessage(tr("Convert canceled"));
                return;
            }
        }
        convertProducer(&m_producers[0], dialog, filename);
    } else if (m_producers.length() > 1) {
        path = QFileDialog::getExistingDirectory(MAIN.centralWidget(), dialog.windowTitle(), path,
                                                 Util::getFileDialogOptions());
        if (path.isEmpty()) {
            MAIN.showStatusMessage(tr("Convert canceled"));
            return;
        }
        if (Util::warnIfNotWritable(path, MAIN.centralWidget(), dialog.windowTitle())) {
            return;
        }
        if (Util::warnIfLowDiskSpace(path)) {
            MAIN.showStatusMessage(tr("Convert canceled"));
            return;
        }

        for (auto &producer : m_producers) {
            QString resource = Util::GetFilenameFromProducer(&producer);
            QFileInfo fi(resource);
            filename = path + nameFormat.arg(fi.completeBaseName(), suffix);
            filename = Util::getNextFile(filename);
            convertProducer(&producer, dialog, filename);
        }
    }
    Settings.setSavePath(QFileInfo(filename).path());
}

void Transcoder::convertProducer(Mlt::Producer *producer, TranscodeDialog &dialog, QString filename)
{
    QString resource = Util::GetFilenameFromProducer(producer);
    QStringList args;
    int in = -1;

    args << "-loglevel" << "verbose";
    args << "-i" << resource;
    args << "-max_muxing_queue_size" << "9999";

    if (dialog.isSubClip()) {
        if (Settings.proxyEnabled()) {
            producer->Mlt::Properties::clear(kOriginalResourceProperty);
        }

        // set trim options
        if (producer->get(kFilterInProperty)) {
            in = producer->get_int(kFilterInProperty);
            int ss = qMax(0, in - qRound(producer->get_fps() * kHandleSeconds));
            auto s = QString::fromLatin1(producer->frames_to_time(ss, mlt_time_clock));
            args << "-ss" << s.replace(',', '.');
            in -= ss;
        } else {
            args << "-ss" << QString::fromLatin1(producer->get_time("in", mlt_time_clock)).replace(',',
                                                                                                   '.').replace(',', '.');
        }
        if (producer->get(kFilterOutProperty)) {
            int out = producer->get_int(kFilterOutProperty);
            int to = qMin(producer->get_playtime() - 1, out + qRound(producer->get_fps() * kHandleSeconds));
            auto s = QString::fromLatin1(producer->frames_to_time(to, mlt_time_clock));
            args << "-to" << s.replace(',', '.');
        } else {
            args << "-to" << QString::fromLatin1(producer->get_time("out", mlt_time_clock)).replace(',', '.');
        }
    }

    // transcode all streams except data, subtitles, and attachments
    auto audioIndex = producer->property_exists(kDefaultAudioIndexProperty) ? producer->get_int(
                          kDefaultAudioIndexProperty) : producer->get_int("audio_index");
    if (producer->get_int("video_index") < audioIndex) {
        args << "-map" << "0:V?" << "-map" << "0:a?";
    } else {
        args << "-map" << "0:a?" << "-map" << "0:V?";
    }
    args << "-map_metadata" << "0" << "-ignore_unknown";

    // Set Sample rate if different than source
    if ( !dialog.sampleRate().isEmpty() ) {
        args << "-ar" << dialog.sampleRate();
    }

    // Set video filters
    args << "-vf";
    QString filterString;
    if (dialog.deinterlace()) {
        QString deinterlaceFilter = QString("bwdif,");
        filterString = filterString + deinterlaceFilter;
    }

    QString color_range;
    if (producer->get("color_range")) {
        if (producer->get_int("color_range") == 2) {
            color_range = "full";
        } else {
            color_range = "mpeg";
        }
    } else if (producer->get("force_full_range")) {
        if (producer->get_int("force_full_range")) {
            color_range = "full";
        } else {
            color_range = "mpeg";
        }
    } else {
        color_range = producer->get("meta.media.color_range");
    }
    if (color_range != "full" && color_range != "mpeg") {
        color_range = "mpeg";
    }

    if (dialog.get709Convert()) {
        QString convertFilter =
            QString("zscale=t=linear:npl=100,format=gbrpf32le,zscale=p=bt709,tonemap=tonemap=hable:desat=0,zscale=t=bt709:m=bt709:r=tv,format=yuv422p,");
        filterString = filterString + convertFilter;
    }
    filterString = filterString +
                   QString("scale=flags=accurate_rnd+full_chroma_inp+full_chroma_int:in_range=%1:out_range=%2").arg(
                       color_range).arg(color_range);
    if (dialog.fpsOverride()) {
        auto fps = QString("%1").arg(dialog.fps(), 0, 'f', 6);
        int numerator, denominator;
        Util::normalizeFrameRate(dialog.fps(), numerator, denominator);
        if (denominator == 1001) {
            fps = QString("%1/%2").arg(numerator).arg(denominator);
        }
        QString minterpFilter =
            QString(",minterpolate='mi_mode=%1:mc_mode=aobmc:me_mode=bidir:vsbmc=1:fps=%2'").arg(
                dialog.frc()).arg(fps);
        filterString = filterString + minterpFilter;
    }
    args << filterString;

    // Specify color range
    if (color_range == "full") {
        args << "-color_range" << "2";
    } else {
        args << "-color_range" << "1";
    }

    int progressive = producer->get_int("meta.media.progressive")
                      || producer->get_int("force_progressive");
    if (!dialog.deinterlace() && !progressive) {
        int tff = producer->get_int("meta.media.top_field_first") || producer->get_int("force_tff");
        args << "-flags" << "+ildct+ilme" << "-top" << QString::number(tff);
    }

    switch (dialog.format()) {
    case 0:
        args << "-f" << "mp4" << "-codec:a" << "ac3" << "-b:a" << "512k" << "-codec:v" << "libx264";
        args << "-preset" << "medium" << "-g" << "1" << "-crf" << "15";
        break;
    case 1:
        args << "-f" << "mov" << "-codec:a" << "pcm_f32le";
        if (dialog.deinterlace() || progressive) {
            args << "-codec:v" << "dnxhd" << "-profile:v" << "dnxhr_hq" << "-pix_fmt" << "yuv422p";
        } else { // interlaced
            args << "-codec:v" << "prores_ks" << "-profile:v" << "standard";
        }
        break;
    case 2:
        args << "-f" << "matroska" << "-codec:a" << "pcm_f32le" << "-codec:v" << "utvideo";
        args << "-pix_fmt" << "yuv422p";
        break;
    }
    if (dialog.get709Convert()) {
        args << "-colorspace" << "bt709" << "-color_primaries" << "bt709" << "-color_trc" << "bt709";
    } else if (dialog.format() == 2 && producer->get_int("meta.media.colorspace") == 709) {
        // Work around a limitation that FFMpeg does not pass colorspace for utvideo
        args << "-colorspace" << "bt709";
    }

    args << "-y" << filename;
    producer->Mlt::Properties::clear(kOriginalResourceProperty);

    FfmpegJob *job = new FfmpegJob(filename, args, false);
    job->setLabel(tr("Convert %1").arg(Util::baseName(filename)));
    if (dialog.isSubClip()) {
        if (producer->get(kMultitrackItemProperty)) {
            QString s = QString::fromLatin1(producer->get(kMultitrackItemProperty));
            auto parts = s.split(':');
            if (parts.length() == 2) {
                int clipIndex = parts[0].toInt();
                int trackIndex = parts[1].toInt();
                QUuid uuid = MAIN.timelineClipUuid(trackIndex, clipIndex);
                if (!uuid.isNull()) {
                    job->setPostJobAction(new ReplaceOnePostJobAction(resource, filename, QString(), uuid,
                                                                      in));
                    JOBS.add(job);
                }
            }
        } else {
            job->setPostJobAction(new OpenPostJobAction(resource, filename, QString()));
            JOBS.add(job);
        }
        return;
    }
    job->setPostJobAction(new ReplaceAllPostJobAction(resource, filename, Util::getHash(*producer)));
    JOBS.add(job);
}
