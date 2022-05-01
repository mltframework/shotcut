/*
 * Copyright (c) 2011-2016 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 * Loosely based on ideas from KAutoSaveFile by Jacob R Rideout <kde@jacobrideout.net>
 * and Kdenlive by Jean-Baptiste Mardelle.
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

#include "autosavefile.h"
#include "settings.h"

#include <QtCore/QDir>
#include <QtCore/QCryptographicHash>

static const QLatin1String subdir("/autosave");
static const QLatin1String extension(".mlt");

static QString hashName(const QString &name)
{
    return QString::fromLatin1(QCryptographicHash::hash(name.toUtf8(),
                                                        QCryptographicHash::Md5).toHex());
}

AutoSaveFile::AutoSaveFile(const QString &filename, QObject *parent)
    : QFile(parent)
    , m_managedFileNameChanged(false)
{
    changeManagedFile(filename);
}

AutoSaveFile::~AutoSaveFile()
{
    if (!fileName().isEmpty())
        remove();
}

void AutoSaveFile::changeManagedFile(const QString &filename)
{
    if (!fileName().isEmpty())
        remove();
    m_managedFile = filename;
    m_managedFileNameChanged = true;
}

bool AutoSaveFile::open(OpenMode openmode)
{
    QString tempFile;

    if (m_managedFileNameChanged) {
        QString staleFilesDir = path();
        if (!QDir().mkpath(staleFilesDir)) {
            return false;
        }
        tempFile = staleFilesDir + QChar::fromLatin1('/') + hashName(m_managedFile) + extension;
    } else {
        tempFile = fileName();
    }
    m_managedFileNameChanged = false;
    setFileName(tempFile);

    return QFile::open(openmode);
}

AutoSaveFile *AutoSaveFile::getFile(const QString &filename)
{
    AutoSaveFile *result = 0;
    QDir appDir(path());
    QFileInfo info(appDir.absolutePath(), hashName(filename) + extension);

    if (info.exists()) {
        result = new AutoSaveFile(filename);
        result->setFileName(info.filePath());
        result->m_managedFileNameChanged = false;
    }

    return result;
}

QString AutoSaveFile::path()
{
    return Settings.appDataLocation() + subdir;
}
