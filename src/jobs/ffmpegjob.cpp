/*
 * Copyright (c) 2016 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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
#include <QDebug>

FfmpegJob::FfmpegJob(const QString& name, const QStringList& args)
    : AbstractJob(name)
    , m_totalFrames(0)
{
    QAction* action = new QAction(tr("Open"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onOpenTriggered()));
    m_successActions << action;
    m_args.append(args);
    setLabel(tr("Check %1").arg(Util::baseName(name)));
}

FfmpegJob::~FfmpegJob()
{

}

void FfmpegJob::start()
{
    QString shotcutPath = qApp->applicationDirPath();
    QFileInfo ffmpegPath(shotcutPath, "ffmpeg");
    setReadChannel(QProcess::StandardError);
    qDebug() << ffmpegPath.absoluteFilePath() << m_args;
#ifdef Q_OS_WIN
    QProcess::start(ffmpegPath.absoluteFilePath(), m_args);
#else
    m_args.prepend(ffmpegPath.absoluteFilePath());
    QProcess::start("/usr/bin/nice", m_args);
#endif
    AbstractJob::start();
}

void FfmpegJob::onOpenTriggered()
{
    TextViewerDialog dialog(&MAIN);
    dialog.setWindowTitle(tr("FFmpeg Log"));
    dialog.setText(log());
    dialog.exec();
}

void FfmpegJob::onReadyRead()
{
    QString msg = readLine();
    if (msg.contains("Duration:")) {
        m_duration = msg.mid(msg.indexOf("Duration:") + 9);
        m_duration = m_duration.left(m_duration.indexOf(','));
        emit progressUpdated(m_index, 0);
        appendToLog(msg);
    }
    else if (!m_totalFrames && msg.contains(" fps")) {
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
        appendToLog(msg);
    }
    else if (msg.startsWith("frame=") && m_totalFrames > 0) {
        msg = msg.mid(msg.indexOf("frame=") + 6);
        msg = msg.left(msg.indexOf(" fps"));
        int frame = msg.toInt();
        emit progressUpdated(m_index, qRound(frame * 100.0 / m_totalFrames));
    }
    else {
        if (!msg.trimmed().isEmpty())
            appendToLog(msg);
    }
}
