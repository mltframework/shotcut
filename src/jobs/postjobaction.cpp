/*
 * Copyright (c) 2018-2020 Meltytech, LLC
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
#include "shotcut_mlt_properties.h"
#include <Logger.h>

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

void ReverseOpenPostJobAction::doAction()
{
    FilePropertiesPostJobAction::doAction();
    QFile::remove(m_fileNameToRemove);
    MAIN.open(m_dstFile);
    MAIN.playlistDock()->on_actionAppendCut_triggered();
}

void ReverseReplacePostJobAction::doAction()
{
    FilePropertiesPostJobAction::doAction();
    QFile::remove(m_fileNameToRemove);
    Mlt::Producer producer(MLT.profile(), m_dstFile.toUtf8().constData());
    if (producer.is_valid()) {
        if (!qstrcmp(producer.get("mlt_service"), "avformat")) {
            producer.set("mlt_service", "avformat-novalidate");
            producer.set("mute_on_pause", 0);
        }
        MLT.lockCreationTime(&producer);
        producer.set_in_and_out(m_in, -1);
        MAIN.replaceInTimeline(m_uuid, producer);
    }
}

void ConvertReplacePostJobAction::doAction()
{
    FilePropertiesPostJobAction::doAction();
    Mlt::Producer producer(MLT.profile(), m_dstFile.toUtf8().constData());
    if (producer.is_valid()) {
        if (!qstrcmp(producer.get("mlt_service"), "avformat")) {
            producer.set("mlt_service", "avformat-novalidate");
            producer.set("mute_on_pause", 0);
        }
        MLT.lockCreationTime(&producer);
        MAIN.replaceAllByHash(m_hash, producer);
    }
}

void ProxyReplacePostJobAction::doAction()
{
    QFileInfo info(m_dstFile);
    QString newFileName = info.path() + "/" + info.baseName() + "." + info.suffix();
    if (QFile::rename(m_dstFile, newFileName)) {
        Mlt::Producer producer(MLT.profile(), newFileName.toUtf8().constData());
        if (producer.is_valid()) {
            producer.set(kIsProxyProperty, 1);
            producer.set(kOriginalResourceProperty, m_srcFile.toUtf8().constData());
            if (!qstrcmp(producer.get("mlt_service"), "avformat")) {
                producer.set("mlt_service", "avformat-novalidate");
                producer.set("mute_on_pause", 0);
            }
            MAIN.replaceAllByHash(m_hash, producer, true);
        } else {
            LOG_WARNING() << "proxy file is invalid" << newFileName;
            QFile::remove(m_dstFile);
        }
    } else {
        LOG_WARNING() << "failed to rename" << m_dstFile << "as" << newFileName;
        QFile::remove(m_dstFile);
    }
}

void ProxyFinalizePostJobAction::doAction()
{
    QFileInfo info(m_dstFile);
    QString newFileName = info.path() + "/" + info.baseName() + "." + info.suffix();
    if (!QFile::rename(m_dstFile, newFileName)) {
        LOG_WARNING() << "failed to rename" << m_dstFile << "as" << newFileName;
        QFile::remove(m_dstFile);
    }
}
