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

#include "jobqueue.h"
#include <QtGui>
#include <QDebug>

MeltJob::MeltJob(const QString& name, const QString& xml)
    : QProcess(0)
    , m_xml(xml)
    , m_ran(false)
{
    setObjectName(name);
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int, QProcess::ExitStatus)));
    connect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

void MeltJob::start()
{
    QString shotcutPath = qApp->applicationDirPath();
//    QString shotcutPath("/Applications/Shotcut.app/Contents/MacOS");
#ifdef Q_WS_WIN
    QFileInfo meltPath(shotcutPath, "melt.exe");
#else
    QFileInfo meltPath(shotcutPath, "melt");
#endif
    setReadChannel(QProcess::StandardError);
    QStringList args;
//    args << "-verbose";
    args << m_xml;
    qDebug() << meltPath.absoluteFilePath() << args.join(" ");
#ifdef Q_WS_WIN
    QProcess::start(meltPath.absoluteFilePath(), args);
#else
    args.prepend(meltPath.absoluteFilePath());
    QProcess::start("/usr/bin/nice", args);
    m_ran = true;
#endif
}

void MeltJob::setModelIndex(const QModelIndex& index)
{
    m_index = index;
}

QModelIndex MeltJob::modelIndex() const
{
    return m_index;
}

bool MeltJob::ran() const
{
    return m_ran;
}

bool MeltJob::stopped() const
{
    return m_killed;
}

void MeltJob::stop()
{
    terminate();
    kill();
    m_killed = true;
}

void MeltJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QFile::remove(m_xml);
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        qDebug() << "melt succeeeded";
        emit finished(this, true);
    } else {
        qDebug() << "melt failed with" << exitCode;
        emit finished(this, false);
    }
}

void MeltJob::onReadyRead()
{
    emit messageAvailable(this);
}

///////////////////////////////////////////////////////////////////////////////

JobQueue::JobQueue(QObject *parent) :
    QStandardItemModel(0, COLUMN_COUNT, parent)
{
}

JobQueue::~JobQueue()
{
    QMutexLocker locker(&m_mutex);
    qDeleteAll(m_jobs);
}

JobQueue& JobQueue::singleton(QObject* parent)
{
    static JobQueue* instance = 0;
    if (!instance)
        instance = new JobQueue(parent);
    return *instance;
}

MeltJob* JobQueue::add(MeltJob* job)
{
    int row = JOBS.rowCount();
    QList<QStandardItem*> items;
    items << new QStandardItem(job->objectName());
    items << new QStandardItem(tr("pending"));
    JOBS.appendRow(items);
    job->setParent(this);
    job->setModelIndex(JOBS.index(row, COLUMN_STATUS));
    connect(job, SIGNAL(messageAvailable(MeltJob*)), this, SLOT(onMessageAvailable(MeltJob*)));
    connect(job, SIGNAL(finished(MeltJob*, bool)), this, SLOT(onFinished(MeltJob*, bool)));
    m_mutex.lock();
    m_jobs.append(job);
    m_mutex.unlock();
    emit jobAdded();
    startNextJob();

    return job;
}

void JobQueue::onMessageAvailable(MeltJob* job)
{
    QString msg = job->readLine();
    if (msg.contains("percentage:")) {
        QStandardItem* item = JOBS.itemFromIndex(job->modelIndex());
        if (item) {
            uint percent = msg.mid(msg.indexOf("percentage:") + 11).toUInt();
            item->setText(QString("%1%").arg(percent));
        }
    }
}

void JobQueue::onFinished(MeltJob* job, bool isSuccess)
{
    QStandardItem* item = JOBS.itemFromIndex(job->modelIndex());
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
    QMutexLocker locker(&m_mutex);
    if (!m_jobs.isEmpty()) {
        foreach(MeltJob* job, m_jobs) {
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

MeltJob* JobQueue::jobFromIndex(const QModelIndex& index) const
{
    return m_jobs.at(index.row());
}
