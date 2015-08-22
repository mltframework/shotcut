/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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
#include "undohelper.h"
#include <QUndoCommand>
#include <QString>

namespace Timeline
{

enum {
    UndoIdTrimClipIn,
    UndoIdTrimClipOut,
    UndoIdFadeIn,
    UndoIdFadeOut,
    UndoIdTrimTransitionIn,
    UndoIdTrimTransitionOut,
    UndoIdAddTransitionByTrimIn,
    UndoIdAddTransitionByTrimOut
};

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(MultitrackModel& model, int trackIndex, const QString& xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    QString m_xml;
    UndoHelper m_undoHelper;
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
    int m_position;
    QString m_xml;
    QStringList m_oldTracks;
    UndoHelper m_undoHelper;
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
    UndoHelper m_undoHelper;
};

class LiftCommand : public QUndoCommand
{
public:
    LiftCommand(MultitrackModel& model, int trackIndex, int clipIndex, const QString &xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    QString m_xml;
    UndoHelper m_undoHelper;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(MultitrackModel& model, int trackIndex, int clipIndex, const QString &xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    QString m_xml;
    UndoHelper m_undoHelper;
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

class LockTrackCommand : public QUndoCommand
{
public:
    LockTrackCommand(MultitrackModel& model, int trackIndex, bool value, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    bool m_value;
    bool m_oldValue;
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
    int m_fromStart;
    int m_toStart;
    UndoHelper m_undoHelper;
};

class TrimClipInCommand : public QUndoCommand
{
public:
    TrimClipInCommand(MultitrackModel& model, int trackIndex, int clipIndex, int delta, bool ripple, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const { return UndoIdTrimClipIn; }
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;
    bool m_ripple;
    UndoHelper m_undoHelper;
};

class TrimClipOutCommand : public QUndoCommand
{
public:
    TrimClipOutCommand(MultitrackModel& model, int trackIndex, int clipIndex, int delta, bool ripple, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const { return UndoIdTrimClipOut; }
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;
    bool m_ripple;
    UndoHelper m_undoHelper;
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
    int id() const { return UndoIdFadeIn; }
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
    int id() const { return UndoIdFadeOut; }
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_duration;
    int m_previous;
};

class AddTransitionCommand : public QUndoCommand
{
public:
    AddTransitionCommand(MultitrackModel& model, int trackIndex, int clipIndex, int position, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_position;
    int m_transitionIndex;
};

class TrimTransitionInCommand : public QUndoCommand
{
public:
    TrimTransitionInCommand(MultitrackModel& model, int trackIndex, int clipIndex, int delta, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const { return UndoIdTrimTransitionIn; }
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;
    bool m_notify;
};

class TrimTransitionOutCommand : public QUndoCommand
{
public:
    TrimTransitionOutCommand(MultitrackModel& model, int trackIndex, int clipIndex, int delta, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const { return UndoIdTrimTransitionOut; }
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;
    bool m_notify;
};

class AddTransitionByTrimInCommand : public QUndoCommand
{
public:
    AddTransitionByTrimInCommand(MultitrackModel& model, int trackIndex, int clipIndex, int delta, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const { return UndoIdAddTransitionByTrimIn; }
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;
    bool m_notify;
};

class AddTransitionByTrimOutCommand : public QUndoCommand
{
public:
    AddTransitionByTrimOutCommand(MultitrackModel& model, int trackIndex, int clipIndex, int delta, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const { return UndoIdAddTransitionByTrimOut; }
    bool mergeWith(const QUndoCommand *other);
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;
    bool m_notify;
};

class AddTrackCommand: public QUndoCommand
{
public:
    AddTrackCommand(MultitrackModel& model, bool isVideo, QUndoCommand* parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    bool m_isVideo;
};

class InsertTrackCommand : public QUndoCommand
{
public:
    InsertTrackCommand(MultitrackModel& model, int trackIndex, QUndoCommand* parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    TrackType m_trackType;
};

class RemoveTrackCommand : public QUndoCommand
{
public:
    RemoveTrackCommand(MultitrackModel& model, int trackIndex, QUndoCommand* parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    QString m_xml;
    TrackType m_trackType;
    QString m_trackName;
};

} // namespace

#endif
