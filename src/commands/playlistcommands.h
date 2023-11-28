/*
 * Copyright (c) 2013-2023 Meltytech, LLC
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
#include <QUuid>

namespace Playlist {

enum {
    UndoIdTrimClipIn = 0,
    UndoIdTrimClipOut,
    UndoIdUpdate
};

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(PlaylistModel &model, const QString &xml, bool emitModified = true,
                  QUndoCommand *parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel &m_model;
    QString m_xml;
    bool m_emitModified;
    QUuid m_uuid;
};

class InsertCommand : public QUndoCommand
{
public:
    InsertCommand(PlaylistModel &model, const QString &xml, int row, QUndoCommand *parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel &m_model;
    QString m_xml;
    int m_row;
    QUuid m_uuid;
};

class UpdateCommand : public QUndoCommand
{
public:
    UpdateCommand(PlaylistModel &model, const QString &xml, int row, QUndoCommand *parent = 0);
    void redo();
    void undo();
protected:
    int id() const
    {
        return UndoIdUpdate;
    }
    bool mergeWith(const QUndoCommand *other);
private:
    PlaylistModel &m_model;
    QString m_newXml;
    QString m_oldXml;
    int m_row;
    QUuid m_uuid;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(PlaylistModel &model, int row, QUndoCommand *parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel &m_model;
    QString m_xml;
    int m_row;
    QUuid m_uuid;
};

class MoveCommand : public QUndoCommand
{
public:
    MoveCommand(PlaylistModel &model, int from, int to, QUndoCommand *parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel &m_model;
    int m_from;
    int m_to;
};

class ClearCommand : public QUndoCommand
{
public:
    ClearCommand(PlaylistModel &model, QUndoCommand *parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel &m_model;
    QString m_xml;
    QVector<QUuid> m_uuids;
};

class SortCommand : public QUndoCommand
{
public:
    SortCommand(PlaylistModel &model, int column, Qt::SortOrder order, QUndoCommand *parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel &m_model;
    int m_column;
    Qt::SortOrder m_order;
    QString m_xml;
    QVector<QUuid> m_uuids;
};

class TrimClipInCommand : public QUndoCommand
{
public:
    TrimClipInCommand(PlaylistModel &model, int row, int in, QUndoCommand *parent = nullptr);
    void redo();
    void undo();
protected:
    int id() const
    {
        return UndoIdTrimClipIn;
    }
    bool mergeWith(const QUndoCommand *other);
private:
    PlaylistModel &m_model;
    int m_row;
    int m_oldIn;
    int m_newIn;
    int m_out;
};

class TrimClipOutCommand : public QUndoCommand
{
public:
    TrimClipOutCommand(PlaylistModel &model, int row, int out, QUndoCommand *parent = nullptr);
    void redo();
    void undo();
protected:
    int id() const
    {
        return UndoIdTrimClipOut;
    }
    bool mergeWith(const QUndoCommand *other);
private:
    PlaylistModel &m_model;
    int m_row;
    int m_in;
    int m_oldOut;
    int m_newOut;
};

class ReplaceCommand : public QUndoCommand
{
public:
    ReplaceCommand(PlaylistModel &model, const QString &xml, int row, QUndoCommand *parent = 0);
    void redo();
    void undo();
private:
    PlaylistModel &m_model;
    QString m_newXml;
    QString m_oldXml;
    int m_row;
    QUuid m_uuid;
};

}

#endif // PLAYLISTCOMMANDS_H
