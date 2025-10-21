/*
 * Copyright (c) 2013-2024 Meltytech, LLC
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

#include <QString>
#include <QUndoCommand>
#include <QUuid>

// 前向声明，避免包含完整的头文件
class QTreeWidget;

namespace Playlist {

/**
 * @brief 定义用于撤销/重做命令的唯一整数 ID。
 *
 * 这些 ID 被 QUndoCommand::id() 方法返回，用于在命令合并时区分不同类型的命令。
 * 只有 ID 相同的命令才有可能被合并。
 */
enum { 
    UndoIdTrimClipIn = 0,  ///< 裁剪片段入点命令的 ID
    UndoIdTrimClipOut,     ///< 裁剪片段出点命令的 ID
    UndoIdUpdate           ///< 更新播放列表项命令的 ID
};

/**
 * @class AppendCommand
 * @brief 封装“追加到播放列表末尾”操作的撤销/重做命令。
 */
class AppendCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param xml 要追加的片段的 XML 字符串。
     * @param emitModified 是否在操作后发出“已修改”信号。
     * @param parent 父命令。
     */
    AppendCommand(PlaylistModel &model,
                  const QString &xml,
                  bool emitModified = true,
                  QUndoCommand *parent = 0);

    void redo(); ///< 执行追加操作。
    void undo(); ///< 撤销追加操作。

private:
    PlaylistModel &m_model; ///< 对播放列表模型的引用。
    QString m_xml;           ///< 保存要追加的片段的 XML。
    bool m_emitModified;     ///< 是否发出修改信号。
    QUuid m_uuid;            ///< 保存片段的 UUID，用于在撤销/重做时保持关联。
};

/**
 * @class InsertCommand
 * @brief 封装“插入到播放列表指定位置”操作的撤销/重做命令。
 */
class InsertCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param xml 要插入的片段的 XML 字符串。
     * @param row 插入的目标行号。
     * @param parent 父命令。
     */
    InsertCommand(PlaylistModel &model, const QString &xml, int row, QUndoCommand *parent = 0);

    void redo(); ///< 执行插入操作。
    void undo(); ///< 撤销插入操作。

private:
    PlaylistModel &m_model; ///< 对播放列表模型的引用。
    QString m_xml;           ///< 保存要插入的片段的 XML。
    int m_row;               ///< 保存插入的目标行号。
    QUuid m_uuid;            ///< 保存片段的 UUID。
};

/**
 * @class UpdateCommand
 * @brief 封装“更新播放列表项”操作的撤销/重做命令。
 */
class UpdateCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param xml 更新后的片段的 XML 字符串。
     * @param row 要更新的行号。
     * @param parent 父命令。
     */
    UpdateCommand(PlaylistModel &model, const QString &xml, int row, QUndoCommand *parent = 0);

    void redo(); ///< 执行更新操作。
    void undo(); ///< 撤销更新操作。

protected:
    int id() const { return UndoIdUpdate; } ///< 返回命令的唯一 ID。
    bool mergeWith(const QUndoCommand *other); ///< 尝试与另一个命令合并。

private:
    PlaylistModel &m_model; ///< 对播放列表模型的引用。
    QString m_newXml;       ///< 保存更新后的 XML。
    QString m_oldXml;       ///< 保存更新前的 XML，用于撤销。
    int m_row;              ///< 保存被更新的行号。
    QUuid m_uuid;           ///< 保存片段的 UUID。
};

/**
 * @class RemoveCommand
 * @brief 封装“删除播放列表项”操作的撤销/重做命令。
 */
class RemoveCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param row 要删除的行号。
     * @param parent 父命令。
     */
    RemoveCommand(PlaylistModel &model, int row, QUndoCommand *parent = 0);

    void redo(); ///< 执行删除操作。
    void undo(); ///< 撤销删除操作。

private:
    PlaylistModel &m_model; ///< 对播放列表模型的引用。
    QString m_xml;           ///< 保存被删除片段的 XML，用于撤销。
    int m_row;               ///< 保存被删除的行号。
    QUuid m_uuid;            ///< 保存被删除片段的 UUID。
};

/**
 * @class MoveCommand
 * @brief 封装“移动播放列表项”操作的撤销/重做命令。
 */
class MoveCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param from 移动的源行号。
     * @param to 移动的目标行号。
     * @param parent 父命令。
     */
    MoveCommand(PlaylistModel &model, int from, int to, QUndoCommand *parent = 0);

    void redo(); ///< 执行移动操作。
    void undo(); ///< 撤销移动操作。

private:
    PlaylistModel &m_model; ///< 对播放列表模型的引用。
    int m_from;             ///< 源行号。
    int m_to;               ///< 目标行号。
};

/**
 * @class ClearCommand
 * @brief 封装“清空播放列表”操作的撤销/重做命令。
 */
class ClearCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param parent 父命令。
     */
    ClearCommand(PlaylistModel &model, QUndoCommand *parent = 0);

    void redo(); ///< 执行清空操作。
    void undo(); ///< 撤销清空操作。

private:
    PlaylistModel &m_model;     ///< 对播放列表模型的引用。
    QString m_xml;               ///< 保存清空前整个播放列表的 XML。
    QVector<QUuid> m_uuids;      ///< 保存清空前所有片段的 UUID 列表。
};

/**
 * @class SortCommand
 * @brief 封装“排序播放列表”操作的撤销/重做命令。
 */
class SortCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param column 排序依据的列。
     * @param order 排序方式（升序或降序）。
     * @param parent 父命令。
     */
    SortCommand(PlaylistModel &model, int column, Qt::SortOrder order, QUndoCommand *parent = 0);

    void redo(); ///< 执行排序操作。
    void undo(); ///< 撤销排序操作。

private:
    PlaylistModel &m_model;     ///< 对播放列表模型的引用。
    int m_column;               ///< 排序依据的列。
    Qt::SortOrder m_order;      ///< 排序方式。
    QString m_xml;               ///< 保存排序前整个播放列表的 XML。
    QVector<QUuid> m_uuids;      ///< 保存排序前所有片段的 UUID 列表。
};

/**
 * @class TrimClipInCommand
 * @brief 封装“裁剪片段入点”操作的撤销/重做命令。
 */
class TrimClipInCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param row 要裁剪的片段所在的行号。
     * @param in 新的入点。
     * @param parent 父命令。
     */
    TrimClipInCommand(PlaylistModel &model, int row, int in, QUndoCommand *parent = nullptr);

    void redo(); ///< 执行裁剪操作。
    void undo(); ///< 撤销裁剪操作。

protected:
    int id() const { return UndoIdTrimClipIn; } ///< 返回命令的唯一 ID。
    bool mergeWith(const QUndoCommand *other);  ///< 尝试与另一个命令合并。

private:
    PlaylistModel &m_model; ///< 对播放列表模型的引用。
    int m_row;              ///< 片段所在的行号。
    int m_oldIn;            ///< 旧的入点，用于撤销。
    int m_newIn;            ///< 新的入点。
    int m_out;              ///< 片段的出点（裁剪入点时保持不变）。
};

/**
 * @class TrimClipOutCommand
 * @brief 封装“裁剪片段出点”操作的撤销/重做命令。
 */
class TrimClipOutCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param row 要裁剪的片段所在的行号。
     * @param out 新的出点。
     * @param parent 父命令。
     */
    TrimClipOutCommand(PlaylistModel &model, int row, int out, QUndoCommand *parent = nullptr);

    void redo(); ///< 执行裁剪操作。
    void undo(); ///< 撤销裁剪操作。

protected:
    int id() const { return UndoIdTrimClipOut; } ///< 返回命令的唯一 ID。
    bool mergeWith(const QUndoCommand *other);   ///< 尝试与另一个命令合并。

private:
    PlaylistModel &m_model; ///< 对播放列表模型的引用。
    int m_row;              ///< 片段所在的行号。
    int m_in;               ///< 片段的入点（裁剪出点时保持不变）。
    int m_oldOut;           ///< 旧的出点，用于撤销。
    int m_newOut;           ///< 新的出点。
};

/**
 * @class ReplaceCommand
 * @brief 封装“替换播放列表项”操作的撤销/重做命令。
 */
class ReplaceCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param xml 替换后的片段的 XML 字符串。
     * @param row 要替换的行号。
     * @param parent 父命令。
     */
    ReplaceCommand(PlaylistModel &model, const QString &xml, int row, QUndoCommand *parent = 0);

    void redo(); ///< 执行替换操作。
    void undo(); ///< 撤销替换操作。

private:
    PlaylistModel &m_model; ///< 对播放列表模型的引用。
    QString m_newXml;       ///< 保存替换后的 XML。
    QString m_oldXml;       ///< 保存替换前的 XML，用于撤销。
    int m_row;              ///< 保存被替换的行号。
    QUuid m_uuid;           ///< 保存被替换片段的 UUID。
};

// --- Bin (箱子) 相关命令 ---

/**
 * @class NewBinCommand
 * @brief 封装“新建箱子”操作的撤销/重做命令。
 */
class NewBinCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param tree 显示箱子的树形控件。
     * @param bin 新箱子的名称。
     * @param parent 父命令。
     */
    NewBinCommand(PlaylistModel &model,
                  QTreeWidget *tree,
                  const QString &bin,
                  QUndoCommand *parent = 0);

    void redo(); ///< 执行新建操作。
    void undo(); ///< 撤销新建操作。

private:
    PlaylistModel &m_model;   ///< 对播放列表模型的引用。
    QTreeWidget *m_binTree;   ///< 指向箱子树形控件的指针。
    QString m_bin;            ///< 新箱子的名称。
    Mlt::Properties m_oldBins; ///< 保存操作前的所有箱子配置，用于撤销。
};

/**
 * @class MoveToBinCommand
 * @brief 封装“移动项目到箱子”操作的撤销/重做命令。
 */
class MoveToBinCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param tree 显示箱子的树形控件。
     * @param bin 目标箱子的名称。
     * @param rows 要移动的项目的行号列表。
     * @param parent 父命令。
     */
    MoveToBinCommand(PlaylistModel &model,
                     QTreeWidget *tree,
                     const QString &bin,
                     const QList<int> &rows,
                     QUndoCommand *parent = 0);

    void redo(); ///< 执行移动操作。
    void undo(); ///< 撤销移动操作。

private:
    PlaylistModel &m_model; ///< 对播放列表模型的引用。
    QTreeWidget *m_binTree; ///< 指向箱子树形控件的指针。
    QString m_bin;          ///< 目标箱子的名称。

    /// 用于保存每个项目移动前所属的箱子
    typedef struct
    {
        int row;      ///< 项目在播放列表中的行号
        QString bin;  ///< 项目原来所属的箱子名称
    } oldData;
    QList<oldData> m_oldData; ///< 保存所有被移动项目的原始信息
};

/**
 * @class RenameBinCommand
 * @brief 封装“重命名/删除箱子”操作的撤销/重做命令。
 */
class RenameBinCommand : public QUndoCommand
{
public:
    /**
     * @brief 构造函数。
     * @param model 关联的播放列表模型。
     * @param tree 显示箱子的树形控件。
     * @param bin 要重命名/删除的箱子的当前名称。
     * @param newName 新名称。如果为空，则表示删除操作。
     * @param parent 父命令。
     */
    RenameBinCommand(PlaylistModel &model,
                     QTreeWidget *tree,
                     const QString &bin,
                     const QString &newName = QString(),
                     QUndoCommand *parent = 0);

    void redo(); ///< 执行重命名/删除操作。
    void undo(); ///< 撤销重命名/删除操作。
    
    /**
     * @brief 静态辅助函数，用于根据 UI 上的箱子树重建 MLT 属性中的箱子列表。
     * @param model 播放列表模型。
     * @param binTree 箱子树形控件。
     */
    static void rebuildBinList(PlaylistModel &model, QTreeWidget *binTree);

private:
    PlaylistModel &m_model;   ///< 对播放列表模型的引用。
    QTreeWidget *m_binTree;   ///< 指向箱子树形控件的指针。
    QString m_bin;            ///< 箱子的原始名称。
    QString m_newName;        ///< 箱子的新名称。
    QList<int> m_removedRows; ///< 如果是删除操作，这里保存了被移除箱子的所有项目的行号，用于撤销。
};

} // namespace Playlist

#endif // PLAYLISTCOMMANDS_H
