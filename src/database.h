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

#ifndef DATABASE_H
#define DATABASE_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>

struct DatabaseJob;
class QTimer;
class Database : public QThread
{
    Q_OBJECT
    explicit Database(QObject *parent = 0);

public:
    static Database& singleton(QWidget* parent = 0);

    bool upgradeVersion1();
    bool putThumbnail(const QString& hash, const QImage& image);
    QImage getThumbnail(const QString& hash);

private slots:
    void commitTransaction();

private slots:
    void shutdown();

private:
    void doJob(DatabaseJob * job);
    void submitAndWaitForJob(DatabaseJob * job);
    void deleteOldThumbnails();
    void run();

    QList<DatabaseJob*> m_jobs;
    QMutex m_mutex;
    QWaitCondition m_waitForFinished;
    QWaitCondition m_waitForNewJob;
    QTimer * m_commitTimer;
};

#define DB Database::singleton()

#endif // DATABASE_H
