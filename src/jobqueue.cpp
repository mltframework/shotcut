/*
 * Copyright (c) 2012-2020 Meltytech, LLC
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
#include "settings.h"
#ifdef Q_OS_WIN
#include "windowstools.h"
#endif

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
    foreach (AbstractJob* job, m_jobs) {
        if (job->state() == QProcess::Running) {
            job->stop();
            break;
        }
    }
    qDeleteAll(m_jobs);
}

AbstractJob* JobQueue::add(AbstractJob* job)
{
    QList<QStandardItem*> items;
    QIcon icon = QIcon::fromTheme("run-build", QIcon(":/icons/oxygen/32x32/actions/run-build.png"));
    items << new QStandardItem(icon, "");
    QStandardItem* item = new QStandardItem(job->label());
    items << item;
    item = new QStandardItem(tr("pending"));
#ifdef Q_OS_MAC
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(QGuiApplication::font().pointSize());
    item->setFont(font);
#endif
    item->setToolTip(tr("Estimated Hours:Minutes:Seconds"));
    items << item;
    appendRow(items);
    job->setParent(this);
    job->setStandardItem(item);
    connect(job, SIGNAL(progressUpdated(QStandardItem*, int)), SLOT(onProgressUpdated(QStandardItem*, int)));
    connect(job, SIGNAL(finished(AbstractJob*, bool, QString)), SLOT(onFinished(AbstractJob*, bool, QString)));
    m_mutex.lock();
    m_jobs.append(job);
    m_mutex.unlock();
    emit jobAdded();
    startNextJob();

    return job;
}

void JobQueue::onProgressUpdated(QStandardItem* standardItem, int percent)
{
    if (standardItem) {
        AbstractJob* job = m_jobs.at(standardItem->row());
        if (job) {
            QString remaining = "--:--:--";
            if (percent > 2)
                remaining = job->estimateRemaining(percent).toString();
            standardItem->setText(QString("%1% (%2)").arg(percent).arg(remaining));
        }
    }
#ifdef Q_OS_WIN
    WindowsTaskbarButton::getInstance().setProgress(percent);
#endif
}

void JobQueue::onFinished(AbstractJob* job, bool isSuccess, QString time)
{
    QStandardItem* item = job->standardItem();
    if (item) {
        QIcon icon;
        if (isSuccess) {
            const QTime& time = QTime::fromMSecsSinceStartOfDay(job->time().elapsed());
            item->setText(time.toString());
            item->setToolTip(tr("Elapsed Hours:Minutes:Seconds"));
            icon = QIcon(":/icons/oxygen/32x32/status/task-complete.png");
        } else if (job->stopped()) {
            item->setText(tr("stopped"));
            icon = QIcon(":/icons/oxygen/32x32/status/task-attempt.png");
        } else {
            item->setText(tr("failed").append(' ').append(time));
            icon = QIcon(":/icons/oxygen/32x32/status/task-reject.png");
        }

        // Remove any touched or incomplete pending proxy files
        if (job->stopped() || !isSuccess)
        if (job->objectName().contains("proxies") && job->objectName().contains(".pending")) {
            QFile::remove(job->objectName());
        }

        item = JOBS.item(item->row(), JobQueue::COLUMN_ICON);
        if (item)
            item->setIcon(icon);
    }
#ifdef Q_OS_WIN
    WindowsTaskbarButton::getInstance().resetProgress();
#endif

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

    m_mutex.unlock();
}

void JobQueue::removeFinished()
{
    QMutexLocker locker(&m_mutex);
    auto row = 0;
    foreach (AbstractJob* job, m_jobs) {
        if (job->ran() && job->state() != QProcess::Running) {
            removeRow(row);
            m_jobs.removeOne(job);
            delete job;
        } else {
            ++row;
        }
    }
}
