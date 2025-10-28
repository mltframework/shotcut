/*
 * Copyright (c) 2021-2024 Meltytech, LLC
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

#ifndef FILTERCOMMANDS_H
#define FILTERCOMMANDS_H

#include "models/attachedfiltersmodel.h"

#include <MltProducer.h>
#include <MltService.h>
#include <QString>
#include <QUndoCommand>
#include <QUuid>

// 前向声明，避免包含完整的头文件
class QmlMetadata;
class FilterController;

namespace Filter {

/**
 * @brief 定义用于撤销/重做命令的唯一整数 ID。
 *
 * 这些 ID 被 QUndoCommand::id() 方法返回，用于在命令合并时区分不同类型的命令。
 * 只有 ID 相同的命令才有可能被合并。
 */
enum {
    UndoIdAdd = 300,              ///< 添加滤镜命令的 ID
    UndoIdMove,                   ///< 移动滤镜命令的 ID
    UndoIdDisable,                ///< 禁用/启用滤镜命令的 ID
    UndoIdChangeParameter,        ///< 修改滤镜参数命令的 ID
    UndoIdChangeAddKeyframe,      ///< 添加关键帧命令的 ID
    UndoIdChangeRemoveKeyframe,   ///< 删除关键帧命令的 ID
    UndoIdChangeKeyframe,         ///< 修改关键帧命令的 ID (似乎未使用)
};

/**
 * @class AddCommand
 * @brief 封装“添加滤镜”操作的撤销/重做命令。
 */
class AddCommand : public QUndoCommand
{
public:
    /**
     * @brief 定义添加操作的类型。
     */
    typedef enum {
        AddSingle,   ///< 添加单个滤镜
        AddSet,      ///< 添加滤镜集中的一个滤镜（非最后一个）
        AddSetLast,  ///< 添加滤镜集的最后一个滤镜
    } AddType;

    /**
     * @brief 构造函数。
     * @param model 关联的滤镜模型，用于执行实际的添加/删除操作。
     * @param name 滤镜的名称，用于在撤销历史中显示。
     * @param service 要添加的 MLT 服务（滤镜）。
     * @param row 滤镜要被添加到的目标行号。
     * @param type 添加操作的类型。
     * @param parent 父命令，用于构建宏命令。
     */
    AddCommand(AttachedFiltersModel &model,
               const QString &name,
               Mlt::Service &service,
               int row,
               AddCommand::AddType type = AddCommand::AddSingle,
               QUndoCommand *parent = 0);

    void redo(); ///< 执行添加操作。
    void undo(); ///< 撤销添加操作。

protected:
    int id() const { return UndoIdAdd; } ///< 返回命令的唯一 ID。
    bool mergeWith(const QUndoCommand *other); ///< 尝试与另一个命令合并。

private:
    AttachedFiltersModel &m_model;      ///< 对滤镜模型的引用。
    std::vector<int> m_rows;            ///< 要添加的滤镜的行号列表（支持批量添加）。
    std::vector<Mlt::Service> m_services; ///< 要添加的 MLT 服务列表。
    Mlt::Producer m_producer;           ///< 目标 Producer 的引用（仅在第一次 redo 前有效）。
    QUuid m_producerUuid;               ///< 目标 Producer 的唯一标识符，用于后续查找。
    AddType m_type;                     ///< 添加操作的类型。
};

/**
 * @class RemoveCommand
 * @brief 封装“删除滤镜”操作的撤销/重做命令。
 */
class RemoveCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的滤镜模型。
     * @param name 滤镜的名称。
     * @param service 被删除的 MLT 服务。
     * @param row 被删除滤镜所在的行号。
     * @param parent 父命令。
     */
    RemoveCommand(AttachedFiltersModel &model,
                  const QString &name,
                  Mlt::Service &service,
                  int row,
                  QUndoCommand *parent = 0);

    void redo(); ///< 执行删除操作。
    void undo(); ///< 撤销删除操作（即重新添加）。

private:
    AttachedFiltersModel &m_model; ///< 对滤镜模型的引用。
    int m_index;                   ///< 未使用的成员变量（可能是遗留代码）。
    int m_row;                     ///< 被删除滤镜的行号。
    Mlt::Producer m_producer;      ///< 目标 Producer 的引用。
    QUuid m_producerUuid;          ///< 目标 Producer 的 UUID。
    Mlt::Service m_service;        ///< 被删除的 MLT 服务，用于 undo 时恢复。
};

/**
 * @class MoveCommand
 * @brief 封装“移动滤镜”操作的撤销/重做命令。
 */
class MoveCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的滤镜模型。
     * @param name 滤镜的名称。
     * @param fromRow 滤镜的原始行号。
     * @param toRow 滤镜的目标行号。
     * @param parent 父命令。
     */
    MoveCommand(AttachedFiltersModel &model,
                const QString &name,
                int fromRow,
                int toRow,
                QUndoCommand *parent = 0);

    void redo(); ///< 执行移动操作。
    void undo(); ///< 撤销移动操作（即移回原位）。

protected:
    int id() const { return UndoIdMove; } ///< 返回命令的唯一 ID。

private:
    AttachedFiltersModel &m_model; ///< 对滤镜模型的引用。
    int m_fromRow;                 ///< 滤镜的原始行号。
    int m_toRow;                   ///< 滤镜的目标行号。
    Mlt::Producer m_producer;      ///< 目标 Producer 的引用。
    QUuid m_producerUuid;          ///< 目标 Producer 的 UUID。
};

/**
 * @class DisableCommand
 * @brief 封装“禁用/启用滤镜”操作的撤销/重做命令。
 */
class DisableCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的滤镜模型。
     * @param name 滤镜的名称。
     * @param row 滤镜所在的行号。
     * @param disabled 目标状态：true 表示禁用，false 表示启用。
     * @param parent 父命令。
     */
    DisableCommand(AttachedFiltersModel &model,
                   const QString &name,
                   int row,
                   bool disabled,
                   QUndoCommand *parent = 0);

    void redo(); ///< 执行禁用/启用操作。
    void undo(); ///< 撤销操作（即恢复到相反状态）。

protected:
    int id() const { return UndoIdDisable; } ///< 返回命令的唯一 ID。
    bool mergeWith(const QUndoCommand *other); ///< 尝试与另一个命令合并（此处被禁用）。

private:
    AttachedFiltersModel &m_model; ///< 对滤镜模型的引用。
    int m_row;                     ///< 滤镜所在的行号。
    Mlt::Producer m_producer;      ///< 目标 Producer 的引用。
    QUuid m_producerUuid;          ///< 目标 Producer 的 UUID。
    bool m_disabled;               ///< 滤镜的目标状态。
};

/**
 * @class PasteCommand
 * @brief 封装“粘贴滤镜”操作的撤销/重做命令。
 */
class PasteCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的滤镜模型。
     * @param filterProducerXml 包含要粘贴的滤镜的 XML 字符串。
     * @param parent 父命令。
     */
    PasteCommand(AttachedFiltersModel &model,
                 const QString &filterProducerXml,
                 QUndoCommand *parent = 0);

    void redo(); ///< 执行粘贴操作。
    void undo(); ///< 撤销粘贴操作（恢复到粘贴前的状态）。

private:
    AttachedFiltersModel &m_model; ///< 对滤镜模型的引用。
    QString m_xml;                 ///< 要粘贴的滤镜的 XML。
    QString m_beforeXml;           ///< 粘贴前所有滤镜的 XML，用于 undo。
    QUuid m_producerUuid;          ///< 目标 Producer 的 UUID。
};

/**
 * @class UndoParameterCommand
 * @brief 封装“修改滤镜参数”操作的撤销/重做命令。
 */
class UndoParameterCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param name 滤镜的名称。
     * @param controller 滤镜控制器，用于更新 UI。
     * @param row 滤镜所在的行号。
     * @param before 修改前的参数状态。
     * @param desc 操作描述，用于在撤销历史中显示。
     * @param parent 父命令。
     */
    UndoParameterCommand(const QString &name,
                         FilterController *controller,
                         int row,
                         Mlt::Properties &before,
                         const QString &desc = QString(),
                         QUndoCommand *parent = 0);

    /**
     * @brief 更新命令中的“修改后”状态。
     * 用于在连续调整参数时合并命令，而不是为每次微调都创建新命令。
     * @param propertyName 被更新的参数名称。
     */
    void update(const QString &propertyName);

    void redo(); ///< 执行参数修改。
    void undo(); ///< 撤销参数修改。

protected:
    int id() const { return UndoIdChangeParameter; } ///< 返回命令的唯一 ID。
    bool mergeWith(const QUndoCommand *other); ///< 尝试与另一个命令合并。

private:
    int m_row;                         ///< 滤镜所在的行号。
    QUuid m_producerUuid;              ///< 目标 Producer 的 UUID。
    Mlt::Properties m_before;          ///< 修改前的参数状态。
    Mlt::Properties m_after;           ///< 修改后的参数状态。
    FilterController *m_filterController; ///< 滤镜控制器指针。
    bool m_firstRedo;                  ///< 标记是否是第一次调用 redo()。
};

/**
 * @class UndoAddKeyframeCommand
 * @brief 封装“添加关键帧”操作的撤销/重做命令。
 *
 * 继承自 UndoParameterCommand，因为添加关键帧本质上也是修改滤镜的参数（动画属性）。
 */
class UndoAddKeyframeCommand : public UndoParameterCommand
{
public:
    /**
     * @brief 构造函数。
     * @param name 滤镜名称。
     * @param controller 滤镜控制器。
     * @param row 滤镜行号。
     * @param before 修改前的参数状态。
     */
    UndoAddKeyframeCommand(const QString &name,
                           FilterController *controller,
                           int row,
                           Mlt::Properties &before)
        : UndoParameterCommand(name, controller, row, before, QObject::tr("add keyframe"))
    {}

protected:
    int id() const { return UndoIdChangeAddKeyframe; } ///< 返回唯一的 ID。
    bool mergeWith(const QUndoCommand *other) { return false; } ///< 禁止合并，因为每次添加都是独立的操作。
};

/**
 * @class UndoRemoveKeyframeCommand
 * @brief 封装“删除关键帧”操作的撤销/重做命令。
 */
class UndoRemoveKeyframeCommand : public UndoParameterCommand
{
public:
    UndoRemoveKeyframeCommand(const QString &name,
                              FilterController *controller,
                              int row,
                              Mlt::Properties &before)
        : UndoParameterCommand(name, controller, row, before, QObject::tr("remove keyframe"))
    {}

protected:
    int id() const { return UndoIdChangeRemoveKeyframe; } ///< 返回唯一的 ID。
    bool mergeWith(const QUndoCommand *other) { return false; } ///< 禁止合并。
};

/**
 * @class UndoModifyKeyframeCommand
 * @brief 封装“修改关键帧”操作的撤销/重做命令。
 */
class UndoModifyKeyframeCommand : public UndoParameterCommand
{
public:
    UndoModifyKeyframeCommand(const QString &name,
                              FilterController *controller,
                              int row,
                              Mlt::Properties &before,
                              int paramIndex,
                              int keyframeIndex)
        : UndoParameterCommand(name, controller, row, before, QObject::tr("modify keyframe"))
        , m_paramIndex(paramIndex)
        , m_keyframeIndex(keyframeIndex)
    {}

protected:
    int id() const { return UndoIdChangeRemoveKeyframe; } ///< 注意：这里返回的 ID 可能是笔误，应为 UndoIdChangeKeyframe。
    bool mergeWith(const QUndoCommand *other)
    {
        // 只有当修改的是同一个参数的同一个关键帧时，才允许合并。
        auto *that = dynamic_cast<const UndoModifyKeyframeCommand *>(other);
        if (!that || m_paramIndex != that->m_paramIndex || m_keyframeIndex != that->m_keyframeIndex)
            return false;
        else
            return UndoParameterCommand::mergeWith(other);
    }

private:
    int m_paramIndex;    ///< 被修改的参数的索引。
    int m_keyframeIndex; ///< 被修改的关键帧的索引。
};

} // namespace Filter

#endif // FILTERCOMMANDS_H

