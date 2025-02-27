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

#ifndef ABSTRACTJOB_H
#define ABSTRACTJOB_H

#include "postjobaction.h"
#include "settings.h"

#include <QElapsedTimer>
#include <QList>
#include <QModelIndex>
#include <QProcess>
#include <QThread>

class QAction;
class QStandardItem;

class AbstractJob : public QProcess
{
    Q_OBJECT
public:
    explicit AbstractJob(const QString &name, QThread::Priority priority = Settings.jobPriority());
    virtual ~AbstractJob() {}

    void setStandardItem(QStandardItem *item);
    QStandardItem *standardItem();
    bool ran() const;
    bool stopped() const;
    bool isFinished() const { return (ran() && state() != QProcess::Running); }
    void appendToLog(const QString &);
    QString log() const;
    QString label() const { return m_label; }
    void setLabel(const QString &label);
    QList<QAction *> standardActions() const { return m_standardActions; }
    QList<QAction *> successActions() const { return m_successActions; }
    QTime estimateRemaining(int percent);
    QElapsedTimer time() const { return m_totalTime; }
    void setPostJobAction(PostJobAction *action);
    bool paused() const;
    void setTarget(const QString &target) { m_target = target; }
    QString target() { return m_target; }

public slots:
    void start(const QString &program, const QStringList &arguments);
    virtual void start();
    virtual void stop();
    void pause();
    void resume();

signals:
    void progressUpdated(QStandardItem *item, int percent);
    void finished(AbstractJob *job, bool isSuccess, QString failureTime = QString());

protected:
    QList<QAction *> m_standardActions;
    QList<QAction *> m_successActions;
    QStandardItem *m_item;

protected slots:
    virtual void onFinished(int exitCode, QProcess::ExitStatus exitStatus = QProcess::NormalExit);
    virtual void onReadyRead();
    virtual void onStarted();

private slots:
    void onProgressUpdated(QStandardItem *, int percent);

private:
    bool m_ran;
    bool m_killed;
    QString m_log;
    QString m_label;
    QElapsedTimer m_estimateTime;
    int m_startingPercent;
    QElapsedTimer m_totalTime;
    QScopedPointer<PostJobAction> m_postJobAction;
    QThread::Priority m_priority;
    QAction *m_actionPause;
    QAction *m_actionResume;
    bool m_isPaused;
    QString m_target;
};

#endif // ABSTRACTJOB_H
