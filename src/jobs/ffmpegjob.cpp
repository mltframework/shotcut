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

#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

FfmpegJob::FfmpegJob(const QString& name, const QStringList& args)
    : AbstractJob(name)
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
    QProcess::start(ffmpegPath.absoluteFilePath(), m_args);
    AbstractJob::start();
}

void FfmpegJob::onOpenTriggered()
{
    TextViewerDialog dialog(&MAIN);
    dialog.setWindowTitle(tr("FFmpeg Log"));
    dialog.setText(log());
    dialog.exec();
}
