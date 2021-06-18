/*
 * Copyright (c) 2013-2018 Meltytech, LLC
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
#include <QtSql>
#include <QDir>
#include <Logger.h>

struct DatabaseJob {
    enum Type {
        PutThumbnail,
        GetThumbnail
    } type;

    QImage image;
    QString hash;
    bool result {false};
    bool completed {false};
};

static QMutex g_mutex;
static Database* instance = nullptr;
static bool g_isShutdown = false;

Database::Database(QObject *parent) : QObject(parent)
{
    m_worker.moveToThread(&m_thread);
    connect(this, &Database::start, &m_worker, &Worker::run);
    connect(&m_worker, &Worker::opened, this, &Database::onOpened);
    connect(&m_worker, &Worker::failing, this, &Database::onFailing);
    connect(&MAIN, &MainWindow::aboutToShutDown, this, &Database::shutdown, Qt::DirectConnection);
    m_thread.start();
}

Database &Database::singleton(QObject *parent)
{
    QMutexLocker locker(&g_mutex);
    if (!instance) {
        instance = new Database(parent);
        emit instance->start();
    }
    return *instance;
}

bool Worker::upgradeVersion1()
{
    bool success = false;
    QSqlQuery query;
    if (query.exec("CREATE TABLE thumbnails (hash TEXT PRIMARY KEY NOT NULL, accessed DATETIME NOT NULL, image BLOB);")) {
        success = query.exec("UPDATE version SET version = 1;");
        if (!success)
            LOG_ERROR() << query.lastError();
    } else {
        LOG_ERROR() << "Failed to create thumbnails table.";
    }
    return success;
}

void Worker::doJob(DatabaseJob * job)
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
            LOG_ERROR() << query.lastError();
        emit failing(!job->result);
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
            auto isFailing = !update.exec();
            emit failing(isFailing);
            if (isFailing)
                LOG_ERROR() << update.lastError();
        }
        job->image = result;
    }
    deleteOldThumbnails();
    job->completed = true;
}

void Worker::commitTransaction()
{
    QSqlDatabase::database().commit();
}

bool Database::putThumbnail(const QString& hash, const QImage& image)
{
    if (!m_isOpened) return false;
    DatabaseJob job;
    job.type = DatabaseJob::PutThumbnail;
    job.hash = hash;
    job.image = image;
    m_worker.submitAndWaitForJob(&job);
    return job.result;
}

void Worker::submitAndWaitForJob(DatabaseJob * job)
{
    job->completed = false;
    QMutexLocker locker(&m_mutex);
    m_jobs.append(job);
    if (m_jobs.size() == 1) {
        //worker was idle until now
        m_waitForNewJob.wakeAll();
    }
    while (!job->completed) {
        m_waitForFinished.wait(&m_mutex);
    }
}

void Worker::quit()
{
    QMutexLocker locker(&m_mutex);
    m_quit = true;
    m_waitForNewJob.wakeAll();
}

QImage Database::getThumbnail(const QString &hash)
{
    if (!m_isOpened) return QImage();
    DatabaseJob job;
    job.type = DatabaseJob::GetThumbnail;
    job.hash = hash;
    m_worker.submitAndWaitForJob(&job);
    return job.image;
}

bool Database::isShutdown() const
{
    return g_isShutdown;
}

void Database::shutdown()
{
    g_isShutdown = true;
    LOG_DEBUG() << "tell worker to quit";
    m_worker.quit();
    LOG_DEBUG() << "quit thread";
    m_thread.quit();
    LOG_DEBUG() << "wait for thread";
    m_thread.wait();
    instance = nullptr;
    LOG_DEBUG() << "end";
}

void Worker::deleteOldThumbnails()
{
    QSqlQuery query;
    // OFFSET is the number of thumbnails to cache.
    if (!query.exec("DELETE FROM thumbnails WHERE hash IN (SELECT hash FROM thumbnails ORDER BY accessed DESC LIMIT -1 OFFSET 10000);"))
        LOG_ERROR() << query.lastError();
}

void Worker::run()
{
    QDir dir(Settings.appDataLocation());
    if (!dir.exists())
        dir.mkpath(dir.path());

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dir.filePath("db.sqlite3"));
    auto result = db.open();
    emit opened(result);
    if (!result) {
        LOG_ERROR() << "database open failed";
        return;
    }

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
            LOG_ERROR() << "Failed to create version table.";
    } else if (query.exec("SELECT version FROM version")) {
        query.next();
        version = query.value(0).toInt();
    } else {
        LOG_ERROR() << "Failed to get version.";
    }
    if (version < 1 && upgradeVersion1())
        version = 1;
    LOG_DEBUG() << "Database version is" << version;

    while (!m_quit) {
        DatabaseJob * newJob = nullptr;
        m_mutex.lock();
        if (m_jobs.isEmpty())
            m_waitForNewJob.wait(&m_mutex, 1000);
        else
            newJob = m_jobs.takeFirst();
        m_mutex.unlock();
        if (newJob) {
            doJob(newJob);
            m_waitForFinished.wakeAll();
        }
    }
    if (m_commitTimer->isActive())
        commitTransaction();
    delete m_commitTimer;

    QString connection = QSqlDatabase::database().connectionName();
    QSqlDatabase::database().close();
//    QSqlDatabase::removeDatabase(connection);
    LOG_DEBUG() << "database closed";
}

