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

#include "jobsdock.h"
#include "ui_jobsdock.h"
#include "jobqueue.h"
#include <QtWidgets>
#include <Logger.h>
#include "dialogs/textviewerdialog.h"

JobsDock::JobsDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::JobsDock)
{
    LOG_DEBUG() << "begin";
    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());
    ui->treeView->setModel(&JOBS);
    QHeaderView* header = ui->treeView->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->cleanButton->hide();
    LOG_DEBUG() << "end";
}

JobsDock::~JobsDock()
{
    JOBS.cleanup();
    delete ui;
}

AbstractJob *JobsDock::currentJob() const
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return 0;
    return JOBS.jobFromIndex(index);
}

void JobsDock::on_treeView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    QMenu menu(this);
    AbstractJob* job = JOBS.jobFromIndex(index);
    if (job) {
        if (job->ran() && job->state() == QProcess::NotRunning && job->exitStatus() == QProcess::NormalExit) {
            menu.addActions(job->successActions());
        }
        if (job->stopped() || (JOBS.isPaused() && !job->ran()))
            menu.addAction(ui->actionRun);
        if (job->state() == QProcess::Running)
            menu.addAction(ui->actionStopJob);
        else
            menu.addAction(ui->actionRemove);
        if (job->ran())
            menu.addAction(ui->actionViewLog);
        menu.addActions(job->standardActions());
    }
    menu.exec(mapToGlobal(pos));
}

void JobsDock::on_actionStopJob_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    AbstractJob* job = JOBS.jobFromIndex(index);
    if (job) job->stop();
}

void JobsDock::on_actionViewLog_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    AbstractJob* job = JOBS.jobFromIndex(index);
    if (job) {
        TextViewerDialog dialog(this);
        dialog.setWindowTitle(tr("Job Log"));
        dialog.setText(job->log());
        dialog.exec();
    }
}

void JobsDock::on_pauseButton_toggled(bool checked)
{
    if (checked)
        JOBS.pause();
    else
        JOBS.resume();
}

void JobsDock::on_actionRun_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    AbstractJob* job = JOBS.jobFromIndex(index);
    if (job) job->start();
}

void JobsDock::on_menuButton_clicked()
{
    on_treeView_customContextMenuRequested(ui->menuButton->mapToParent(QPoint(0, 0)));
}

void JobsDock::on_treeView_doubleClicked(const QModelIndex &index)
{
    AbstractJob* job = JOBS.jobFromIndex(index);
    if (job && job->ran() && job->state() == QProcess::NotRunning && job->exitStatus() == QProcess::NormalExit) {
        foreach (QAction* action, job->successActions()) {
            if (action->text() == "Open") {
                action->trigger();
                break;
            }
        }
    }
}

void JobsDock::on_actionRemove_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    JOBS.remove(index);
}
