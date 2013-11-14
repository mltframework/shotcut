/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include "database.h"
#include "models/playlistmodel.h"
#include <QtSql>
#include <QStandardPaths>
#include <QDir>
#include <QtDebug>

static Database* instance = 0;

Database::Database(QObject *parent) :
    QObject(parent)
{
    QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
    if (!dir.exists())
        dir.mkpath(dir.path());

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dir.filePath("db.sqlite3"));
    db.open();

    // Initialize version table, if needed.
    int version = 0;
    QSqlQuery query;
    if (query.exec("CREATE TABLE version (version INTEGER);")) {
        if (!query.exec("INSERT INTO version VALUES (0);"))
            qCritical() << __PRETTY_FUNCTION__ << "Failed to create version table.";
    } else if (query.exec("SELECT version FROM version")) {
        query.next();
        version = query.value(0).toInt();
    } else {
        qCritical() << __PRETTY_FUNCTION__ << "Failed to get version.";
    }
    if (version < 1 && upgradeVersion1())
        version = 1;
    qDebug() << "Database version is" << version;
}

Database &Database::singleton(QWidget *parent)
{
    if (!instance)
        instance = new Database(parent);
    return *instance;
}

Database::~Database()
{
    QSqlDatabase::database().close();
    QSqlDatabase::removeDatabase(QSqlDatabase::database().connectionName());
    instance = 0;
}

bool Database::upgradeVersion1()
{
    bool success = false;
    QSqlQuery query;
    if (query.exec("CREATE TABLE thumbnails (hash TEXT PRIMARY KEY NOT NULL, accessed DATETIME NOT NULL, image BLOB);")) {
        success = query.exec("UPDATE version SET version = 1;");
        if (!success)
            qCritical() << __FUNCTION__ << query.lastError();
    } else {
        qCritical() << __PRETTY_FUNCTION__ << "Failed to create thumbnails table.";
    }
    return success;
}

bool Database::putThumbnail(const QString& hash, const QImage& image)
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "BMP");

    QSqlQuery query;
    query.prepare("INSERT INTO thumbnails VALUES (:hash, datetime('now'), :image);");
    query.bindValue(":hash", hash);
    query.bindValue(":image", ba);
    bool result = query.exec();
    if (!result)
        qCritical() << __FUNCTION__ << query.lastError();
    deleteOldThumbnails();
    return result;
}

QImage Database::getThumbnail(const QString &hash)
{
    QImage result;
    QSqlQuery query;
    query.prepare("SELECT image FROM thumbnails WHERE hash = :hash;");
    query.bindValue(":hash", hash);
    if (query.exec() && query.first()) {
        result.loadFromData(query.value(0).toByteArray(), "BMP");
        QSqlQuery update;
        update.prepare("UPDATE thumbnails SET accessed = datetime('now') WHERE hash = :hash ;");
        update.bindValue(":hash", hash);
        if (!update.exec())
            qCritical() << __FUNCTION__ << update.lastError();
    }
//    qDebug() << __FUNCTION__ << result.byteCount();
    deleteOldThumbnails();
    return result;
}

void Database::deleteOldThumbnails()
{
    QSqlQuery query;
    // OFFSET is the numner of thumbnails to cache.
    if (!query.exec("DELETE FROM thumbnails WHERE hash IN (SELECT hash FROM thumbnails ORDER BY accessed DESC LIMIT -1 OFFSET 10000);"))
        qCritical() << __FUNCTION__ << query.lastError();
}

