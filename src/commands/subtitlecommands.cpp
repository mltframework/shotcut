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

#include "subtitlecommands.h"

#include "Logger.h"
#include "mainwindow.h"

#include <QFileInfo>

namespace Subtitles {

/**
 * @class InsertTrackCommand
 * @brief 封装“插入字幕轨道”操作的撤销/重做命令。
 */
InsertTrackCommand::InsertTrackCommand(SubtitlesModel &model,
                                       const SubtitlesModel::SubtitleTrack &track,
                                       int index)
    : QUndoCommand(0)
    , m_model(model)
    , m_track(track) // 保存要插入的轨道对象
    , m_index(index)  // 保存插入的位置
{
    setText(QObject::tr("Add subtitle track: %1").arg(m_track.name));
}

void InsertTrackCommand::redo()
{
    LOG_DEBUG() << m_track.name;
    m_model.doInsertTrack(m_track, m_index);
}

void InsertTrackCommand::undo()
{
    // 撤销插入，即移除该轨道
    m_model.doRemoveTrack(m_index);
}

/**
 * @class RemoveTrackCommand
 * @brief 封装“删除字幕轨道”操作的撤销/重做命令。
 */
RemoveTrackCommand::RemoveTrackCommand(SubtitlesModel &model, int trackIndex)
    : QUndoCommand(0)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_saveTrack(m_model.getTrack(trackIndex)) // 保存被删除的轨道信息
{
    setText(QObject::tr("Remove subtitle track: %1").arg(m_saveTrack.name));
    // 保存该轨道上所有的字幕项，以便撤销时恢复
    int count = m_model.itemCount(m_trackIndex);
    m_saveSubtitles.reserve(count);
    for (int i = 0; i < count; i++) {
        m_saveSubtitles.push_back(m_model.getItem(m_trackIndex, i));
    }
}

void RemoveTrackCommand::redo()
{
    m_model.doRemoveTrack(m_trackIndex);
}

void RemoveTrackCommand::undo()
{
    // 撤销删除：先恢复轨道，再恢复轨道上的所有字幕项
    m_model.doInsertTrack(m_saveTrack, m_trackIndex);
    m_model.doInsertSubtitleItems(m_trackIndex, m_saveSubtitles);
}

/**
 * @class EditTrackCommand
 * @brief 封装“编辑字幕轨道属性”操作的撤销/重做命令。
 */
EditTrackCommand::EditTrackCommand(SubtitlesModel &model,
                                   const SubtitlesModel::SubtitleTrack &track,
                                   int index)
    : QUndoCommand(0)
    , m_model(model)
    , m_newTrack(track) // 保存轨道的新属性
    , m_index(index)
{
    // 获取并保存轨道的旧属性
    m_oldTrack = m_model.getTrack(index);
    setText(QObject::tr("Edit subtitle track: %1").arg(m_newTrack.name));
}

void EditTrackCommand::redo()
{
    LOG_DEBUG() << m_oldTrack.name;
    m_model.doEditTrack(m_newTrack, m_index);
}

void EditTrackCommand::undo()
{
    // 恢复到旧属性
    m_model.doEditTrack(m_oldTrack, m_index);
}

/**
 * @class OverwriteSubtitlesCommand
 * @brief 封装“覆盖式添加字幕”操作的撤销/重做命令。
 *
 * 此命令会移除与新字幕时间范围重叠的旧字幕，然后插入新字幕。
 * 常用于从文件导入字幕或粘贴字幕。
 */
OverwriteSubtitlesCommand::OverwriteSubtitlesCommand(SubtitlesModel &model,
                                                     int trackIndex,
                                                     const QList<Subtitles::SubtitleItem> &items)
    : QUndoCommand(0)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_newSubtitles(items) // 保存要插入的新字幕列表
{
    // 根据字幕数量设置显示文本
    if (m_newSubtitles.size() == 1) {
        setText(QObject::tr("Add subtitle"));
    } else {
        setText(QObject::tr("Add %n subtitles", nullptr, m_newSubtitles.size()));
    }

    if (m_newSubtitles.size() <= 0) {
        return;
    }
    // 计算新字幕的时间范围
    int64_t startPosition = m_newSubtitles[0].start;
    int64_t endPosition = m_newSubtitles[m_newSubtitles.size() - 1].end;
    // 保存所有与新字幕时间范围重叠的旧字幕，以便撤销时恢复
    int count = m_model.itemCount(m_trackIndex);
    for (int i = 0; i < count; i++) {
        auto item = m_model.getItem(m_trackIndex, i);
        if ((item.start >= startPosition && item.start < endPosition)
            || (item.end > startPosition && item.end < endPosition)) {
            m_saveSubtitles.push_back(item);
        }
    }
}

void OverwriteSubtitlesCommand::redo()
{
    LOG_DEBUG() << m_newSubtitles.size();

    if (m_newSubtitles.size() > 0) {
        // 先移除重叠的旧字幕
        if (m_saveSubtitles.size() > 0) {
            m_model.doRemoveSubtitleItems(m_trackIndex, m_saveSubtitles);
        }
        // 再插入新字幕
        m_model.doInsertSubtitleItems(m_trackIndex, m_newSubtitles);
    }
}

void OverwriteSubtitlesCommand::undo()
{
    LOG_DEBUG() << m_newSubtitles.size();

    if (m_newSubtitles.size() > 0) {
        // 撤销：先移除新插入的字幕
        m_model.doRemoveSubtitleItems(m_trackIndex, m_newSubtitles);
        // 再恢复被覆盖的旧字幕
        if (m_saveSubtitles.size() > 0) {
            m_model.doInsertSubtitleItems(m_trackIndex, m_saveSubtitles);
        }
    }
}

/**
 * @class RemoveSubtitlesCommand
 * @brief 封装“删除字幕项”操作的撤销/重做命令。
 */
RemoveSubtitlesCommand::RemoveSubtitlesCommand(SubtitlesModel &model,
                                               int trackIndex,
                                               const QList<Subtitles::SubtitleItem> &items)
    : QUndoCommand(0)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_items(items) // 保存要删除的字幕项列表
{
    if (m_items.size() == 1) {
        setText(QObject::tr("Remove subtitle"));
    } else {
        setText(QObject::tr("Remove %n subtitles", nullptr, m_items.size()));
    }
}

void RemoveSubtitlesCommand::redo()
{
    LOG_DEBUG() << m_items.size();
    m_model.doRemoveSubtitleItems(m_trackIndex, m_items);
}

void RemoveSubtitlesCommand::undo()
{
    LOG_DEBUG() << m_items.size();
    // 撤销删除，即重新插入被删除的字幕项
    if (m_items.size() > 0) {
        m_model.doInsertSubtitleItems(m_trackIndex, m_items);
    }
}

/**
 * @class SetTextCommand
 * @brief 封装“修改字幕文本”操作的撤销/重做命令。
 */
SetTextCommand::SetTextCommand(SubtitlesModel &model,
                               int trackIndex,
                               int itemIndex,
                               const QString &text)
    : QUndoCommand(0)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_itemIndex(itemIndex)
    , m_newText(text) // 保存新文本
{
    setText(QObject::tr("Edit subtitle text"));
    // 获取并保存旧文本
    m_oldText = QString::fromStdString(m_model.getItem(trackIndex, itemIndex).text);
}

void SetTextCommand::redo()
{
    m_model.doSetText(m_trackIndex, m_itemIndex, m_newText);
}

void SetTextCommand::undo()
{
    m_model.doSetText(m_trackIndex, m_itemIndex, m_oldText);
}

/**
 * @brief 合并连续的文本修改操作。
 *
 * 当用户在文本框中连续输入时，可以将这些修改合并为一个撤销步骤。
 */
bool SetTextCommand::mergeWith(const QUndoCommand *other)
{
    const SetTextCommand *that = static_cast<const SetTextCommand *>(other);
    // 只有作用于同一个字幕项的文本修改才能合并
    if (m_trackIndex != that->m_trackIndex || m_itemIndex != that->m_itemIndex) {
        return false;
    }
    LOG_DEBUG() << "track" << m_trackIndex << "item" << m_itemIndex;
    // 用新命令的最终文本更新当前命令的“新文本”
    m_newText = that->m_newText;
    return true;
}

/**
 * @class SetStartCommand
 * @brief 封装“修改字幕开始时间”操作的撤销/重做命令。
 */
SetStartCommand::SetStartCommand(SubtitlesModel &model,
                                 int trackIndex,
                                 int itemIndex,
                                 int64_t msTime)
    : QUndoCommand(0)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_itemIndex(itemIndex)
    , m_newStart(msTime) // 保存新的开始时间
{
    setText(QObject::tr("Change subtitle start"));
    // 获取并保存旧的开始时间
    m_oldStart = m_model.getItem(trackIndex, itemIndex).start;
}

void SetStartCommand::redo()
{
    // 获取当前结束时间，然后设置新的开始时间
    int64_t endTime = m_model.getItem(m_trackIndex, m_itemIndex).end;
    m_model.doSetTime(m_trackIndex, m_itemIndex, m_newStart, endTime);
}

void SetStartCommand::undo()
{
    // 恢复旧的开始时间
    int64_t endTime = m_model.getItem(m_trackIndex, m_itemIndex).end;
    m_model.doSetTime(m_trackIndex, m_itemIndex, m_oldStart, endTime);
}

/**
 * @brief 合并连续的开始时间修改操作。
 */
bool SetStartCommand::mergeWith(const QUndoCommand *other)
{
    const SetStartCommand *that = static_cast<const SetStartCommand *>(other);
    if (m_trackIndex != that->m_trackIndex || m_itemIndex != that->m_itemIndex) {
        return false;
    }
    LOG_DEBUG() << "track" << m_trackIndex << "item" << m_itemIndex;
    m_newStart = that->m_newStart; // 更新为最终的开始时间
    return true;
}

/**
 * @class SetEndCommand
 * @brief 封装“修改字幕结束时间”操作的撤销/重做命令。
 */
SetEndCommand::SetEndCommand(SubtitlesModel &model, int trackIndex, int itemIndex, int64_t msTime)
    : QUndoCommand(0)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_itemIndex(itemIndex)
    , m_newEnd(msTime) // 保存新的结束时间
{
    setText(QObject::tr("Change subtitle end"));
    // 获取并保存旧的结束时间
    m_oldEnd = m_model.getItem(trackIndex, itemIndex).end;
}

void SetEndCommand::redo()
{
    // 获取当前开始时间，然后设置新的结束时间
    int64_t startTime = m_model.getItem(m_trackIndex, m_itemIndex).start;
    m_model.doSetTime(m_trackIndex, m_itemIndex, startTime, m_newEnd);
}

void SetEndCommand::undo()
{
    // 恢复旧的结束时间
    int64_t startTime = m_model.getItem(m_trackIndex, m_itemIndex).start;
    m_model.doSetTime(m_trackIndex, m_itemIndex, startTime, m_oldEnd);
}

/**
 * @brief 合并连续的结束时间修改操作。
 */
bool SetEndCommand::mergeWith(const QUndoCommand *other)
{
    const SetEndCommand *that = static_cast<const SetEndCommand *>(other);
    if (m_trackIndex != that->m_trackIndex || m_itemIndex != that->m_itemIndex) {
        return false;
    }
    LOG_DEBUG() << "track" << m_trackIndex << "item" << m_itemIndex;
    m_newEnd = that->m_newEnd; // 更新为最终的结束时间
    return true;
}

/**
 * @class MoveSubtitlesCommand
 * @brief 封装“移动字幕项”操作的撤销/重做命令。
 *
 * 该命令会移动一个或多个字幕项，保持它们之间的相对时间间隔不变。
 */
MoveSubtitlesCommand::MoveSubtitlesCommand(SubtitlesModel &model,
                                           int trackIndex,
                                           const QList<Subtitles::SubtitleItem> &items,
                                           int64_t msTime)
    : QUndoCommand(0)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_oldSubtitles(items) // 保存移动前的字幕项列表
{
    if (m_oldSubtitles.size() <= 0) {
        return;
    }
    if (m_oldSubtitles.size() == 1) {
        setText(QObject::tr("Move subtitle"));
    } else {
        setText(QObject::tr("Move %n subtitles", nullptr, m_oldSubtitles.size()));
    }
    // 计算移动的时间差（delta），并创建移动后的新字幕列表
    int64_t delta = msTime - m_oldSubtitles[0].start;
    for (int i = 0; i < m_oldSubtitles.size(); i++) {
        m_newSubtitles.push_back(m_oldSubtitles[i]);
        m_newSubtitles[i].start += delta; // 调整开始时间
        m_newSubtitles[i].end += delta;   // 调整结束时间
    }
}

void MoveSubtitlesCommand::redo()
{
    LOG_DEBUG() << m_oldSubtitles.size();
    // 移动操作：先移除旧位置的，再在新位置插入
    m_model.doRemoveSubtitleItems(m_trackIndex, m_oldSubtitles);
    m_model.doInsertSubtitleItems(m_trackIndex, m_newSubtitles);
}

void MoveSubtitlesCommand::undo()
{
    LOG_DEBUG() << m_oldSubtitles.size();
    // 撤销移动：先移除新位置的，再在旧位置插入
    m_model.doRemoveSubtitleItems(m_trackIndex, m_newSubtitles);
    m_model.doInsertSubtitleItems(m_trackIndex, m_oldSubtitles);
}

/**
 * @brief 合并连续的移动操作。
 */
bool MoveSubtitlesCommand::mergeWith(const QUndoCommand *other)
{
    const MoveSubtitlesCommand *that = static_cast<const MoveSubtitlesCommand *>(other);
    // 合并条件：作用于同一轨道，移动的字幕数量相同，且当前命令的“新位置”是下一个命令的“旧位置”
    if (m_trackIndex != that->m_trackIndex) {
        return false;
    }
    if (m_oldSubtitles.size() != that->m_oldSubtitles.size()) {
        return false;
    }
    if (m_newSubtitles[0].start != that->m_oldSubtitles[0].start) {
        return false;
    }
    LOG_DEBUG() << "track" << m_trackIndex;
    // 用新命令的最终位置更新当前命令的“新位置”
    m_newSubtitles = that->m_newSubtitles;
    return true;
}

} // namespace Subtitles
