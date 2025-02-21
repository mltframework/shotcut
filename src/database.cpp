/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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

#include "Logger.h"
#include "dialogs/longuitask.h"
#include "settings.h"

#include <utime.h>
#include <QDir>
#include <QFileInfo>
#include <QtSql>

static QMutex g_mutex;
static Database *instance = nullptr;
static const int kMaxThumbnailCount = 5000;
static const int kDeleteThumbnailsTimeoutMs = 60000;

Database::Database(QObject *parent)
    : QObject(parent)
{
    m_deleteTimer.setInterval(kDeleteThumbnailsTimeoutMs);
    connect(&m_deleteTimer, SIGNAL(timeout()), this, SLOT(deleteOldThumbnails()));
    thumbnailsDir(); // convert from db to filesystem if needed
    m_deleteTimer.start();
}

Database &Database::singleton(QObject *parent)
{
    QMutexLocker locker(&g_mutex);
    if (!instance) {
        instance = new Database(parent);
    }
    return *instance;
}

static QString toFileName(const QString &s)
{
    QString result = s;
    return result.replace(':', '-') + +".png";
}

QDir Database::thumbnailsDir()
{
    QDir dir(Settings.appDataLocation());
    const char *subfolder = "thumbnails";
    if (!dir.cd(subfolder)) {
        if (dir.mkdir(subfolder)) {
            dir.cd(subfolder);

            // Convert the DB data to files on the filesystem.
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
                query.exec(QStringLiteral("SELECT hash, accessed, image FROM thumbnails ORDER BY "
                                          "accessed DESC LIMIT %1")
                               .arg(kMaxThumbnailCount));
                for (int i = 0; query.next(); i++) {
                    QString fileName = toFileName(query.value(0).toString());
                    longTask.reportProgress(
                        QObject::tr(
                            "Please wait for this one-time update to the thumbnail cache..."),
                        i,
                        n);
                    if (img.loadFromData(query.value(2).toByteArray(), "PNG")) {
                        img.save(dir.filePath(fileName));
                        auto accessed = query.value(1).toDateTime();
                        auto offset = accessed.timeZone().offsetFromUtc(accessed);
                        struct utimbuf utimes
                        {
                            static_cast<time_t>(accessed.toSecsSinceEpoch() + offset),
                                static_cast<time_t>(accessed.toSecsSinceEpoch() + offset)
                        };
                        ::utime(dir.filePath(fileName).toUtf8().constData(), &utimes);
                    }
                }
                db.close();
                QSqlDatabase::removeDatabase("QSQLITE");
            }
        }
    }
    return dir;
}

bool Database::putThumbnail(const QString &hash, const QImage &image)
{
    return image.save(thumbnailsDir().filePath(toFileName(hash)));
}

QImage Database::getThumbnail(const QString &hash)
{
    QString filePath = thumbnailsDir().filePath(toFileName(hash));
    ::utime(filePath.toUtf8().constData(), nullptr);
    return QImage(filePath);
}

void Database::deleteOldThumbnails()
{
    auto result = QtConcurrent::run([=]() {
        QDir dir = thumbnailsDir();
        auto ls = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDir::Time);
        if (ls.size() - kMaxThumbnailCount > 0)
            LOG_DEBUG() << "removing" << ls.size() - kMaxThumbnailCount;
        for (int i = kMaxThumbnailCount; i < ls.size(); i++) {
            QString filePath = dir.filePath(ls[i]);
            if (!QFile::remove(filePath)) {
                LOG_WARNING() << "failed to delete" << filePath;
            }
        }
    });
}
