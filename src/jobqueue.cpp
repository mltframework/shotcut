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
#include <QtWidgets>
#include <QDebug>
#include "mainwindow.h"
#include "settings.h"

MeltJob::MeltJob(const QString& name, const QString& xml)
    : QProcess(0)
    , m_xml(xml)
    , m_ran(false)
    , m_killed(false)
    , m_label(name)
{
    setObjectName(name);
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int, QProcess::ExitStatus)));
    connect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

MeltJob::~MeltJob()
{
    QFile::remove(m_xml);
}

void MeltJob::start()
{
    QString shotcutPath = qApp->applicationDirPath();
//    QString shotcutPath("/Applications/Shotcut.app/Contents/MacOS");
#ifdef Q_OS_WIN
    QFileInfo meltPath(shotcutPath, QString(Settings.playerGPU()? "melt.exe" : "qmelt.exe"));
#else
    QFileInfo meltPath(shotcutPath, QString(Settings.playerGPU()? "melt" : "qmelt"));
#endif
    setReadChannel(QProcess::StandardError);
    QStringList args;
    args << "-progress2";
    args << m_xml;
    qDebug() << meltPath.absoluteFilePath() << args;
#ifdef Q_OS_WIN
    QProcess::start(meltPath.absoluteFilePath(), args);
#else
    args.prepend(meltPath.absoluteFilePath());
    QProcess::start("/usr/bin/nice", args);
#endif
    m_ran = true;
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

void MeltJob::appendToLog(const QString& s)
{
    m_log.append(s);
}

QString MeltJob::log() const
{
    return m_log;
}

QString MeltJob::xml() const
{
    QFile f(m_xml);
    f.open(QIODevice::ReadOnly);
    QString s(f.readAll());
    f.close();
    return s;
}

void MeltJob::setLabel(const QString &label)
{
    m_label = label;
}

void MeltJob::stop()
{
    terminate();
    QTimer::singleShot(2000, this, SLOT(kill()));
    m_killed = true;
}

void MeltJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
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

MeltJob* JobQueue::add(MeltJob* job)
{
    int row = rowCount();
    QList<QStandardItem*> items;
    items << new QStandardItem(job->label());
    items << new QStandardItem(tr("pending"));
    appendRow(items);
    job->setParent(this);
    job->setModelIndex(index(row, COLUMN_STATUS));
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
//    qDebug() << msg;
    if (msg.contains("percentage:")) {
        QStandardItem* item = itemFromIndex(job->modelIndex());
        if (item) {
            uint percent = msg.mid(msg.indexOf("percentage:") + 11).toUInt();
            item->setText(QString("%1%").arg(percent));
        }
    }
    else {
        job->appendToLog(msg);
    }
}

void JobQueue::onFinished(MeltJob* job, bool isSuccess)
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
