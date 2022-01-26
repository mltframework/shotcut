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

#include "ffprobejob.h"
#include "mainwindow.h"
#include "dialogs/textviewerdialog.h"
#include "util.h"

#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <Logger.h>

FfprobeJob::FfprobeJob(const QString& name, const QStringList& args)
    : AbstractJob(name)
{
    m_args.append(args);
}

FfprobeJob::~FfprobeJob()
{

}

void FfprobeJob::start()
{
    QString shotcutPath = qApp->applicationDirPath();
    QFileInfo ffprobePath(shotcutPath, "ffprobe");
    setReadChannel(QProcess::StandardOutput);
    LOG_DEBUG() << ffprobePath.absoluteFilePath()  + " " + m_args.join(' ');
    AbstractJob::start(ffprobePath.absoluteFilePath(), m_args);
}

void FfprobeJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    AbstractJob::onFinished(exitCode, exitStatus);
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        TextViewerDialog dialog(&MAIN);
        dialog.setWindowTitle(tr("More Information"));
        dialog.setText(log().replace("\\:", ":"));
        dialog.exec();
    }
    deleteLater();
}
