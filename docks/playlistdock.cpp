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

#include "playlistdock.h"
#include "ui_playlistdock.h"
#include "dialogs/durationdialog.h"
#include "mainwindow.h"
#include <QtGui/QMenu>
#include <QDebug>

PlaylistDock::PlaylistDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistDock)
{
    ui->setupUi(this);
    toggleViewAction()->setIcon(QIcon::fromTheme("view-media-playlist", windowIcon()));
    ui->tableView->setModel(&m_model);
    QMenu* menu = new QMenu(this);
    menu->addAction(ui->actionAppendCut);
    menu->addAction(ui->actionAppendBlank);
    menu->addAction(ui->actionInsertCut);
    menu->addAction(ui->actionInsertBlank);
    ui->addButton->setMenu(menu);
    ui->addButton->setDefaultAction(ui->actionAppendCut);
    ui->tableView->setDragDropMode(QAbstractItemView::InternalMove);
    ui->tableView->setDropIndicatorShown(true);
    ui->tableView->setDragDropOverwriteMode(false);
    connect(ui->actionRemove, SIGNAL(triggered()), this, SLOT(on_removeButton_clicked()));
}

PlaylistDock::~PlaylistDock()
{
    delete ui;
}

void PlaylistDock::on_menuButton_clicked()
{
    QPoint pos = ui->menuButton->mapToParent(QPoint(0, 0));
    QMenu menu(this);
    menu.addAction(ui->actionGoto);
    menu.addAction(ui->actionOpen);
    menu.addAction(ui->actionRemove);
    menu.addAction(ui->actionRemoveAll);

    QModelIndex index = ui->tableView->currentIndex();
    if (index.isValid()) {
        Mlt::ClipInfo* info = m_model.playlist()->clip_info(index.row());
        if (info && info->resource && MLT.producer()->get("resource")
                && !strcmp(info->resource, MLT.producer()->get("resource"))) {
            menu.addAction(ui->actionUpdate);
        }
        delete info;
    }
    menu.addAction(ui->actionClose);
    menu.exec(mapToGlobal(pos));
}

void PlaylistDock::on_actionInsertCut_triggered()
{
    if (MLT.producer()) {
        QModelIndex index = ui->tableView->currentIndex();
        m_model.insert(MLT.producer(), index.row());
    }
}

void PlaylistDock::on_actionAppendCut_triggered()
{
    if (MLT.producer() && MLT.producer()->is_valid()) {
        if (MLT.producer()->type() == playlist_type)
            emit showStatusMessage(tr("You cannot insert a playlist into a playlist!"));
        else if (MLT.isSeekable())
            m_model.append(MLT.producer());
        else {
            DurationDialog dialog(this);
            dialog.setDuration(MLT.profile().fps() * 5);
            if (dialog.exec() == QDialog::Accepted) {
                MLT.producer()->set_in_and_out(0, dialog.duration());
                if (MLT.producer()->get("mlt_service") && !strcmp(MLT.producer()->get("mlt_service"), "avformat"))
                    MLT.producer()->set("mlt_service", "avformat-novalidate");
                m_model.append(MLT.producer());
            }
        }
    }
}

void PlaylistDock::on_actionInsertBlank_triggered()
{
    DurationDialog dialog(this);
    dialog.setDuration(MLT.profile().fps() * 5);
    if (dialog.exec() == QDialog::Accepted) {
        QModelIndex index = ui->tableView->currentIndex();
        if (index.isValid())
            m_model.insertBlank(dialog.duration(), index.row());
        else
            m_model.appendBlank(dialog.duration());
    }
}

void PlaylistDock::on_actionAppendBlank_triggered()
{
    DurationDialog dialog(this);
    dialog.setDuration(MLT.profile().fps() * 5);
    if (dialog.exec() == QDialog::Accepted)
        m_model.appendBlank(dialog.duration());
}

void PlaylistDock::on_actionUpdate_triggered()
{
    QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid()) return;
    Mlt::ClipInfo* info = m_model.playlist()->clip_info(index.row());
    if (!info) return;
    if (info->resource && MLT.producer()->get("resource")
            && !strcmp(info->resource, MLT.producer()->get("resource"))) {
        if (MLT.isSeekable()) {
            m_model.update(index.row(), MLT.producer()->get_in(), MLT.producer()->get_out());
        }
        else {
            // change the duration
            DurationDialog dialog(this);
            dialog.setDuration(info->frame_count - 1);
            if (dialog.exec() == QDialog::Accepted)
                m_model.update(index.row(), 0, dialog.duration());
        }
    }
    else {
        emit showStatusMessage(tr("This clip does not match the selected cut in the playlist!"));
    }
    delete info;
}

void PlaylistDock::on_removeButton_clicked()
{
    QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid()) return;
    m_model.remove(index.row());
    int count = m_model.playlist()->count();
    if (count == 0) {
        ui->removeButton->setEnabled(false);
        return;
    }
    Mlt::ClipInfo* i = m_model.playlist()->clip_info(
                index.row() >= count? count-1 : index.row());
    if (i) {
        emit itemActivated(i->start);
        delete i;
    }
}

void PlaylistDock::on_actionOpen_triggered()
{
    QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid()) return;
    Mlt::ClipInfo* i = m_model.playlist()->clip_info(index.row());
    if (i) {
        Mlt::Producer* p = new Mlt::Producer(i->producer);
        emit clipOpened(p, i->frame_in, i->frame_out);
        delete i;
    }
}

void PlaylistDock::on_tableView_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(ui->actionGoto);
    menu.addAction(ui->actionOpen);
    menu.addAction(ui->actionRemove);

    QModelIndex index = ui->tableView->currentIndex();
    if (index.isValid()) {
        Mlt::ClipInfo* info = m_model.playlist()->clip_info(index.row());
        if (info && info->resource && MLT.producer()->get("resource")
                && !strcmp(info->resource, MLT.producer()->get("resource"))) {
            menu.addAction(ui->actionUpdate);
        }
        delete info;
    }
    menu.exec(mapToGlobal(pos));
}

void PlaylistDock::on_tableView_doubleClicked(const QModelIndex &index)
{
    Mlt::ClipInfo* i = m_model.playlist()->clip_info(index.row());
    if (i) {
        emit itemActivated(i->start);
        delete i;
        ui->removeButton->setEnabled(true);
    }
}

void PlaylistDock::on_actionGoto_triggered()
{
    on_tableView_doubleClicked(ui->tableView->currentIndex());
}

void PlaylistDock::on_actionRemoveAll_triggered()
{
    m_model.clear();
}

void PlaylistDock::on_actionClose_triggered()
{
    MainWindow* main = qobject_cast<MainWindow*>(qApp->activeWindow());
    if (main && main->continueModified())
        m_model.close();
}
