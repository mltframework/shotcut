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
#include <QUndoCommand>
#include "meltedplaylistmodel.h"
#include "transportcontrol.h"

namespace Ui {
class MeltedPlaylistDock;
}

namespace MeltedPlaylist {
class TransportControl;
}

class MeltedPlaylistDock : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit MeltedPlaylistDock(QWidget *parent = 0);
    ~MeltedPlaylistDock();
    QAbstractItemModel* model() const;
    const TransportControllable* transportControl() const;

signals:
    void appendRequested();
    void insertRequested(int row);

public slots:
    void onConnected(const QString& address, quint16 port = 5250, quint8 unit = 0);
    void onDisconnected();
    void onUnitChanged(quint8 unit);

protected:
    void keyPressEvent(QKeyEvent *);

private slots:
    void on_tableView_clicked(const QModelIndex &index);

    void on_tableView_doubleClicked(const QModelIndex &index);

    void onDropped(QString clip, int row);

    void onAppend(QString clip, int in = -1, int out = -1);

    void onInsert(QString clip, int row, int in = -1, int out = -1);

    void onMoveClip(int from, int to);

    void onSuccess();

    void on_tableView_customContextMenuRequested(const QPoint &pos);
    
    void on_addButton_clicked();
    
    void on_removeButton_clicked();
    
    void on_menuButton_clicked();
    
    void on_actionInsertCut_triggered();
    
    void on_actionOpen_triggered();
    
    void on_actionGoto_triggered();
    
    void on_actionRemoveAll_triggered();
    
    void on_actionWipe_triggered();
    
    void on_actionClean_triggered();

private:
    Ui::MeltedPlaylistDock *ui;
    MeltedPlaylistModel m_model;
    QList<QUndoCommand*> m_operations;
    MeltedPlaylist::TransportControl* m_transportControl;
};

namespace MeltedPlaylist
{

class TransportControl : public TransportControllable
{
    Q_OBJECT
public:
    explicit TransportControl(MeltedPlaylistModel& model);

public slots:
    void play(double speed = 1.0);
    void pause();
    void stop();
    void seek(int position);
    void rewind();
    void fastForward();
    void previous(int currentPosition);
    void next(int currentPosition);
    void setIn(int in);
    void setOut(int out);

private:
    MeltedPlaylistModel& m_model;
};

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(MeltedPlaylistModel& model, const QString& clip, int in = -1, int out = -1, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MeltedPlaylistModel& m_model;
    QString m_clip;
    int m_in;
    int m_out;
    bool m_skip;
};

class InsertCommand : public QUndoCommand
{
public:
    InsertCommand(MeltedPlaylistModel& model, const QString& clip, int row, int in = -1, int out = -1, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MeltedPlaylistModel& m_model;
    QString m_clip;
    int m_row;
    int m_in;
    int m_out;
    bool m_skip;
};

class SetInCommand : public QUndoCommand
{
public:
    SetInCommand(MeltedPlaylistModel& model, const QString& clip, int row, int in, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MeltedPlaylistModel& m_model;
    QString m_clip;
    int m_row;
    int m_old;
    int m_new;
};

class SetOutCommand : public QUndoCommand
{
public:
    SetOutCommand(MeltedPlaylistModel& model, const QString& clip, int row, int out, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MeltedPlaylistModel& m_model;
    QString m_clip;
    int m_row;
    int m_old;
    int m_new;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(MeltedPlaylistModel& model, QString& clip, int row, int in = -1, int out = -1, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MeltedPlaylistModel& m_model;
    QString m_clip;
    int m_row;
    int m_in;
    int m_out;
    bool m_skip;
};

class MoveCommand : public QUndoCommand
{
public:
    MoveCommand(MeltedPlaylistModel& model, QString& clip, int from, int to, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MeltedPlaylistModel& m_model;
    int m_from;
    int m_to;
    bool m_skip;
};

} // namespace MeltedPlaylist

#endif // MELTEDPLAYLISTDOCK_H
