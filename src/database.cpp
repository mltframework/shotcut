/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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
#include "mainwindow.h"
#include <QtSql>
#include <QStandardPaths>
#include <QDir>
#include <QtDebug>

struct DatabaseJob {
    enum Type {
        PutThumbnail,
        GetThumbnail
    } type;

    QImage image;
    QString hash;
    bool result;
    bool completed;
    DatabaseJob()
        : result(false)
        , completed(false)
    {}
};

static Database* instance = 0;

Database::Database(QObject *parent) :
    QThread(parent)
    , m_commitTimer(0)
{
}

Database &Database::singleton(QWidget *parent)
{
    if (!instance) {
        instance = new Database(parent);
        instance->start();
    }
    return *instance;
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

void Database::doJob(DatabaseJob * job)
{
    if (!m_commitTimer->isActive())
        QSqlDatabase::database().transaction();
    m_commitTimer->start();

    if (job->type == DatabaseJob::PutThumbnail) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        job->image.save(&buffer, "PNG");

        QSqlQuery query;
        query.prepare("DELETE FROM thumbnails WHERE hash = :hash;");
        query.bindValue(":hash", job->hash);
        query.exec();
        query.prepare("INSERT INTO thumbnails VALUES (:hash, datetime('now'), :image);");
        query.bindValue(":hash", job->hash);
        query.bindValue(":image", ba);
        job->result = query.exec();
        if (!job->result)
            qCritical() << __FUNCTION__ << query.lastError();
    } else if (job->type == DatabaseJob::GetThumbnail) {
        QImage result;
        QSqlQuery query;
        query.prepare("SELECT image FROM thumbnails WHERE hash = :hash;");
        query.bindValue(":hash", job->hash);
        if (query.exec() && query.first()) {
            result.loadFromData(query.value(0).toByteArray(), "PNG");
            QSqlQuery update;
            update.prepare("UPDATE thumbnails SET accessed = datetime('now') WHERE hash = :hash ;");
            update.bindValue(":hash", job->hash);
            if (!update.exec())
                qCritical() << __FUNCTION__ << update.lastError();
        }
        job->image = result;
    }
    deleteOldThumbnails();
    job->completed = true;
}

void Database::commitTransaction()
{
    QSqlDatabase::database().commit();
}

bool Database::putThumbnail(const QString& hash, const QImage& image)
{
    DatabaseJob job;
    job.type = DatabaseJob::PutThumbnail;
    job.hash = hash;
    job.image = image;
    submitAndWaitForJob(&job);
    return job.result;
}

void Database::submitAndWaitForJob(DatabaseJob * job)
{
    job->completed = false;
    m_mutex.lock();
    m_jobs.append(job);
    if (m_jobs.size() == 1) {
        //worker was idle until now
        m_waitForNewJob.wakeAll();
    }
    while (!job->completed) {
        m_waitForFinished.wait(&m_mutex);
    }
    m_mutex.unlock();
}

QImage Database::getThumbnail(const QString &hash)
{
    DatabaseJob job;
    job.type = DatabaseJob::GetThumbnail;
    job.hash = hash;
    submitAndWaitForJob(&job);
    return job.image;
}

void Database::shutdown()
{
    requestInterruption();
    wait();
    QString connection = QSqlDatabase::database().connectionName();
    QSqlDatabase::database().close();
    QSqlDatabase::removeDatabase(connection);
    instance = 0;
}

void Database::deleteOldThumbnails()
{
    QSqlQuery query;
    // OFFSET is the numner of thumbnails to cache.
    if (!query.exec("DELETE FROM thumbnails WHERE hash IN (SELECT hash FROM thumbnails ORDER BY accessed DESC LIMIT -1 OFFSET 10000);"))
        qCritical() << __FUNCTION__ << query.lastError();
}

void Database::run()
{
    connect(&MAIN, SIGNAL(aboutToShutDown()),
            this, SLOT(shutdown()), Qt::DirectConnection);

    QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
    if (!dir.exists())
        dir.mkpath(dir.path());

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dir.filePath("db.sqlite3"));
    db.open();

    m_commitTimer = new QTimer();
    m_commitTimer->setSingleShot(true);
    m_commitTimer->setInterval(5000);
    connect(m_commitTimer, SIGNAL(timeout()),
            this, SLOT(commitTransaction()));

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

    while (true) {
        DatabaseJob * newJob = 0;
        m_mutex.lock();
        if (m_jobs.isEmpty())
            m_waitForNewJob.wait(&m_mutex, 1000);
        else
            newJob = m_jobs.takeFirst();
        m_mutex.unlock();
        QCoreApplication::processEvents();
        if (newJob) {
            doJob(newJob);
            m_waitForFinished.wakeAll();
        }
        if (isInterruptionRequested())
            break;
    }
    if (m_commitTimer->isActive())
        commitTransaction();
    delete m_commitTimer;
}

