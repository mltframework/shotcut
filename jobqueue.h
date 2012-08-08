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
    void start();
    void setModelIndex(const QModelIndex& index);
    QModelIndex modelIndex() const;
    bool ran() const;

signals:
    void messageAvailable(MeltJob* job);
    void finished(MeltJob* job, bool isSuccess);

private:
    QString m_xml;
    QModelIndex m_index;
    bool m_ran;

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
    ~JobQueue();
    void startNextJob();

public:
    static JobQueue& singleton(QObject* parent = 0);
    MeltJob* add(MeltJob *job);

signals:
    void jobAdded();

public slots:
    void onMessageAvailable(MeltJob* job);
    void onFinished(MeltJob* job, bool isSuccess);

private:
    QList<MeltJob*> m_jobs;
    QMutex m_mutex; // protects m_jobs
};

#define JOBS JobQueue::singleton()

#endif // JOBQUEUE_H
