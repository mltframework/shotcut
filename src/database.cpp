/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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

#include "database.h"
#include "models/playlistmodel.h"
#include "mainwindow.h"
#include "settings.h"
#include "dialogs/longuitask.h"
#include <QtSql>
#include <QDir>
#include <QFileInfo>
#include <Logger.h>
#include <utime.h>

static QMutex g_mutex;
static Database* instance = nullptr;
static const int kMaxThumbnailCount = 10000;
static const int kDeleteThumbnailsTimeoutMs = 2000;

Database::Database(QObject *parent) : QObject(parent)
{
    m_deleteTimer.setInterval(kDeleteThumbnailsTimeoutMs);
    m_deleteTimer.setSingleShot(true);
    connect(&m_deleteTimer, &QTimer::timeout, this, &Database::deleteOldThumbnails);
}

Database &Database::singleton(QObject *parent)
{
    QMutexLocker locker(&g_mutex);
    if (!instance) {
        instance = new Database(parent);
    }
    return *instance;
}

QDir Database::thumbnailsDir()
{
    QDir dir(Settings.appDataLocation());
    const char* subfolder = "thumbnails";
    if (!dir.cd(subfolder)) {
        if (dir.mkdir(subfolder)) {
            dir.cd(subfolder);

            LongUiTask longTask(QObject::tr("Converting Thumbnails"));
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
            QDir appDir(Settings.appDataLocation());
            QString dbFilePath = appDir.filePath("db.sqlite3");
            db.setDatabaseName(dbFilePath);
            if (db.open()) {
                QSqlQuery query;
                QImage img;
                query.setForwardOnly(true);
                int n = -1;
                if (query.exec("SELECT COUNT(*) FROM thumbnails;") && query.next()) {
                    n = query.value(0).toInt();
                }
                query.exec("SELECT hash, accessed, image FROM thumbnails;");
                for (int i = 0; query.next(); i++) {
                    QString fileName = query.value(0).toString() + ".png";
                    longTask.reportProgress(QObject::tr("Please wait for this one-time update to the thumbnail cache..."), i, n);
                    if (img.loadFromData(query.value(2).toByteArray(), "PNG")) {
                        img.save(dir.filePath(fileName));
                        auto accessed = query.value(1).toDateTime();
                        auto offset = accessed.timeZone().offsetFromUtc(accessed);
                        struct utimbuf utimes {accessed.toSecsSinceEpoch() + offset, accessed.toSecsSinceEpoch() + offset};
                        ::utime(dir.filePath(fileName).toUtf8().constData(), &utimes);
                    }
                }
                db.close();
                QSqlDatabase::removeDatabase("QSQLITE");
            }
            QFile::remove(dbFilePath);
        }
    }
    return dir;
}

bool Database::putThumbnail(const QString& hash, const QImage& image)
{
    m_deleteTimer.start();
    return image.save(thumbnailsDir().filePath(hash + ".png"));
}

QImage Database::getThumbnail(const QString &hash)
{
    QString filePath = thumbnailsDir().filePath(hash + ".png");
    ::utime(filePath.toUtf8().constData(), nullptr);
    return QImage(filePath);
}

void Database::deleteOldThumbnails()
{
    QDir dir = thumbnailsDir();
    auto ls = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDir::Time);
    for (int i = kMaxThumbnailCount; i < ls.size(); i++) {
        QString filePath = dir.filePath(ls[i]);
        if (!QFile::remove(filePath)) {
            LOG_WARNING() << "failed to delete" << filePath;
        }
    }
}
