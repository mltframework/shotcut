/*
 * Copyright (c) 2025 Meltytech, LLC
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

#include "screencapturejob.h"

#include "Logger.h"
#include "mainwindow.h"
#include "postjobaction.h"
#include "settings.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>

ScreenCaptureJob::ScreenCaptureJob(const QString &name,
                                   const QString &filename,
                                   const QRect &captureRect)
    : AbstractJob(name, QThread::NormalPriority)
    , m_filename(filename)
    , m_rect(captureRect)
    , m_isAutoOpen(true)
{
    QAction *action = new QAction(tr("Open"), this);
    action->setToolTip(tr("Open the capture"));
    action->setData("Open");
    connect(action, SIGNAL(triggered(bool)), this, SLOT(onOpenTriggered()));
    m_successActions << action;
    m_standardActions.clear();
}

ScreenCaptureJob::~ScreenCaptureJob() {}

void ScreenCaptureJob::start()
{
    LOG_DEBUG() << "starting screen capture job";

    connect(this, &QProcess::finished, this, &ScreenCaptureJob::onFinished);

    QStringList args;
#ifdef Q_OS_MAC
    args << "-C";
    args << "-g";
    args << "-k";
    args << "-v";
    args << m_filename;
    LOG_DEBUG() << "screencapture " + args.join(' ');
    AbstractJob::start("screencapture", args);
#else
    QString vcodec("libx264");
    args << "-f"
#ifdef Q_OS_WIN
         << "gdigrab";
    args << "-offset_x" << QString::number(m_rect.x());
    args << "-offset_y" << QString::number(m_rect.y());
    args << "-framerate" << QString::number(MLT.profile().fps());
    args << "-video_size" << QString("%1x%2").arg(m_rect.width()).arg(m_rect.height());
    args << "-i"
         << "desktop";
    args << "-f"
         << "dshow";
    args << "-i"
         << "audio=" + Settings.audioInput();
    if (Settings.encodeUseHardware() && !Settings.encodeHardware().isEmpty()) {
        vcodec = "h264_mf";
        args << "-rate_control"
             << "quality";
        args << "-q:v"
             << "90";
        args << "-pix_fmt"
             << "nv12";
        args << "-hw_encoding"
             << "true";
#else
         << "x11grab";
    args << "-grab_x" << QString::number(m_rect.x());
    args << "-grab_y" << QString::number(m_rect.y());
    args << "-framerate" << QString::number(MLT.profile().fps());
    args << "-video_size" << QString("%1x%2").arg(m_rect.width()).arg(m_rect.height());
    args << "-i"
         << ":0.0";
    args << "-f"
         << "pulse";
    args << "-i" << Settings.audioInput();
    if (Settings.encodeUseHardware() && Settings.encodeHardware().contains("h264_vaapi")) {
        vcodec = "h264_vaapi";
        args << "-qp"
             << "18";
        args << "-init_hw_device"
             << "vaapi=vaapi0:";
        args << "-filter_hw_device"
             << "vaapi0";
        args << "-vf"
             << "format=nv12,hwupload";
        args << "-quality"
             << "1";
        args << "-rc_mode"
             << "CQP";
#endif
    } else {
        args << "-crf"
             << "18";
        args << "-tune"
             << "film";
        args << "-pix_fmt"
             << "yuv420p";
    }
    args << "-codec:v" << vcodec;
    if (vcodec == "h264_mf") {
        args << "-profile:v"
             << "100";
    } else {
        args << "-profile:v"
             << "high";
    }
    args << "-bf"
         << "2";
    args << "-g" << QString::number(qRound(2.0 * MLT.profile().fps()));
    args << "-color_range"
         << "jpeg";
    args << "-color_primaries"
         << "bt709";
    args << "-color_trc"
         << "bt709";
    args << "-colorspace"
         << "bt709";
    args << "-y" << m_filename;
    QString shotcutPath = qApp->applicationDirPath();
    QFileInfo ffmpegPath(shotcutPath, "ffmpeg");
    setReadChannel(QProcess::StandardError);
    LOG_DEBUG() << ffmpegPath.absoluteFilePath() + " " + args.join(' ');
    AbstractJob::start(ffmpegPath.absoluteFilePath(), args);
#endif
}

void ScreenCaptureJob::stop()
{
    // Try to terminate gracefully
    write("q");
    QTimer::singleShot(3000, this, [this]() { AbstractJob::stop(); });
}

void ScreenCaptureJob::onOpenTriggered()
{
    MAIN.open(m_filename);
}

void ScreenCaptureJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    AbstractJob::onFinished(exitCode, exitStatus);

    if (m_isAutoOpen && exitCode == 0 && QFileInfo::exists(m_filename)) {
        // Automatically open the captured file
        m_isAutoOpen = false;
        QTimer::singleShot(0, this, [this]() { MAIN.open(m_filename); });
    }
}
