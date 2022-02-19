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
#include "mainwindow.h"
#include "dialogs/textviewerdialog.h"
#include "util.h"
#include <MltProperties.h>

#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <Logger.h>

FfmpegJob::FfmpegJob(const QString& name, const QStringList& args, bool isOpenLog, QThread::Priority priority)
    : AbstractJob(name, priority)
    , m_outputMsgRead(false)
    , m_totalFrames(0)
    , m_previousPercent(0)
    , m_isOpenLog(isOpenLog)
{
    QAction* action = new QAction(tr("Open"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onOpenTriggered()));
    m_successActions << action;
    m_args.append(args);
    setLabel(tr("Check %1").arg(Util::baseName(name)));
}

FfmpegJob::~FfmpegJob()
{
    if (objectName().contains("proxies") && objectName().contains(".pending.")){
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
    QTimer::singleShot(3000, this, [this](){ AbstractJob::stop(); });
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

void FfmpegJob::onReadyRead()
{
    QString msg;
    do {
        msg = readLine();
        if (!msg.startsWith("frame=") && (!msg.trimmed().isEmpty())) {
            appendToLog(msg);
        }
        if (msg.contains("Duration:")) {
            m_duration = msg.mid(msg.indexOf("Duration:") + 9);
            m_duration = m_duration.left(m_duration.indexOf(','));
            emit progressUpdated(m_item, 0);
        }
        else if (!m_outputMsgRead) {
            // Wait for the "Output" then read the output fps to calculate number of frames.
            if (msg.contains("Output ")) {
                m_outputMsgRead = true;
            }
        }
        if (!m_totalFrames && msg.contains(" fps")) {
            Mlt::Profile profile;
            QRegularExpression re("(\\d+|\\d+.\\d+) fps");
            QRegularExpressionMatch match = re.match(msg);
            if (match.hasMatch()) {
                QString fps = match.captured(1);
                profile.set_frame_rate(qRound(fps.toFloat() * 1000), 1000);
            } else {
                profile.set_frame_rate(25, 1);
            }
            Mlt::Properties props;
            props.set("_profile", profile.get_profile(), 0);
            m_totalFrames = props.time_to_frames(m_duration.toLatin1().constData());
        }
        else if (msg.startsWith("frame=") && m_totalFrames > 0) {
            msg = msg.mid(msg.indexOf("frame=") + 6);
            msg = msg.left(msg.indexOf(" fps"));
            int frame = msg.toInt();
            int percent = qRound(frame * 100.0 / m_totalFrames);
            if (percent != m_previousPercent) {
                emit progressUpdated(m_item, percent);
                m_previousPercent = percent;
            }
        }
    } while (!msg.isEmpty());
}
