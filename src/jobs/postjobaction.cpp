/*
 * Copyright (c) 2018-2026 Meltytech, LLC
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

#include "Logger.h"
#include "docks/playlistdock.h"
#include "docks/subtitlesdock.h"
#include "mainwindow.h"
#include "shotcut_mlt_properties.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QString>

// For file time functions in FilePropertiesPostJobAction::doAction();
#include <sys/stat.h>
#include <utime.h>

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

void OpenPostJobAction::doAction()
{
    FilePropertiesPostJobAction::doAction();
    if (!m_fileNameToRemove.isEmpty()) {
        QFile::remove(m_fileNameToRemove);
    }
    MAIN.open(m_dstFile);
    MAIN.playlistDock()->onAppendCutActionTriggered();
}

void ReplaceOnePostJobAction::doAction()
{
    FilePropertiesPostJobAction::doAction();
    if (!m_fileNameToRemove.isEmpty()) {
        QFile::remove(m_fileNameToRemove);
    }
    Mlt::Producer newProducer(MLT.profile(), m_dstFile.toUtf8().constData());
    if (newProducer.is_valid()) {
        Mlt::Producer *producer = MLT.setupNewProducer(&newProducer);
        producer->set_in_and_out(m_in, -1);
        MAIN.replaceInTimeline(m_uuid, *producer);
        delete producer;
    }
}

void ReplaceAllPostJobAction::doAction()
{
    FilePropertiesPostJobAction::doAction();
    Mlt::Producer newProducer(MLT.profile(), m_dstFile.toUtf8().constData());
    if (newProducer.is_valid()) {
        Mlt::Producer *producer = MLT.setupNewProducer(&newProducer);
        MAIN.replaceAllByHash(m_hash, *producer);
        delete producer;
    }
}

void ProxyReplacePostJobAction::doAction()
{
    // Inject rotation metadata via a fast -c copy pass before the final rename.
    if (m_rotation != 0) {
        QString shotcutPath = qApp->applicationDirPath();
        QString ffmpegExe = QDir(shotcutPath).filePath("ffmpeg");
        QString tempFile = m_dstFile + ".rot.mp4";
        QStringList args;
        args << QStringLiteral("-display_rotation:v:0") << QString::number(m_rotation)
             << QStringLiteral("-i") << m_dstFile << QStringLiteral("-c") << QStringLiteral("copy")
             << QStringLiteral("-y") << tempFile;
        if (QProcess::execute(ffmpegExe, args) == 0) {
            QFile::remove(m_dstFile);
            QFile::rename(tempFile, m_dstFile);
        } else {
            LOG_WARNING() << "rotation injection failed for" << m_dstFile;
            QFile::remove(tempFile);
        }
    }
    FilePropertiesPostJobAction::doAction();
    QFileInfo info(m_dstFile);
    QString newFileName = info.path() + "/" + info.baseName() + "." + info.suffix();
    QFile::remove(newFileName);
    if (QFile::rename(m_dstFile, newFileName)) {
        Mlt::Producer newProducer(MLT.profile(), newFileName.toUtf8().constData());
        if (newProducer.is_valid()) {
            Mlt::Producer *producer = MLT.setupNewProducer(&newProducer);
            producer->set(kIsProxyProperty, 1);
            producer->set(kOriginalResourceProperty, m_srcFile.toUtf8().constData());
            MAIN.replaceAllByHash(m_hash, *producer, true);
            delete producer;
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
    // Inject rotation metadata via a fast -c copy pass before the final rename.
    if (m_rotation != 0) {
        QString shotcutPath = qApp->applicationDirPath();
        QString ffmpegExe = QDir(shotcutPath).filePath("ffmpeg");
        QString tempFile = m_dstFile + ".rot.mp4";
        QStringList args;
        args << QStringLiteral("-display_rotation:v:0") << QString::number(m_rotation)
             << QStringLiteral("-i") << m_dstFile << QStringLiteral("-c") << QStringLiteral("copy")
             << QStringLiteral("-y") << tempFile;
        if (QProcess::execute(ffmpegExe, args) == 0) {
            QFile::remove(m_dstFile);
            QFile::rename(tempFile, m_dstFile);
        } else {
            LOG_WARNING() << "rotation injection failed for" << m_dstFile;
            QFile::remove(tempFile);
        }
    }
    FilePropertiesPostJobAction::doAction();
    QFileInfo info(m_dstFile);
    QString newFileName = info.path() + "/" + info.baseName() + "." + info.suffix();
    if (!QFile::rename(m_dstFile, newFileName)) {
        LOG_WARNING() << "failed to rename" << m_dstFile << "as" << newFileName;
        QFile::remove(m_dstFile);
    }
}

void ImportSrtPostJobAction::doAction()
{
    m_dock->importSrtFromFile(m_srtFile, m_trackName, m_lang, m_includeNonspoken);
}
