/*
 * Copyright (c) 2012-2017 Meltytech, LLC
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

#include "abstractjob.h"
#include <QApplication>
#include <QTimer>
#include <Logger.h>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

AbstractJob::AbstractJob(const QString& name)
    : QProcess(0)
    , m_item(0)
    , m_ran(false)
    , m_killed(false)
    , m_label(name)
{
    setObjectName(name);
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int, QProcess::ExitStatus)));
    connect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(this, SIGNAL(started()), this, SLOT(onStarted()));
}

void AbstractJob::start()
{
    m_ran = true;
    m_time.start();
    emit progressUpdated(m_item, 0);
}

void AbstractJob::setStandardItem(QStandardItem* item)
{
    m_item = item;
}

QStandardItem* AbstractJob::standardItem()
{
    return m_item;
}

bool AbstractJob::ran() const
{
    return m_ran;
}

bool AbstractJob::stopped() const
{
    return m_killed;
}

void AbstractJob::appendToLog(const QString& s)
{
    m_log.append(s);
}

QString AbstractJob::log() const
{
    return m_log;
}

void AbstractJob::setLabel(const QString &label)
{
    m_label = label;
}

QTime AbstractJob::estimateRemaining(int percent)
{
    QTime result;
    if (percent) {
        int averageMs = m_time.elapsed() / percent;
        result = QTime::fromMSecsSinceStartOfDay(averageMs * (100 - percent));
    }
    return result;
}

void AbstractJob::stop()
{
    closeWriteChannel();
    terminate();
    QTimer::singleShot(2000, this, SLOT(kill()));
    m_killed = true;
}

void AbstractJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_log.append(readAll());
    const QTime& time = QTime::fromMSecsSinceStartOfDay(m_time.elapsed());
    if (exitStatus == QProcess::NormalExit && exitCode == 0 && !m_killed) {
        LOG_INFO() << "job succeeeded";
        m_log.append(QString("Completed successfully in %1\n").arg(time.toString()));
        emit progressUpdated(m_item, 100);
        emit finished(this, true);
    } else if (m_killed) {
        LOG_INFO() << "job stopped";
        m_log.append(QString("Stopped by user at %1\n").arg(time.toString()));
        emit finished(this, false);
    } else {
        LOG_INFO() << "job failed with" << exitCode;
        m_log.append(QString("Failed with exit code %1\n").arg(exitCode));
        emit finished(this, false);
    }
}

void AbstractJob::onReadyRead()
{
    QString msg = readLine();
    appendToLog(msg);
}

void AbstractJob::onStarted()
{
#ifdef Q_OS_WIN
    qint64 processId = QProcess::processId();
    HANDLE processHandle = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processId);
    if(processHandle)
    {
        SetPriorityClass(processHandle, IDLE_PRIORITY_CLASS);
        CloseHandle(processHandle);
    }
#endif
}
