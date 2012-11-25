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

#include "meltedunitdock.h"
#include "ui_meltedunitdock.h"

MeltedUnitDock::MeltedUnitDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::MeltedUnitDock)
{
    ui->setupUi(this);
    ui->tableView->setModel(&m_model);
    connect(&m_model, SIGNAL(loaded()), ui->tableView, SLOT(resizeColumnsToContents()));
}

MeltedUnitDock::~MeltedUnitDock()
{
    delete ui;
}

QAbstractItemModel *MeltedUnitDock::playlistModel() const
{
    return (QAbstractItemModel*) &m_model;
}

void MeltedUnitDock::onConnected(const QString &address, quint16 port, quint8 unit)
{
    m_model.onConnected(address, port, unit);
}

void MeltedUnitDock::onDisconnected()
{
    m_model.onDisconnected();
}

void MeltedUnitDock::onUnitChanged(quint8 unit)
{
    m_model.onUnitChanged(unit);
}
