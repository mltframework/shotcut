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

#include "abstractjob.h"
#include <QApplication>
#include <QTimer>
#include <Logger.h>

AbstractJob::AbstractJob(const QString& name)
    : QProcess(0)
    , m_ran(false)
    , m_killed(false)
    , m_label(name)
{
    setObjectName(name);
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int, QProcess::ExitStatus)));
    connect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

void AbstractJob::start()
{
    m_ran = true;
}

void AbstractJob::setModelIndex(const QModelIndex& index)
{
    m_index = index;
}

QModelIndex AbstractJob::modelIndex() const
{
    return m_index;
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
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        LOG_DEBUG() << "job succeeeded";
        emit finished(this, true);
    } else {
        LOG_DEBUG() << "job failed with" << exitCode;
        emit finished(this, false);
    }
}

void AbstractJob::onReadyRead()
{
    QString msg = readLine();
    appendToLog(msg);
}
