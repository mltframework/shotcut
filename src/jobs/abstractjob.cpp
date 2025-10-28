/*
 * Copyright (c) 2012-2025 Meltytech, LLC
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

#include "Logger.h"
#include "postjobaction.h"

#include <QAction>
#include <QApplication>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <signal.h>
#endif

/**
 * @class AbstractJob
 * @brief 后台任务的抽象基类。
 * 
 * 这个类继承自 `QProcess`，封装了在 Shotcut 中运行外部进程（如 `ffmpeg`）的通用逻辑。
 * 它提供了任务状态管理、进度更新、暂停/恢复、日志记录以及任务完成后的操作等功能。
 * 所有具体的后台任务（如 `EncodeJob`）都应该继承自此类。
 */

AbstractJob::AbstractJob(const QString &name, QThread::Priority priority)
    : QProcess(0)
    , m_item(0)                  ///< 在任务列表中显示的 QStandardItem
    , m_ran(false)               ///< 标记任务是否已启动
    , m_killed(false)            ///< 标记任务是否被用户停止
    , m_label(name)              ///< 任务的标签/名称
    , m_startingPercent(0)       ///< 用于估算剩余时间的起始百分比
    , m_priority(priority)       ///< 任务进程的优先级
    , m_isPaused(false)          ///< 标记任务是否处于暂停状态
{
    setObjectName(name);
    // 连接 QProcess 的信号到本类的槽函数
    connect(this,
            SIGNAL(finished(int, QProcess::ExitStatus)),
            this,
            SLOT(onFinished(int, QProcess::ExitStatus)));
    connect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(this, SIGNAL(started()), this, SLOT(onStarted()));
    connect(this,
            SIGNAL(progressUpdated(QStandardItem *, int)),
            SLOT(onProgressUpdated(QStandardItem *, int)));

    // 创建暂停和恢复的动作
    m_actionPause = new QAction(tr("Pause This Job"), this);
    m_standardActions << m_actionPause;
    m_actionPause->setEnabled(false);
    m_actionResume = new QAction(tr("Resume This Job"), this);
    m_actionResume->setEnabled(false);
    m_standardActions << m_actionResume;

    // 连接动作的触发信号到本类的槽
    connect(m_actionPause, &QAction::triggered, this, &AbstractJob::pause);
    connect(m_actionResume, &QAction::triggered, this, &AbstractJob::resume);
    // 任务完成后，禁用暂停/恢复按钮
    connect(this, &AbstractJob::finished, this, [this]() {
        m_actionPause->setEnabled(false);
        m_actionResume->setEnabled(false);
    });
}

/**
 * @brief 启动任务。
 * 这是一个重写函数，用于在启动进程前进行一些初始化工作。
 */
void AbstractJob::start()
{
    m_killed = false;
    m_ran = true;
    m_estimateTime.start(); // 启动用于估算剩余时间的计时器
    m_totalTime.start();    // 启动总计时器
    emit progressUpdated(m_item, 0); // 发出进度更新信号，初始为 0%
}

/// ... (其他简单的 getter/setter 方法)

/**
 * @brief 将字符串追加到任务日志中。
 * @param s 要追加的字符串。
 */
void AbstractJob::appendToLog(const QString &s)
{
    // 限制日志大小，防止占用过多内存
    if (m_log.size() < 100 * 1024 * 1024 /* 100 MiB */) {
        m_log.append(s);
    }
}

/**
 * @brief 根据当前百分比估算剩余时间。
 * @param percent 当前的进度百分比。
 * @return 估算的剩余时间。
 */
QTime AbstractJob::estimateRemaining(int percent)
{
    QTime result;
    if (percent) {
        // 计算每完成 1% 所需的平均时间
        int averageMs = m_estimateTime.elapsed() / qMax(1, percent - qMax(0, m_startingPercent));
        // 估算剩余时间
        result = QTime::fromMSecsSinceStartOfDay(averageMs * (100 - percent));
    }
    return result;
}

/**
 * @brief 设置任务完成后的操作。
 * @param action 指向 PostJobAction 对象的指针，该对象将在任务成功完成后执行。
 */
void AbstractJob::setPostJobAction(PostJobAction *action)
{
    m_postJobAction.reset(action); // 使用智能指针管理生命周期
}

/**
 * @brief 启动外部进程。
 * @param program 要执行的程序。
 * @param arguments 传递给程序的参数列表。
 */
void AbstractJob::start(const QString &program, const QStringList &arguments)
{
    QString prog = program;
    QStringList args = arguments;
#ifndef Q_OS_WIN
    // 在非 Windows 系统上，使用 `nice` 命令来调整进程优先级
    if (m_priority == QThread::LowPriority || m_priority == QThread::HighPriority) {
        args.prepend(program);
        args.prepend(m_priority == QThread::LowPriority ? "3" : "-3"); // nice 值
        args.prepend("-n");
        prog = "nice";
    }
#endif
    QProcess::start(prog, args); // 调用 QProcess 的 start 方法
    AbstractJob::start();        // 调用本类的 start 方法进行初始化
    m_actionPause->setEnabled(true);  // 启用暂停按钮
    m_actionResume->setEnabled(false); // 禁用恢复按钮
    m_isPaused = false;
}

/**
 * @brief 停止任务。
 * 首先尝试优雅地终止进程，如果 2 秒内未退出，则强制杀死。
 */
void AbstractJob::stop()
{
    // 如果任务处于暂停状态，先恢复它，以便它能响应终止信号
    if (paused()) {
#ifdef Q_OS_WIN
        ::DebugActiveProcessStop(QProcess::processId());
#else
        ::kill(QProcess::processId(), SIGCONT);
#endif
    }
    closeWriteChannel(); // 关闭写入通道
    terminate();         // 发送终止信号
    // 设置一个 2 秒的超时，如果进程仍未退出，则强制杀死
    QTimer::singleShot(2000, this, SLOT(kill()));
    m_killed = true;
    m_actionPause->setEnabled(false);
    m_actionResume->setEnabled(false);
}

/**
 * @brief 暂停任务。
 * 通过向进程发送 SIGSTOP (Linux/macOS) 或 DebugActiveProcess (Windows) 信号来实现。
 */
void AbstractJob::pause()
{
    m_isPaused = true;
    m_actionPause->setEnabled(false);
    m_actionResume->setEnabled(true);

#ifdef Q_OS_WIN
    ::DebugActiveProcess(QProcess::processId());
#else
    ::kill(QProcess::processId(), SIGSTOP);
#endif
    emit progressUpdated(m_item, -1); // 使用 -1 表示暂停状态
}

/**
 * @brief 恢复被暂停的任务。
 * 通过向进程发送 SIGCONT (Linux/macOS) 或 DebugActiveProcessStop (Windows) 信号来实现。
 */
void AbstractJob::resume()
{
    m_actionPause->setEnabled(true);
    m_actionResume->setEnabled(false);
    m_startingPercent = -1; // 重置起始百分比，以便重新计算剩余时间
#ifdef Q_OS_WIN
    ::DebugActiveProcessStop(QProcess::processId());
#else
    ::kill(QProcess::processId(), SIGCONT);
#endif
    m_isPaused = false;
    emit progressUpdated(m_item, 0); // 恢复进度显示
}

/**
 * @brief 当进程结束时调用。
 * @param exitCode 进程的退出码。
 * @param exitStatus 进程的退出状态（正常退出或崩溃）。
 */
void AbstractJob::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const QTime &time = QTime::fromMSecsSinceStartOfDay(m_totalTime.elapsed());
    // 读取进程剩余的输出
    if (isOpen()) {
        m_log.append(readAll());
    }
    // 根据退出状态和码执行相应操作
    if (exitStatus == QProcess::NormalExit && exitCode == 0 && !m_killed) {
        // 任务成功完成
        if (m_postJobAction) {
            m_postJobAction->doAction(); // 执行后置操作
        }
        LOG_INFO() << "job succeeeded";
        m_log.append(QStringLiteral("Completed successfully in %1\n").arg(time.toString()));
        emit progressUpdated(m_item, 100);
        emit finished(this, true); // 发出成功信号
    } else if (m_killed) {
        // 任务被用户停止
        LOG_INFO() << "job stopped";
        m_log.append(QStringLiteral("Stopped by user at %1\n").arg(time.toString()));
        emit finished(this, false); // 发出失败信号
    } else {
        // 任务失败
        LOG_INFO() << "job failed with" << exitCode;
        m_log.append(QStringLiteral("Failed with exit code %1\n").arg(exitCode));
        emit finished(this, false); // 发出失败信号
    }
    m_isPaused = false;
}

/**
 * @brief 当进程有新的标准输出/错误输出时调用。
 */
void AbstractJob::onReadyRead()
{
    QString msg;
    // 循环读取所有可用行
    do {
        msg = readLine();
        appendToLog(msg);
    } while (!msg.isEmpty());
}

/**
 * @brief 当进程成功启动后调用。
 */
void AbstractJob::onStarted()
{
#ifdef Q_OS_WIN
    // 在 Windows 上，使用 API 设置进程优先级
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

/**
 * @brief 当进度更新时调用。
 * @param percent 当前的进度百分比。
 */
void AbstractJob::onProgressUpdated(QStandardItem *, int percent)
{
    // 在首次报告大于 0 的百分比时，启动估算计时器
    if (percent > 0 && (percent == 1 || m_startingPercent < 0)) {
        m_estimateTime.restart();
        m_startingPercent = percent;
    }
}
