/*
 * Copyright (c) 2012-2016 Meltytech, LLC
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

#include "jobqueue.h"
#include <QtWidgets>
#include <Logger.h>
#include "mainwindow.h"
#include "settings.h"

JobQueue::JobQueue(QObject *parent) :
    QStandardItemModel(0, COLUMN_COUNT, parent),
    m_paused(false)
{
}

JobQueue& JobQueue::singleton(QObject* parent)
{
    static JobQueue* instance = 0;
    if (!instance)
        instance = new JobQueue(parent);
    return *instance;
}

void JobQueue::cleanup()
{
    QMutexLocker locker(&m_mutex);
    qDeleteAll(m_jobs);
}

AbstractJob* JobQueue::add(AbstractJob* job)
{
    int row = rowCount();
    QList<QStandardItem*> items;
    items << new QStandardItem(job->label());
    items << new QStandardItem(tr("pending"));
    appendRow(items);
    job->setParent(this);
    job->setModelIndex(index(row, COLUMN_STATUS));
    connect(job, SIGNAL(progressUpdated(QModelIndex,uint)), this, SLOT(onProgressUpdated(QModelIndex,uint)));
    connect(job, SIGNAL(finished(AbstractJob*, bool)), this, SLOT(onFinished(AbstractJob*, bool)));
    m_mutex.lock();
    m_jobs.append(job);
    m_mutex.unlock();
    emit jobAdded();
    startNextJob();

    return job;
}

void JobQueue::onProgressUpdated(QModelIndex index, uint percent)
{
    QStandardItem* item = itemFromIndex(index);
    if (item)
        item->setText(QString("%1%").arg(percent));
}

void JobQueue::onFinished(AbstractJob* job, bool isSuccess)
{
    QStandardItem* item = itemFromIndex(job->modelIndex());
    if (item) {
        if (isSuccess)
            item->setText(tr("done"));
        else if (job->stopped())
            item->setText(tr("stopped"));
        else
            item->setText(tr("failed"));
    }
    startNextJob();
}

void JobQueue::startNextJob()
{
    if (m_paused) return;
    QMutexLocker locker(&m_mutex);
    if (!m_jobs.isEmpty()) {
        foreach(AbstractJob* job, m_jobs) {
            // if there is already a job started or running, then exit
            if (job->ran() && job->state() != QProcess::NotRunning)
                break;
            // otherwise, start first non-started job and exit
            if (!job->ran()) {
                job->start();
                break;
            }
        }
    }
}

AbstractJob* JobQueue::jobFromIndex(const QModelIndex& index) const
{
    return m_jobs.at(index.row());
}

void JobQueue::pause()
{
    m_paused = true;
}

void JobQueue::resume()
{
    m_paused = false;
    startNextJob();
}

bool JobQueue::isPaused() const
{
    return m_paused;
}

bool JobQueue::hasIncomplete() const
{
    foreach (AbstractJob* job, m_jobs) {
        if (!job->ran() || job->state() == QProcess::Running)
            return true;
    }
    return false;
}

void JobQueue::remove(const QModelIndex& index)
{
    int row = index.row();
    removeRow(index.row());
    m_mutex.lock();

    AbstractJob* job = m_jobs.at(row);
    m_jobs.removeOne(job);
    delete job;

    // Reindex the subsequent jobs.
    for (int i = row; i < m_jobs.size(); ++i) {
        QModelIndex modelIndex = this->index(i, COLUMN_STATUS);
        m_jobs.at(i)->setModelIndex(modelIndex);
    }
    m_mutex.unlock();
}
