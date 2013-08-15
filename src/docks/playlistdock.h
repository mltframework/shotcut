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
#include <QUndoCommand>
#include <QSettings>
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
    int position();

signals:
    void clipOpened(void* producer, int in, int out);
    void itemActivated(int start);
    void showStatusMessage(QString);

public slots:
    void incrementIndex();
    void decrementIndex();
    void setIndex(int row);
    void moveClipUp();
    void moveClipDown();
    void on_actionOpen_triggered();
    void on_actionInsertCut_triggered();
    void on_actionUpdate_triggered();
    void on_removeButton_clicked();

private slots:
    void on_menuButton_clicked();

    void on_actionAppendCut_triggered();

    void on_actionInsertBlank_triggered();

    void on_actionAppendBlank_triggered();

    void on_tableView_customContextMenuRequested(const QPoint &pos);

    void on_tableView_doubleClicked(const QModelIndex &index);

    void on_actionGoto_triggered();

    void on_actionRemoveAll_triggered();

    void on_actionClose_triggered();

    void onPlaylistCreated();

    void onPlaylistLoaded();

    void onPlaylistCleared();

    void onDropped(const QMimeData *data, int row);

    void onMoveClip(int from, int to);

    void onPlayerDragStarted();

    void on_addButton_clicked();

    void on_actionThumbnailsHidden_triggered(bool checked);

    void on_actionLeftAndRight_triggered(bool checked);

    void on_actionTopAndBottom_triggered(bool checked);

    void on_actionInOnlySmall_triggered(bool checked);

    void on_actionInOnlyLarge_triggered(bool checked);

private:
    Ui::PlaylistDock *ui;
    PlaylistModel m_model;
    int m_defaultRowHeight;
    QSettings m_settings;
};

namespace Playlist
{

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(PlaylistModel& model, const QString& xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel& m_model;
    QString m_xml;
};

class InsertCommand : public QUndoCommand
{
public:
    InsertCommand(PlaylistModel& model, const QString& xml, int row, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel& m_model;
    QString m_xml;
    int m_row;
};

class UpdateCommand : public QUndoCommand
{
public:
    UpdateCommand(PlaylistModel& model, const QString& xml, int row, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel& m_model;
    QString m_newXml;
    QString m_oldXml;
    int m_row;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(PlaylistModel& model, int row, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel& m_model;
    QString m_xml;
    int m_row;
};

class MoveCommand : public QUndoCommand
{
public:
    MoveCommand(PlaylistModel& model, int from, int to, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel& m_model;
    int m_from;
    int m_to;
};

class ClearCommand : public QUndoCommand
{
public:
    ClearCommand(PlaylistModel& model, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel& m_model;
    QString m_xml;
};

}

#endif // PLAYLISTDOCK_H
