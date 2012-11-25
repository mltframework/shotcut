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

#ifndef MELTEDUNITDOCK_H
#define MELTEDUNITDOCK_H

#include <QDockWidget>
#include "meltedplaylistmodel.h"

namespace Ui {
class MeltedUnitDock;
}

class MeltedUnitDock : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit MeltedUnitDock(QWidget *parent = 0);
    ~MeltedUnitDock();
    QAbstractItemModel* playlistModel() const;
    
public slots:
    void onConnected(const QString& address, quint16 port = 5250, quint8 unit = 0);
    void onDisconnected();
    void onUnitChanged(quint8 unit);

private slots:
    void on_tableView_doubleClicked(const QModelIndex &index);

private:
    Ui::MeltedUnitDock *ui;
    MeltedPlaylistModel m_model;
};

#endif // MELTEDUNITDOCK_H
