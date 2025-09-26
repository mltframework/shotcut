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

ScreenCaptureJob::ScreenCaptureJob(const QString &name, const QString &filename, bool isRecording)
    : AbstractJob(name, QThread::NormalPriority)
    , m_filename(filename)
    , m_isRecording(isRecording)
{
    QAction *action = new QAction(tr("Open"), this);
    action->setToolTip(tr("Open the capture"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(onOpenTriggered()));
    m_successActions << action;

    // Add stop action for recording jobs
    if (m_isRecording) {
        QAction *stopAction = new QAction(tr("Stop Recording"), this);
        stopAction->setToolTip(tr("Stop the screen recording"));
        connect(stopAction, &QAction::triggered, this, &ScreenCaptureJob::stop);
        m_standardActions << stopAction;
    }
}

ScreenCaptureJob::~ScreenCaptureJob() {}

void ScreenCaptureJob::start()
{
    LOG_DEBUG() << "starting screen capture job";

    connect(this, &QProcess::finished, this, &ScreenCaptureJob::onFinished);

    QStringList args;
#ifdef Q_OS_MAC
    if (m_isRecording) {
        // For screen recording, use -v for video only
        args << "-v";
    } else {
        // For screenshot, use -i for interactive selection and -t png for PNG format
        args << "-i"
             << "-t"
             << "png";
    }
    args << m_filename;
    LOG_DEBUG() << "screencapture " + args.join(' ');
    AbstractJob::start("screencapture", args);
#endif
}

void ScreenCaptureJob::stop()
{
    if (m_isRecording) {
        // For screen recording, try to terminate gracefully first
#ifdef Q_OS_MAC
        write("q");
        QTimer::singleShot(1000, this, [this]() { AbstractJob::stop(); });
#endif
    } else {
        // For screenshots, terminate immediately
        AbstractJob::stop();
    }
}

void ScreenCaptureJob::onOpenTriggered()
{
    MAIN.open(m_filename);
}

void ScreenCaptureJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    AbstractJob::onFinished(exitCode, exitStatus);

    if (exitCode == 0 && QFileInfo::exists(m_filename)) {
        // Automatically open the captured file
        QTimer::singleShot(0, this, [this]() { MAIN.open(m_filename); });
    }
}
