/*
 * Copyright (c) 2018 Meltytech, LLC
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

#include "postjobaction.h"
#include "mainwindow.h"
#include "docks/playlistdock.h"

// For file time functions in FilePropertiesPostJobAction::doAction();
#include <utime.h>
#include <sys/stat.h>

#include <QFile>

void FilePropertiesPostJobAction::doAction()
{
    // TODO: When QT 5.10 is available, use QFileDevice functions
#ifdef Q_OS_WIN
    struct _stat srcTime;
    struct _utimbuf dstTime;
    _stat(m_srcFile.toUtf8().constData(), &srcTime);
    dstTime.actime = srcTime.st_atime;
    dstTime.modtime = srcTime.st_mtime;
    _utime(m_dstFile.toUtf8().constData(), &dstTime);
#else
    struct stat srcTime;
    struct utimbuf dstTime;
    stat(m_srcFile.toUtf8().constData(), &srcTime);
    dstTime.actime = srcTime.st_atime;
    dstTime.modtime = srcTime.st_mtime;
    utime(m_dstFile.toUtf8().constData(), &dstTime);
#endif
}

void ReverseFilePostJobAction::doAction()
{
    FilePropertiesPostJobAction::doAction();
    QFile::remove(m_fileNameToRemove);
    MAIN.open(m_dstFile);
    MAIN.playlistDock()->on_actionAppendCut_triggered();
}
