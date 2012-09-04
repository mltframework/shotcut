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
#include <QtGui/QMenu>
#include <QDebug>

PlaylistDock::PlaylistDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistDock)
{
    ui->setupUi(this);
    ui->tableView->setModel(&m_model);
    QMenu* menu = new QMenu(this);
    menu->addAction(ui->actionAppendCut);
    menu->addAction(ui->actionAppendBlank);
    menu->addAction(ui->actionInsertCut);
    menu->addAction(ui->actionInsertBlank);
    menu->addAction(ui->actionUpdate);
    ui->addButton->setMenu(menu);
    ui->addButton->setDefaultAction(ui->actionAppendCut);
}

PlaylistDock::~PlaylistDock()
{
    delete ui;
}

void PlaylistDock::on_menuButton_clicked()
{

}

void PlaylistDock::on_actionInsertCut_triggered()
{

}

void PlaylistDock::on_actionAppendCut_triggered()
{
    if (MLT.producer())
        m_model.append(MLT.producer());
}

void PlaylistDock::on_actionInsertBlank_triggered()
{

}

void PlaylistDock::on_actionAppendBlank_triggered()
{

}

void PlaylistDock::on_actionUpdate_triggered()
{

}

void PlaylistDock::on_removeButton_clicked()
{

}

void PlaylistDock::on_tableView_activated(const QModelIndex &index)
{
    Mlt::ClipInfo* i = m_model.playlist().clip_info(index.row());
    if (i)
        emit itemActivated(i->producer, i->frame_in, i->frame_out);
    delete i;
}
