/*
 * Copyright (c) 2024 Meltytech, LLC
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

#ifndef SUBTITLECOMMANDS_H
#define SUBTITLECOMMANDS_H

#include "models/subtitlesmodel.h"
#include <QUndoCommand>

namespace Subtitles {

enum {
    UndoIdSubText = 400,
    UndoIdSubStart,
    UndoIdSubEnd,
    UndoIdSubMove,
};

class InsertTrackCommand : public QUndoCommand
{
public:
    InsertTrackCommand(SubtitlesModel &model, const SubtitlesModel::SubtitleTrack &track, int index);
    void redo();
    void undo();
private:
    SubtitlesModel &m_model;
    SubtitlesModel::SubtitleTrack m_track;
    int m_index;
};

class RemoveTrackCommand : public QUndoCommand
{
public:
    RemoveTrackCommand(SubtitlesModel &model, int trackIndex);
    void redo();
    void undo();
private:
    SubtitlesModel &m_model;
    int m_trackIndex;
    SubtitlesModel::SubtitleTrack m_saveTrack;
    QList<Subtitles::SubtitleItem> m_saveSubtitles;
};

class OverwriteSubtitlesCommand : public QUndoCommand
{
public:
    OverwriteSubtitlesCommand(SubtitlesModel &model, int trackIndex,
                              const QList<Subtitles::SubtitleItem> &items);
    void redo();
    void undo();
protected:
    QList<Subtitles::SubtitleItem> m_newSubtitles;
private:
    SubtitlesModel &m_model;
    int m_trackIndex;
    QList<Subtitles::SubtitleItem> m_saveSubtitles;
};

class RemoveSubtitlesCommand : public QUndoCommand
{
public:
    RemoveSubtitlesCommand(SubtitlesModel &model, int trackIndex,
                           const QList<Subtitles::SubtitleItem> &items);
    void redo();
    void undo();
private:
    SubtitlesModel &m_model;
    int m_trackIndex;
    QList<Subtitles::SubtitleItem> m_items;
};

class SetTextCommand : public QUndoCommand
{
public:
    SetTextCommand(SubtitlesModel &model, int trackIndex, int itemIndex, const QString &text);
    void redo();
    void undo();
protected:
    int id() const
    {
        return UndoIdSubText;
    }
    bool mergeWith(const QUndoCommand *other);
private:
    SubtitlesModel &m_model;
    int m_trackIndex;
    int m_itemIndex;
    QString m_newText;
    QString m_oldText;
};

class SetStartCommand : public QUndoCommand
{
public:
    SetStartCommand(SubtitlesModel &model, int trackIndex, int itemIndex, int64_t msTime);
    void redo();
    void undo();
protected:
    int id() const
    {
        return UndoIdSubStart;
    }
    bool mergeWith(const QUndoCommand *other);
private:
    SubtitlesModel &m_model;
    int m_trackIndex;
    int m_itemIndex;
    int64_t m_newStart;
    int64_t m_oldStart;
};

class SetEndCommand : public QUndoCommand
{
public:
    SetEndCommand(SubtitlesModel &model, int trackIndex, int itemIndex, int64_t msTime);
    void redo();
    void undo();
protected:
    int id() const
    {
        return UndoIdSubEnd;
    }
    bool mergeWith(const QUndoCommand *other);
private:
    SubtitlesModel &m_model;
    int m_trackIndex;
    int m_itemIndex;
    int64_t m_newEnd;
    int64_t m_oldEnd;
};

class MoveSubtitlesCommand : public QUndoCommand
{
public:
    MoveSubtitlesCommand(SubtitlesModel &model, int trackIndex,
                         const QList<Subtitles::SubtitleItem> &items, int64_t msTime);
    void redo();
    void undo();
protected:
    int id() const
    {
        return UndoIdSubMove;
    }
    bool mergeWith(const QUndoCommand *other);
private:
    SubtitlesModel &m_model;
    int m_trackIndex;
    QList<Subtitles::SubtitleItem> m_oldSubtitles;
    QList<Subtitles::SubtitleItem> m_newSubtitles;
};

}

#endif // SUBTITLECOMMANDS_H
