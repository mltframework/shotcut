/*
 * Copyright (c) 2016-2022 Meltytech, LLC
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

#include "ffmpegjob.h"

#include "Logger.h"
#include "dialogs/textviewerdialog.h"
#include "mainwindow.h"
#include "util.h"

#include <MltProperties.h>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <cstdio>

FfmpegJob::FfmpegJob(const QString &name,
                     const QStringList &args,
                     bool isOpenLog,
                     QThread::Priority priority)
    : AbstractJob(name, priority)
    , m_duration(0.0)
    , m_previousPercent(0)
    , m_isOpenLog(isOpenLog)
{
    QAction *action = new QAction(tr("Open"), this);
    action->setData("Open");
    connect(action, SIGNAL(triggered()), this, SLOT(onOpenTriggered()));
    m_successActions << action;
    m_args.append(args);
    setLabel(tr("Check %1").arg(Util::baseName(name)));
}

FfmpegJob::~FfmpegJob()
{
    if (objectName().contains("proxies") && objectName().contains(".pending.")) {
        QFile::remove(objectName());
    }
}

void FfmpegJob::start()
{
    QString shotcutPath = qApp->applicationDirPath();
    QFileInfo ffmpegPath(shotcutPath, "ffmpeg");
    setReadChannel(QProcess::StandardError);
    LOG_DEBUG() << ffmpegPath.absoluteFilePath() + " " + m_args.join(' ');
    AbstractJob::start(ffmpegPath.absoluteFilePath(), m_args);
}

void FfmpegJob::stop()
{
    write("q");
    QTimer::singleShot(3000, this, [this]() { AbstractJob::stop(); });
}

void FfmpegJob::onOpenTriggered()
{
    if (m_isOpenLog) {
        TextViewerDialog dialog(&MAIN);
        dialog.setWindowTitle(tr("FFmpeg Log"));
        dialog.setText(log());
        dialog.exec();
    } else {
        MAIN.open(objectName().toUtf8().constData());
    }
}

static double timeToSeconds(QString time)
{
    int h, m, s, mil;
    const int ret = std::sscanf(time.toLatin1().constData(), "%d:%d:%d.%d", &h, &m, &s, &mil);
    if (ret != 4) {
        LOG_ERROR() << "unable to parse time:" << time;
        return -1.0;
    }
    return (h * 60.0 * 60.0) + (m * 60.0) + s + (mil / 100.0);
}

void FfmpegJob::onReadyRead()
{
    QString msg;
    do {
        msg = readLine();
        if (!msg.startsWith("frame=") && (!msg.trimmed().isEmpty())) {
            appendToLog(msg);
        }
        if (m_duration == 0 && msg.contains("Duration:")) {
            msg = msg.mid(msg.indexOf("Duration:") + 9);
            msg = msg.left(msg.indexOf(','));
            m_duration = timeToSeconds(msg);
            emit progressUpdated(m_item, 0);
        } else if (m_duration != 0 && msg.startsWith("frame=")) {
            msg = msg.mid(msg.indexOf("time=") + 6);
            msg = msg.left(msg.indexOf(" bitrate"));
            double time = timeToSeconds(msg);
            int percent = qRound(time * 100.0 / m_duration);
            if (percent != m_previousPercent) {
                emit progressUpdated(m_item, percent);
                m_previousPercent = percent;
            }
        }
    } while (!msg.isEmpty());
}
