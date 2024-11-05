/*
 * Copyright (c) 2022 Meltytech, LLC
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

#include "gopro2gpxjob.h"
#include "mainwindow.h"
#include "dialogs/textviewerdialog.h"
#include "util.h"

#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <Logger.h>

GoPro2GpxJob::GoPro2GpxJob(const QString &name, const QStringList &args)
    : AbstractJob(name)
{
    m_args.append(args);
    setLabel(QStringLiteral("%1 %2").arg(tr("Export GPX"), Util::baseName(name)));
}

void GoPro2GpxJob::start()
{
    QString shotcutPath = qApp->applicationDirPath();
    QFileInfo gopro2gpxPath(shotcutPath, "gopro2gpx");
    setReadChannel(QProcess::StandardOutput);
    LOG_DEBUG() << gopro2gpxPath.absoluteFilePath()  + " " + m_args.join(' ');
    AbstractJob::start(gopro2gpxPath.absoluteFilePath(), m_args);
}
