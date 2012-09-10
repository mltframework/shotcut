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

#ifndef PLAYLISTDOCK_H
#define PLAYLISTDOCK_H

#include <QDockWidget>
#include "models/playlistmodel.h"

namespace Ui {
    class PlaylistDock;
}

class PlaylistDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit PlaylistDock(QWidget *parent = 0);
    ~PlaylistDock();
    PlaylistModel* model() {
        return &m_model;
    }

signals:
    void clipOpened(void* producer, int in, int out);
    void itemActivated(int start);
    void showStatusMessage(QString);

private slots:
    void on_menuButton_clicked();

    void on_actionInsertCut_triggered();

    void on_actionAppendCut_triggered();

    void on_actionInsertBlank_triggered();

    void on_actionAppendBlank_triggered();

    void on_actionUpdate_triggered();

    void on_removeButton_clicked();

    void on_actionOpen_triggered();

    void on_tableView_customContextMenuRequested(const QPoint &pos);

    void on_tableView_doubleClicked(const QModelIndex &index);

    void on_actionGoto_triggered();

    void on_actionRemoveAll_triggered();

    void on_actionClose_triggered();

    void onPlaylistCreated();

    void onPlaylistCleared();

private:
    Ui::PlaylistDock *ui;
    PlaylistModel m_model;
};

#endif // PLAYLISTDOCK_H
