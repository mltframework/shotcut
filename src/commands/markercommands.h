/*
 * Copyright (c) 2021-2023 Meltytech, LLC
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

#ifndef MARKERCOMMANDS_H
#define MARKERCOMMANDS_H

#include "models/markersmodel.h"

#include <QUndoCommand>

namespace Markers {

/**
 * @brief 定义用于撤销/重做命令的唯一整数 ID。
 *
 * 这个 ID 被 QUndoCommand::id() 方法返回，用于在命令合并时区分不同类型的命令。
 * 只有 ID 相同的命令才有可能被合并。
 */
enum {
    UndoIdUpdate = 200, ///< 更新标记命令的 ID
};

/**
 * @class DeleteCommand
 * @brief 封装“删除标记”操作的撤销/重做命令。
 */
class DeleteCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的标记模型，用于执行实际的删除/插入操作。
     * @param delMarker 被删除的标记对象，用于在撤销时恢复。
     * @param index 被删除标记在模型中的索引。
     */
    DeleteCommand(MarkersModel &model, const Marker &delMarker, int index);

    void redo(); ///< 执行删除操作。
    void undo(); ///< 撤销删除操作（即重新插入标记）。

private:
    MarkersModel &m_model; ///< 对标记模型的引用。
    Marker m_delMarker;     ///< 保存被删除的标记数据。
    int m_index;            ///< 保存被删除标记的索引。
};

/**
 * @class AppendCommand
 * @brief 封装“添加标记”操作的撤销/重做命令。
 */
class AppendCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的标记模型。
     * @param newMarker 新添加的标记对象。
     * @param index 新标记被添加后所在的索引。
     */
    AppendCommand(MarkersModel &model, const Marker &newMarker, int index);

    void redo(); ///< 执行添加操作。
    void undo(); ///< 撤销添加操作（即删除刚添加的标记）。

private:
    MarkersModel &m_model; ///< 对标记模型的引用。
    Marker m_newMarker;     ///< 保存新添加的标记数据。
    int m_index;            ///< 保存新标记的索引。
};

/**
 * @class UpdateCommand
 * @brief 封装“更新/移动标记”操作的撤销/重做命令。
 */
class UpdateCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的标记模型。
     * @param newMarker 标记的新状态（更新后的数据）。
     * @param oldMarker 标记的旧状态（更新前的数据），用于撤销。
     * @param index 被更新的标记在模型中的索引。
     */
    UpdateCommand(MarkersModel &model, const Marker &newMarker, const Marker &oldMarker, int index);

    void redo(); ///< 执行更新操作。
    void undo(); ///< 撤销更新操作（即恢复到旧状态）。

protected:
    int id() const { return UndoIdUpdate; } ///< 返回命令的唯一 ID，用于合并。
    bool mergeWith(const QUndoCommand *other); ///< 尝试与另一个命令合并。

private:
    MarkersModel &m_model; ///< 对标记模型的引用。
    Marker m_newMarker;     ///< 保存标记的新状态。
    Marker m_oldMarker;     ///< 保存标记的旧状态。
    int m_index;            ///< 保存被更新标记的索引。
};

/**
 * @class ClearCommand
 * @brief 封装“清空所有标记”操作的撤销/重做命令。
 */
class ClearCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的标记模型。
     * @param clearMarkers 包含所有被清空的标记的列表，用于撤销时恢复。
     */
    ClearCommand(MarkersModel &model, QList<Marker> &clearMarkers);

    void redo(); ///< 执行清空操作。
    void undo(); ///< 撤销清空操作（即恢复所有标记）。

private:
    MarkersModel &m_model;        ///< 对标记模型的引用。
    QList<Marker> m_clearMarkers; ///< 保存所有被清空的标记列表。
};

} // namespace Markers

#endif // MARKERCOMMANDS_H
