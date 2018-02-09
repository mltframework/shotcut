/*
 * Copyright (c) 2012-2018 Meltytech, LLC
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
#include <QUndoCommand>
#include "models/playlistmodel.h"

namespace Ui {
    class PlaylistDock;
}

class QAbstractItemView;

class PlaylistDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit PlaylistDock(QWidget *parent = 0);
    ~PlaylistDock();
    PlaylistModel* model() {
        return &m_model;
    }
    int position();

signals:
    void clipOpened(Mlt::Producer* producer);
    void itemActivated(int start);
    void showStatusMessage(QString);
    void addAllTimeline(Mlt::Playlist*);

public slots:
    void incrementIndex();
    void decrementIndex();
    void setIndex(int row);
    void moveClipUp();
    void moveClipDown();
    void on_actionOpen_triggered();
    void on_actionInsertCut_triggered();
    void on_actionAppendCut_triggered();
    void on_actionUpdate_triggered();
    void on_removeButton_clicked();
    void setUpdateButtonEnabled(bool modified);
    void onProducerOpened();

private slots:
    void on_menuButton_clicked();

    void on_actionInsertBlank_triggered();

    void on_actionAppendBlank_triggered();

    void viewCustomContextMenuRequested(const QPoint &pos);

    void viewDoubleClicked(const QModelIndex &index);

    void on_actionGoto_triggered();

    void on_actionRemoveAll_triggered();

    void onPlaylistCreated();

    void onPlaylistLoaded();

    void onPlaylistCleared();

    void onPlaylistClosed();

    void onDropped(const QMimeData *data, int row);

    void onMoveClip(int from, int to);

    void onPlayerDragStarted();

    void on_addButton_clicked();

    void on_actionThumbnailsHidden_triggered(bool checked);

    void on_actionLeftAndRight_triggered(bool checked);

    void on_actionTopAndBottom_triggered(bool checked);

    void on_actionInOnlySmall_triggered(bool checked);

    void on_actionInOnlyLarge_triggered(bool checked);

    void on_actionAddToTimeline_triggered();

    void on_updateButton_clicked();

    void updateViewModeFromActions();

    void on_tilesButton_clicked();

    void on_iconsButton_clicked();

    void on_detailsButton_clicked();

protected:
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

private:
    void setViewMode(PlaylistModel::ViewMode mode);

    Ui::PlaylistDock *ui;
    QAbstractItemView *m_view;
    QAbstractItemView *m_iconsView;
    PlaylistModel m_model;
    int m_defaultRowHeight;
};

#endif // PLAYLISTDOCK_H
