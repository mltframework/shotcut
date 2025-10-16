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
    QString qp("-crf");
    args << "-f"
         << "x11grab";
    args << "-framerate" << QString::number(MLT.profile().fps());
    args << "-grab_x" << QString::number(m_rect.x());
    args << "-grab_y" << QString::number(m_rect.y());
    args << "-video_size" << QString("%1x%2").arg(m_rect.width()).arg(m_rect.height());
    args << "-i"
         << ":0.0";
    args << "-f"
         << "pulse";
    args << "-i"
         << "default";

    if (Settings.encodeUseHardware()) {
        auto hwCodecs = Settings.encodeHardware();
        if (hwCodecs.contains("h264_vaapi")) {
            vcodec = "h264_vaapi";
            qp = "-qp";
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
        }
    } else {
        args << "-tune"
             << "film";
        args << "-pix_fmt"
             << "yuv420p";
    }
    args << "-codec:v" << vcodec;
    args << "-profile:v"
         << "high";
    args << qp << "18";
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
