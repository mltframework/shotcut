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

#include "jobsdock.h"
#include "ui_jobsdock.h"
#include "jobqueue.h"
#include <QtWidgets>
#include "dialogs/textviewerdialog.h"
#include "mainwindow.h"

JobsDock::JobsDock(QWidget *parent) :
    Panel(tr("Jobs"), parent),
    ui(new Ui::JobsDock)
{
    ui->setupUi(this);
    setWidget(ui->dockWidgetContents);
    ui->treeView->setModel(&JOBS);
    QHeaderView* header = ui->treeView->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->cleanButton->hide();
}

JobsDock::~JobsDock()
{
    JOBS.cleanup();
    delete ui;
}

void JobsDock::on_treeView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    QMenu menu(this);
    MeltJob* job = JOBS.jobFromIndex(index);
    if (job) {
        if (job->ran() && job->state() == QProcess::NotRunning && job->exitStatus() == QProcess::NormalExit) {
            menu.addAction(ui->actionOpen);
            menu.addAction(ui->actionOpenFolder);
        }
        if (job->stopped() || (JOBS.isPaused() && !job->ran()))
            menu.addAction(ui->actionRun);
        if (job->state() == QProcess::Running)
            menu.addAction(ui->actionStopJob);
        if (job->ran())
            menu.addAction(ui->actionViewLog);
        menu.addAction(ui->actionViewXml);
    }
    menu.exec(mapToGlobal(pos));
}

void JobsDock::on_actionStopJob_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    MeltJob* job = JOBS.jobFromIndex(index);
    if (job) job->stop();
}

void JobsDock::on_actionViewLog_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    MeltJob* job = JOBS.jobFromIndex(index);
    if (job) {
        TextViewerDialog dialog(this);
        dialog.setWindowTitle(tr("Job Log"));
        dialog.setText(job->log());
        dialog.exec();
    }
}

void JobsDock::on_actionViewXml_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    MeltJob* job = JOBS.jobFromIndex(index);
    if (job) {
        TextViewerDialog dialog(this);
        dialog.setWindowTitle(tr("MLT XML"));
        dialog.setText(job->xml());
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

void JobsDock::on_actionOpen_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    MeltJob* job = JOBS.jobFromIndex(index);
    if (job)
        MAIN.open(job->objectName().toUtf8().constData());
}

void JobsDock::on_actionRun_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    MeltJob* job = JOBS.jobFromIndex(index);
    if (job) job->start();
}

void JobsDock::on_menuButton_clicked()
{
    on_treeView_customContextMenuRequested(ui->menuButton->mapToParent(QPoint(0, 0)));
}

void JobsDock::on_actionOpenFolder_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;
    MeltJob* job = JOBS.jobFromIndex(index);
    if (job) {
        QFileInfo fi(job->objectName());
        QUrl url(QString("file://").append(fi.path()), QUrl::TolerantMode);
        QDesktopServices::openUrl(url);
    }
}
