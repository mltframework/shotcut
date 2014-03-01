/*
 * Copyright (c) 2013-2014 Meltytech, LLC
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

#ifndef COMMANDS_H
#define COMMANDS_H

#include "models/multitrackmodel.h"
#include <QUndoCommand>
#include <QString>

namespace Timeline
{

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(MultitrackModel& model, int trackIndex, const QString& xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    QString m_xml;
};

class InsertCommand : public QUndoCommand
{
public:
    InsertCommand(MultitrackModel& model, int trackIndex, int position, const QString &xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_position;
    QString m_xml;
};

class OverwriteCommand : public QUndoCommand
{
public:
    OverwriteCommand(MultitrackModel& model, int trackIndex, int position, const QString &xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    QString m_playlistXml;
    int m_position;
    QString m_xml;
};

class LiftCommand : public QUndoCommand
{
public:
    LiftCommand(MultitrackModel& model, int trackIndex, int clipIndex, int position, const QString &xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_position;
    QString m_xml;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(MultitrackModel& model, int trackIndex, int clipIndex, int position, const QString &xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_position;
    QString m_xml;
};

class NameTrackCommand : public QUndoCommand
{
public:
    NameTrackCommand(MultitrackModel& model, int trackIndex, const QString& name, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    QString m_name;
    QString m_oldName;
};

class MuteTrackCommand : public QUndoCommand
{
public:
    MuteTrackCommand(MultitrackModel& model, int trackIndex, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    bool m_oldValue;
};

class HideTrackCommand : public QUndoCommand
{
public:
    HideTrackCommand(MultitrackModel& model, int trackIndex, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    bool m_oldValue;
};

class CompositeTrackCommand : public QUndoCommand
{
public:
    CompositeTrackCommand(MultitrackModel& model, int trackIndex, Qt::CheckState value, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    Qt::CheckState m_value;
    Qt::CheckState m_oldValue;
};

class MoveClipCommand : public QUndoCommand
{
public:
    MoveClipCommand(MultitrackModel& model, int fromTrackIndex, int toTrackIndex, int clipIndex, int position, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_fromTrackIndex;
    int m_toTrackIndex;
    int m_fromClipIndex;
    int m_toClipIndex;
    int m_fromStart;
    int m_toStart;
};

class TrimClipInCommand : public QUndoCommand
{
public:
    TrimClipInCommand(MultitrackModel& model, int trackIndex, int clipIndex, int delta, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const;
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;
    bool m_notify;
};

class TrimClipOutCommand : public QUndoCommand
{
public:
    TrimClipOutCommand(MultitrackModel& model, int trackIndex, int clipIndex, int delta, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const;
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;
    bool m_notify;
};

class SplitCommand : public QUndoCommand
{
public:
    SplitCommand(MultitrackModel& model, int trackIndex, int clipIndex, int position, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_position;
};

class FadeInCommand : public QUndoCommand
{
public:
    FadeInCommand(MultitrackModel& model, int trackIndex, int clipIndex, int duration, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const;
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_duration;
    int m_previous;
};

class FadeOutCommand : public QUndoCommand
{
public:
    FadeOutCommand(MultitrackModel& model, int trackIndex, int clipIndex, int duration, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const;
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_duration;
    int m_previous;
};

}

#endif
