/*
 * Copyright (c) 2015-2019 Meltytech, LLC
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

#ifndef _PLAYLISTTABLE_H
#define _PLAYLISTTABLE_H

#include <QTableView>

class PlaylistTable : public QTableView
{
    Q_OBJECT
public:
    PlaylistTable(QWidget *parent = nullptr);
    void keyPressEvent(QKeyEvent *) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

signals:
    void movedToEnd();
};

#endif
