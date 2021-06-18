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

#ifndef DATABASE_H
#define DATABASE_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>

struct DatabaseJob;
class QTimer;

class Worker : public QObject
{
    Q_OBJECT

public:
    void submitAndWaitForJob(DatabaseJob * job);
    void quit();

signals:
    void opened(bool);
    void failing(bool);

public slots:
    void run();

private slots:
    void commitTransaction();

private:
    bool upgradeVersion1();
    void doJob(DatabaseJob * job);
    void deleteOldThumbnails();

    QList<DatabaseJob*> m_jobs;
    QMutex m_mutex;
    QWaitCondition m_waitForFinished;
    QWaitCondition m_waitForNewJob;
    QTimer* m_commitTimer {nullptr};
    bool m_quit {false};
};

class Database : public QObject
{
    Q_OBJECT
    explicit Database(QObject *parent = 0);

public:
    static Database& singleton(QObject *parent = 0);

    bool putThumbnail(const QString& hash, const QImage& image);
    QImage getThumbnail(const QString& hash);
    bool isShutdown() const;
    bool isFailing() const { return m_isFailing; }

signals:
    void start();

private slots:
    void shutdown();
    void onOpened(bool success) { m_isOpened = success; }
    void onFailing(bool failing) { m_isFailing = failing; }

private:
    Worker m_worker;
    QThread m_thread;
    bool m_isFailing {false};
    bool m_isOpened {false};
};

#define DB Database::singleton()

#endif // DATABASE_H
