/*
 * Copyright (c) 2012 Meltytech, LLC
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

#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#include <QProcess>
#include <QStandardItemModel>
#include <QMutex>

class MeltJob : public QProcess
{
    Q_OBJECT
public:
    MeltJob(const QString& name, const QString& xml);
    ~MeltJob();
    void start();
    void setModelIndex(const QModelIndex& index);
    QModelIndex modelIndex() const;
    bool ran() const;
    bool stopped() const;
    void appendToLog(const QString&);
    QString log() const;
    QString xml() const;

public slots:
    void stop();

signals:
    void messageAvailable(MeltJob* job);
    void finished(MeltJob* job, bool isSuccess);

private:
    QString m_xml;
    QModelIndex m_index;
    bool m_ran;
    bool m_killed;
    QString m_log;

private slots:
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onReadyRead();
};

class JobQueue : public QStandardItemModel
{
    Q_OBJECT
protected:
    enum ColumnRole {
        COLUMN_OUTPUT,
        COLUMN_STATUS,
        COLUMN_COUNT
    };
    JobQueue(QObject *parent);
    void startNextJob();

public:
    static JobQueue& singleton(QObject* parent = 0);
    void cleanup();
    MeltJob* add(MeltJob *job);
    MeltJob* jobFromIndex(const QModelIndex& index) const;
    void pause();
    void resume();
    bool isPaused() const;

signals:
    void jobAdded();

public slots:
    void onMessageAvailable(MeltJob* job);
    void onFinished(MeltJob* job, bool isSuccess);

private:
    QList<MeltJob*> m_jobs;
    QMutex m_mutex; // protects m_jobs
    bool m_paused;
};

#define JOBS JobQueue::singleton()

#endif // JOBQUEUE_H
