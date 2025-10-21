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

/**
 * @brief 定义用于撤销/重做命令的唯一整数 ID。
 *
 * 这些 ID 被 QUndoCommand::id() 方法返回，用于在命令合并时区分不同类型的命令。
 * 只有 ID 相同的命令才有可能被合并。
 */
enum {
    UndoIdSubText = 400,  ///< 修改字幕文本命令的 ID
    UndoIdSubStart,       ///< 修改字幕开始时间命令的 ID
    UndoIdSubEnd,         ///< 修改字幕结束时间命令的 ID
    UndoIdSubMove,        ///< 移动字幕命令的 ID
};

/**
 * @class InsertTrackCommand
 * @brief 封装“插入字幕轨道”操作的撤销/重做命令。
 */
class InsertTrackCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的字幕模型。
     * @param track 要插入的轨道对象。
     * @param index 插入轨道的目标索引。
     */
    InsertTrackCommand(SubtitlesModel &model, const SubtitlesModel::SubtitleTrack &track, int index);

    void redo(); ///< 执行插入操作。
    void undo(); ///< 撤销插入操作。

private:
    SubtitlesModel &m_model;                    ///< 对字幕模型的引用。
    SubtitlesModel::SubtitleTrack m_track;      ///< 保存要插入的轨道数据。
    int m_index;                                ///< 保存插入的目标索引。
};

/**
 * @class RemoveTrackCommand
 * @brief 封装“删除字幕轨道”操作的撤销/重做命令。
 */
class RemoveTrackCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的字幕模型。
     * @param trackIndex 要删除的轨道的索引。
     */
    RemoveTrackCommand(SubtitlesModel &model, int trackIndex);

    void redo(); ///< 执行删除操作。
    void undo(); ///< 撤销删除操作。

private:
    SubtitlesModel &m_model;                        ///< 对字幕模型的引用。
    int m_trackIndex;                                ///< 被删除轨道的索引。
    SubtitlesModel::SubtitleTrack m_saveTrack;       ///< 保存被删除轨道的数据，用于撤销。
    QList<Subtitles::SubtitleItem> m_saveSubtitles;  ///< 保存被删除轨道上的所有字幕项，用于撤销。
};

/**
 * @class EditTrackCommand
 * @brief 封装“编辑字幕轨道属性”操作的撤销/重做命令。
 */
class EditTrackCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的字幕模型。
     * @param track 包含新属性的轨道对象。
     * @param index 要编辑的轨道的索引。
     */
    EditTrackCommand(SubtitlesModel &model, const SubtitlesModel::SubtitleTrack &track, int index);

    void redo(); ///< 执行编辑操作。
    void undo(); ///< 撤销编辑操作。

private:
    SubtitlesModel &m_model;                    ///< 对字幕模型的引用。
    SubtitlesModel::SubtitleTrack m_oldTrack;   ///< 保存轨道的旧属性，用于撤销。
    SubtitlesModel::SubtitleTrack m_newTrack;   ///< 保存轨道的新属性。
    int m_index;                                ///< 被编辑轨道的索引。
};

/**
 * @class OverwriteSubtitlesCommand
 * @brief 封装“覆盖式添加字幕”操作的撤销/重做命令。
 *
 * 此命令会移除与新字幕时间范围重叠的旧字幕，然后插入新字幕。
 */
class OverwriteSubtitlesCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的字幕模型。
     * @param trackIndex 目标轨道的索引。
     * @param items 要插入的新字幕项列表。
     */
    OverwriteSubtitlesCommand(SubtitlesModel &model,
                              int trackIndex,
                              const QList<Subtitles::SubtitleItem> &items);

    void redo(); ///< 执行覆盖操作。
    void undo(); ///< 撤销覆盖操作。

protected:
    QList<Subtitles::SubtitleItem> m_newSubtitles; ///< 保存要插入的新字幕项列表。

private:
    SubtitlesModel &m_model;                        ///< 对字幕模型的引用。
    int m_trackIndex;                                ///< 目标轨道的索引。
    QList<Subtitles::SubtitleItem> m_saveSubtitles;  ///< 保存被覆盖的旧字幕项列表，用于撤销。
};

/**
 * @class RemoveSubtitlesCommand
 * @brief 封装“删除字幕项”操作的撤销/重做命令。
 */
class RemoveSubtitlesCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的字幕模型。
     * @param trackIndex 目标轨道的索引。
     * @param items 要删除的字幕项列表。
     */
    RemoveSubtitlesCommand(SubtitlesModel &model,
                           int trackIndex,
                           const QList<Subtitles::SubtitleItem> &items);

    void redo(); ///< 执行删除操作。
    void undo(); ///< 撤销删除操作。

private:
    SubtitlesModel &m_model;                    ///< 对字幕模型的引用。
    int m_trackIndex;                            ///< 目标轨道的索引。
    QList<Subtitles::SubtitleItem> m_items;     ///< 保存要删除的字幕项列表，用于撤销。
};

/**
 * @class SetTextCommand
 * @brief 封装“修改字幕文本”操作的撤销/重做命令。
 */
class SetTextCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的字幕模型。
     * @param trackIndex 目标轨道的索引。
     * @param itemIndex 目标字幕项的索引。
     * @param text 新的文本内容。
     */
    SetTextCommand(SubtitlesModel &model, int trackIndex, int itemIndex, const QString &text);

    void redo(); ///< 执行修改操作。
    void undo(); ///< 撤销修改操作。

protected:
    int id() const { return UndoIdSubText; } ///< 返回命令的唯一 ID。
    bool mergeWith(const QUndoCommand *other); ///< 尝试与另一个命令合并。

private:
    SubtitlesModel &m_model;   ///< 对字幕模型的引用。
    int m_trackIndex;           ///< 目标轨道的索引。
    int m_itemIndex;            ///< 目标字幕项的索引。
    QString m_newText;          ///< 保存新的文本内容。
    QString m_oldText;          ///< 保存旧的文本内容，用于撤销。
};

/**
 * @class SetStartCommand
 * @brief 封装“修改字幕开始时间”操作的撤销/重做命令。
 */
class SetStartCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的字幕模型。
     * @param trackIndex 目标轨道的索引。
     * @param itemIndex 目标字幕项的索引。
     * @param msTime 新的开始时间（毫秒）。
     */
    SetStartCommand(SubtitlesModel &model, int trackIndex, int itemIndex, int64_t msTime);

    void redo(); ///< 执行修改操作。
    void undo(); ///< 撤销修改操作。

protected:
    int id() const { return UndoIdSubStart; } ///< 返回命令的唯一 ID。
    bool mergeWith(const QUndoCommand *other); ///< 尝试与另一个命令合并。

private:
    SubtitlesModel &m_model;   ///< 对字幕模型的引用。
    int m_trackIndex;           ///< 目标轨道的索引。
    int m_itemIndex;            ///< 目标字幕项的索引。
    int64_t m_newStart;         ///< 保存新的开始时间。
    int64_t m_oldStart;         ///< 保存旧的开始时间，用于撤销。
};

/**
 * @class SetEndCommand
 * @brief 封装“修改字幕结束时间”操作的撤销/重做命令。
 */
class SetEndCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的字幕模型。
     * @param trackIndex 目标轨道的索引。
     * @param itemIndex 目标字幕项的索引。
     * @param msTime 新的结束时间（毫秒）。
     */
    SetEndCommand(SubtitlesModel &model, int trackIndex, int itemIndex, int64_t msTime);

    void redo(); ///< 执行修改操作。
    void undo(); ///< 撤销修改操作。

protected:
    int id() const { return UndoIdSubEnd; } ///< 返回命令的唯一 ID。
    bool mergeWith(const QUndoCommand *other); ///< 尝试与另一个命令合并。

private:
    SubtitlesModel &m_model;   ///< 对字幕模型的引用。
    int m_trackIndex;           ///< 目标轨道的索引。
    int m_itemIndex;            ///< 目标字幕项的索引。
    int64_t m_newEnd;           ///< 保存新的结束时间。
    int64_t m_oldEnd;           ///< 保存旧的结束时间，用于撤销。
};

/**
 * @class MoveSubtitlesCommand
 * @brief 封装“移动字幕项”操作的撤销/重做命令。
 */
