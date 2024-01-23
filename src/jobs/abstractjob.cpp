/*
 * Copyright (c) 2012-2024 Meltytech, LLC
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
#include "postjobaction.h"

#include <QApplication>
#include <QTimer>
#include <QAction>
#include <Logger.h>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <signal.h>
#endif

AbstractJob::AbstractJob(const QString &name, QThread::Priority priority)
    : QProcess(0)
    , m_item(0)
    , m_ran(false)
    , m_killed(false)
    , m_label(name)
    , m_startingPercent(0)
    , m_priority(priority)
{
    setObjectName(name);
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int,
                                                                                     QProcess::ExitStatus)));
    connect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(this, SIGNAL(started()), this, SLOT(onStarted()));
    connect(this, SIGNAL(progressUpdated(QStandardItem *, int)), SLOT(onProgressUpdated(QStandardItem *,
                                                                                        int)));
    m_actionPause = new QAction(tr("Pause This Job"), this);
    m_standardActions << m_actionPause;
    m_actionResume = new QAction(tr("Resume This Job"), this);
    m_actionResume->setEnabled(false);
    m_standardActions << m_actionResume;

    connect(m_actionPause, &QAction::triggered, this, &AbstractJob::pause);
    connect(m_actionResume, &QAction::triggered, this, &AbstractJob::resume);
}

void AbstractJob::start()
{
    m_killed = false;
    m_ran = true;
    m_estimateTime.start();
    m_totalTime.start();
    emit progressUpdated(m_item, 0);
}

void AbstractJob::setStandardItem(QStandardItem *item)
{
    m_item = item;
}

QStandardItem *AbstractJob::standardItem()
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

void AbstractJob::appendToLog(const QString &s)
{
    if (m_log.size() < 100 * 1024 * 1024 /* MiB */) {
        m_log.append(s);
    }
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
        int averageMs = m_estimateTime.elapsed() / qMax(1, percent - qMax(0, m_startingPercent));
        result = QTime::fromMSecsSinceStartOfDay(averageMs * (100 - percent));
    }
    return result;
}

void AbstractJob::setPostJobAction(PostJobAction *action)
{
    m_postJobAction.reset(action);
}

bool AbstractJob::paused() const
{
    return !m_actionPause->isEnabled();
}

void AbstractJob::start(const QString &program, const QStringList &arguments)
{
    QString prog = program;
    QStringList args = arguments;
#ifndef Q_OS_WIN
    if (m_priority == QThread::LowPriority || m_priority == QThread::HighPriority) {
        args.prepend(program);
        args.prepend(m_priority == QThread::LowPriority ? "3" : "-3");
        args.prepend("-n");
        prog = "nice";
    }
#endif
    QProcess::start(prog, args);
    AbstractJob::start();
}

void AbstractJob::stop()
{
    closeWriteChannel();
    terminate();
    QTimer::singleShot(2000, this, SLOT(kill()));
    m_killed = true;
}

void AbstractJob::pause()
{
    m_actionPause->setEnabled(false);
    m_actionResume->setEnabled(true);

#ifdef Q_OS_WIN
    ::DebugActiveProcess(QProcess::processId());
#else
    ::kill(QProcess::processId(), SIGSTOP);
#endif
    emit progressUpdated(m_item, -1);
}

void AbstractJob::resume()
{
    m_actionPause->setEnabled(true);
    m_actionResume->setEnabled(false);
    m_startingPercent = -1;
#ifdef Q_OS_WIN
    ::DebugActiveProcessStop(QProcess::processId());
#else
    ::kill(QProcess::processId(), SIGCONT);
#endif
    emit progressUpdated(m_item, 0);
}

void AbstractJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const QTime &time = QTime::fromMSecsSinceStartOfDay(m_totalTime.elapsed());
    if (isOpen()) {
        m_log.append(readAll());
    }
    if (exitStatus == QProcess::NormalExit && exitCode == 0 && !m_killed) {
        if (m_postJobAction) {
            m_postJobAction->doAction();
        }
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
    QString msg;
    do {
        msg = readLine();
        appendToLog(msg);
    } while (!msg.isEmpty());
}

void AbstractJob::onStarted()
{
#ifdef Q_OS_WIN
    qint64 processId = QProcess::processId();
    HANDLE processHandle = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processId);
    if (processHandle) {
        switch (m_priority) {
        case QThread::LowPriority:
            SetPriorityClass(processHandle, BELOW_NORMAL_PRIORITY_CLASS);
            break;
        case QThread::HighPriority:
            SetPriorityClass(processHandle, ABOVE_NORMAL_PRIORITY_CLASS);
            break;
        default:
            SetPriorityClass(processHandle, NORMAL_PRIORITY_CLASS);
        }
        CloseHandle(processHandle);
    }
#endif
}

void AbstractJob::onProgressUpdated(QStandardItem *, int percent)
{
    // Start timer on first reported percentage > 0.
    if (percent > 0 && (percent == 1 || m_startingPercent < 0)) {
        m_estimateTime.restart();
        m_startingPercent = percent;
    }
}
