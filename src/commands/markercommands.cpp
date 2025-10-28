/*
 * Copyright (c) 2021 Meltytech, LLC
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

#include "markercommands.h"

#include "Logger.h"

namespace Markers {

/**
 * @class DeleteCommand
 * @brief 封装“删除标记”操作的撤销/重做命令。
 */
DeleteCommand::DeleteCommand(MarkersModel &model, const Marker &delMarker, int index)
    : QUndoCommand(0) // 没有父命令
    , m_model(model)   // 持有对 MarkersModel 的引用
    , m_delMarker(delMarker) // 保存被删除的标记，以便 undo 时恢复
    , m_index(index)   // 保存被删除标记的索引
{
    // 设置在撤销历史中显示的文本
    setText(QObject::tr("Delete marker: %1").arg(m_delMarker.text));
}

/// 执行“删除标记”操作
void DeleteCommand::redo()
{
    // 调用模型的私有方法 doRemove 来实际删除标记
    m_model.doRemove(m_index);
}

/// 撤销“删除标记”操作（即重新插入标记）
void DeleteCommand::undo()
{
    // 调用模型的私有方法 doInsert，在原来的位置上恢复被删除的标记
    m_model.doInsert(m_index, m_delMarker);
}

/**
 * @class AppendCommand
 * @brief 封装“添加标记”操作的撤销/重做命令。
 */
AppendCommand::AppendCommand(MarkersModel &model, const Marker &newMarker, int index)
    : QUndoCommand(0)
    , m_model(model)
    , m_newMarker(newMarker) // 保存新添加的标记
    , m_index(index)         // 保存新标记被添加后的索引
{
    setText(QObject::tr("Add marker: %1").arg(m_newMarker.text));
}

/// 执行“添加标记”操作
void AppendCommand::redo()
{
    // 调用模型的私有方法 doAppend 来添加标记
    m_model.doAppend(m_newMarker);
}

/// 撤销“添加标记”操作（即删除刚添加的标记）
void AppendCommand::undo()
{
    // 调用模型的私有方法 doRemove，根据索引删除刚添加的标记
    m_model.doRemove(m_index);
}

/**
 * @class UpdateCommand
 * @brief 封装“更新/移动标记”操作的撤销/重做命令。
 */
UpdateCommand::UpdateCommand(MarkersModel &model,
                             const Marker &newMarker,
                             const Marker &oldMarker,
                             int index)
    : QUndoCommand(0)
    , m_model(model)
    , m_newMarker(newMarker) // 保存标记的新状态
    , m_oldMarker(oldMarker) // 保存标记的旧状态，用于 undo
    , m_index(index)         // 保存被更新的标记的索引
{
    // 根据变化的类型智能地设置撤销历史中的显示文本
    if (m_newMarker.text == m_oldMarker.text && m_newMarker.color == m_oldMarker.color) {
        // 如果只有时间位置变化，则显示为“移动”
        setText(QObject::tr("Move marker: %1").arg(m_oldMarker.text));
    } else {
        // 如果文本或颜色变化，则显示为“编辑”
        setText(QObject::tr("Edit marker: %1").arg(m_oldMarker.text));
    }
}

/// 执行“更新标记”操作
void UpdateCommand::redo()
{
    // 调用模型的私有方法 doUpdate，应用新的标记状态
    m_model.doUpdate(m_index, m_newMarker);
}

/// 撤销“更新标记”操作（即恢复到旧状态）
void UpdateCommand::undo()
{
    // 调用模型的私有方法 doUpdate，恢复旧的标记状态
    m_model.doUpdate(m_index, m_oldMarker);
}

/**
 * @brief 尝试将另一个 UpdateCommand 与当前命令合并。
 *
 * 这对于连续操作（如拖动滑块调整标记位置）非常有用。可以将多个微小的更新合并为一个撤销步骤。
 */
bool UpdateCommand::mergeWith(const QUndoCommand *other)
{
    const UpdateCommand *that = static_cast<const UpdateCommand *>(other);
    LOG_DEBUG() << "this index" << m_index << "that index" << that->m_index;

    // 合并前提：必须是作用于同一个标记的 UpdateCommand
    if (that->id() != id() || that->m_index != m_index)
        return false;

    bool merge = false;
    // 情况1：新命令只改变了位置（start/end），与上一个移动操作合并
    if (that->m_newMarker.text == m_oldMarker.text && that->m_newMarker.color == m_oldMarker.color) {
        merge = true;
    }
    // 情况2：新命令只改变了文本/颜色，与上一个编辑操作合并
    else if (that->m_newMarker.end == m_oldMarker.end
               && that->m_newMarker.start == m_oldMarker.start) {
        merge = true;
    }

    if (!merge)
        return false;

    // 如果可以合并，就用新命令的最终状态更新当前命令的“新状态”
    m_newMarker = that->m_newMarker;
    return true;
}

/**
 * @class ClearCommand
 * @brief 封装“清空所有标记”操作的撤销/重做命令。
 */
ClearCommand::ClearCommand(MarkersModel &model, QList<Marker> &clearMarkers)
    : QUndoCommand(0)
    , m_model(model)
    , m_clearMarkers(clearMarkers) // 保存所有被清空的标记列表，用于 undo
{
    setText(QObject::tr("Clear markers"));
}

/// 执行“清空所有标记”操作
void ClearCommand::redo()
{
    // 调用模型的私有方法 doClear 来清空所有标记
    m_model.doClear();
}

/// 撤销“清空所有标记”操作（即恢复所有标记）
void ClearCommand::undo()
{
    // 调用模型的私有方法 doReplace，用保存的列表替换当前空的标记列表
    m_model.doReplace(m_clearMarkers);
}

} // namespace Markers
