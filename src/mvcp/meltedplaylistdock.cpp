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

#include "meltedplaylistdock.h"
#include "ui_meltedplaylistdock.h"

MeltedPlaylistDock::MeltedPlaylistDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::MeltedPlaylistDock)
{
    ui->setupUi(this);
    ui->tableView->setModel(&m_model);
    connect(&m_model, SIGNAL(loaded()), ui->tableView, SLOT(resizeColumnsToContents()));
}

MeltedPlaylistDock::~MeltedPlaylistDock()
{
    delete ui;
}

QAbstractItemModel *MeltedPlaylistDock::playlistModel() const
{
    return (QAbstractItemModel*) &m_model;
}

void MeltedPlaylistDock::onConnected(const QString &address, quint16 port, quint8 unit)
{
    m_model.onConnected(address, port, unit);
}

void MeltedPlaylistDock::onDisconnected()
{
    m_model.onDisconnected();
}

void MeltedPlaylistDock::onUnitChanged(quint8 unit)
{
    m_model.onUnitChanged(unit);
}

void MeltedPlaylistDock::on_tableView_doubleClicked(const QModelIndex &index)
{
    m_model.gotoClip(index.row());
}
