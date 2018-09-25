/*
 * Copyright (c) 2013-2018 Meltytech, LLC
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

#ifndef PLAYLISTCOMMANDS_H
#define PLAYLISTCOMMANDS_H

#include "models/playlistmodel.h"
#include <QUndoCommand>
#include <QString>

namespace Playlist
{

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(PlaylistModel& model, const QString& xml, bool emitModified = true, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel& m_model;
    QString m_xml;
    bool m_emitModified;
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

#endif // PLAYLISTCOMMANDS_H
