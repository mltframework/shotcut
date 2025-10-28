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

#include "playlistcommands.h"
#include "Logger.h"
#include "docks/playlistdock.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"

#include <QTreeWidget>

namespace Playlist {

/**
 * @class AppendCommand
 * @brief 封装“追加到播放列表末尾”操作的撤销/重做命令。
 */
AppendCommand::AppendCommand(PlaylistModel &model,
                             const QString &xml,
                             bool emitModified,
                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_xml(xml) // 保存要追加的片段的 XML
    , m_emitModified(emitModified) // 标记是否需要发出“已修改”信号
{
    setText(QObject::tr("Append playlist item %1").arg(m_model.rowCount() + 1));
}

void AppendCommand::redo()
{
    LOG_DEBUG() << "";
    // 从 XML 创建一个新的 Producer
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.append(producer, m_emitModified);
    // 处理 UUID：如果是第一次 redo，则生成并保存；否则恢复已保存的 UUID
    if (m_uuid.isNull()) {
        m_uuid = MLT.ensureHasUuid(producer);
    } else {
        MLT.setUuid(producer, m_uuid);
    }
}

void AppendCommand::undo()
{
    LOG_DEBUG() << "";
    // 移除播放列表的最后一项
    m_model.remove(m_model.rowCount() - 1);
}

/**
 * @class InsertCommand
 * @brief 封装“插入到播放列表指定位置”操作的撤销/重做命令。
 */
InsertCommand::InsertCommand(PlaylistModel &model, const QString &xml, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_xml(xml)
    , m_row(row) // 保存插入的行号
{
    setText(QObject::tr("Insert playist item %1").arg(row + 1));
}

void InsertCommand::redo()
{
    LOG_DEBUG() << "row" << m_row;
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.insert(producer, m_row);
    // 处理 UUID，逻辑同 AppendCommand
    if (m_uuid.isNull()) {
        m_uuid = MLT.ensureHasUuid(producer);
    } else {
        MLT.setUuid(producer, m_uuid);
    }
}

void InsertCommand::undo()
{
    LOG_DEBUG() << "row" << m_row;
    // 移除指定行的项
    m_model.remove(m_row);
}

/**
 * @class UpdateCommand
 * @brief 封装“更新播放列表项”操作的撤销/重做命令。
 */
UpdateCommand::UpdateCommand(PlaylistModel &model, const QString &xml, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_newXml(xml) // 保存更新后的 XML
    , m_row(row)
{
    setText(QObject::tr("Update playlist item %1").arg(row + 1));
    // 获取当前项的信息，并保存其 XML 作为“旧状态”
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    info->producer->set_in_and_out(info->frame_in, info->frame_out);
    m_oldXml = MLT.XML(info->producer);
}

void UpdateCommand::redo()
{
    LOG_DEBUG() << "row" << m_row;
    Mlt::Producer producer(MLT.profile(), "xml-string", m_newXml.toUtf8().constData());
    m_model.update(m_row, producer);
    // 处理 UUID
    if (m_uuid.isNull()) {
        m_uuid = MLT.ensureHasUuid(producer);
    } else {
        MLT.setUuid(producer, m_uuid);
    }
}

void UpdateCommand::undo()
{
    LOG_DEBUG() << "row" << m_row;
    // 恢复到旧状态
    Mlt::Producer producer(MLT.profile(), "xml-string", m_oldXml.toUtf8().constData());
    m_model.update(m_row, producer);
}

/**
 * @brief 合并连续的更新操作。
 *
 * 如果用户连续多次更新同一个项目，可以将这些操作合并为一个撤销步骤。
 */
bool UpdateCommand::mergeWith(const QUndoCommand *other)
{
    const UpdateCommand *that = static_cast<const UpdateCommand *>(other);
    LOG_DEBUG() << "this row" << m_row << "that row" << that->m_row;
    // 只有作用于同一行的更新命令才能合并
    if (that->id() != id() || that->m_row != m_row)
        return false;
    // 用新命令的最终状态更新当前命令的“新状态”
    m_newXml = that->m_newXml;
    return true;
}

/**
 * @class RemoveCommand
 * @brief 封装“删除播放列表项”操作的撤销/重做命令。
 */
RemoveCommand::RemoveCommand(PlaylistModel &model, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
{
    // 在构造时保存被删除项的 XML 和 UUID
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    info->producer->set_in_and_out(info->frame_in, info->frame_out);
    m_xml = MLT.XML(info->producer);
    setText(QObject::tr("Remove playlist item %1").arg(row + 1));
    m_uuid = MLT.ensureHasUuid(*info->producer);
}

void RemoveCommand::redo()
{
    LOG_DEBUG() << "row" << m_row;
    m_model.remove(m_row);
}

void RemoveCommand::undo()
{
    LOG_DEBUG() << "row" << m_row;
    // 从保存的 XML 重建 Producer，并插入到原来的位置
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.insert(producer, m_row);
    // 恢复 UUID
    MLT.setUuid(producer, m_uuid);
}

/**
 * @class ClearCommand
 * @brief 封装“清空播放列表”操作的撤销/重做命令。
 */
ClearCommand::ClearCommand(PlaylistModel &model, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
{
    // 保存整个播放列表的 XML 和所有项的 UUID
    m_xml = MLT.XML(m_model.playlist());
    setText(QObject::tr("Clear playlist"));
    for (int i = 0; i < m_model.playlist()->count(); i++) {
        Mlt::Producer clip(m_model.playlist()->get_clip(i));
        if (clip.is_valid()) {
            m_uuids << MLT.ensureHasUuid(clip.parent());
        }
    }
}

void ClearCommand::redo()
{
    LOG_DEBUG() << "";
    m_model.clear();
}

void ClearCommand::undo()
{
    LOG_DEBUG() << "";
    // 从保存的 XML 重建整个播放列表
    Mlt::Producer *producer = new Mlt::Producer(MLT.profile(),
                                                "xml-string",
                                                m_xml.toUtf8().constData());
    if (producer->is_valid()) {
        producer->set("resource", "<playlist>");
        if (!MLT.setProducer(producer)) {
            m_model.load();
            // 恢复所有项的 UUID
            for (int i = 0; i < m_model.playlist()->count(); i++) {
                Mlt::Producer clip(m_model.playlist()->get_clip(i));
                if (clip.is_valid() && i < m_uuids.size()) {
                    MLT.setUuid(clip.parent(), m_uuids[i]);
                }
            }
            MLT.pause();
            MAIN.seekPlaylist(0);
        }
    } else {
        LOG_ERROR() << "failed to restore playlist from XML";
    }
}

/**
 * @class MoveCommand
 * @brief 封装“移动播放列表项”操作的撤销/重做命令。
 */
MoveCommand::MoveCommand(PlaylistModel &model, int from, int to, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_from(from)
    , m_to(to)
{
    setText(QObject::tr("Move item from %1 to %2").arg(from + 1).arg(to + 1));
}

void MoveCommand::redo()
{
    LOG_DEBUG() << "from" << m_from << "to" << m_to;
    m_model.move(m_from, m_to);
}

void MoveCommand::undo()
{
    LOG_DEBUG() << "from" << m_from << "to" << m_to;
    // 撤销移动，即从目标位置移回原位置
    m_model.move(m_to, m_from);
}

/**
 * @class SortCommand
 * @brief 封装“排序播放列表”操作的撤销/重做命令。
 */
SortCommand::SortCommand(PlaylistModel &model, int column, Qt::SortOrder order, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_column(column)
    , m_order(order)
{
    // 保存排序前的完整状态（XML 和 UUID）
    m_xml = MLT.XML(m_model.playlist());
    QString columnName = m_model.headerData(m_column, Qt::Horizontal, Qt::DisplayRole).toString();
    setText(QObject::tr("Sort playlist by %1").arg(columnName));
    for (int i = 0; i < m_model.playlist()->count(); i++) {
        Mlt::Producer clip(m_model.playlist()->get_clip(i));
        if (clip.is_valid()) {
            m_uuids << MLT.ensureHasUuid(clip.parent());
        }
    }
}

void SortCommand::redo()
{
    LOG_DEBUG() << m_column;
    m_model.sort(m_column, m_order);
}

void SortCommand::undo()
{
    LOG_DEBUG() << "";
    // 撤销排序，即恢复到排序前的状态
    Mlt::Producer *producer = new Mlt::Producer(MLT.profile(),
                                                "xml-string",
                                                m_xml.toUtf8().constData());
    if (producer->is_valid()) {
        producer->set("resource", "<playlist>");
        if (!MLT.setProducer(producer)) {
            m_model.load();
            for (int i = 0; i < m_model.playlist()->count(); i++) {
                Mlt::Producer clip(m_model.playlist()->get_clip(i));
                if (clip.is_valid() && i < m_uuids.size()) {
                    MLT.setUuid(clip.parent(), m_uuids[i]);
                }
            }
            MLT.pause();
            MAIN.seekPlaylist(0);
        }
    } else {
        LOG_ERROR() << "failed to restore playlist from XML";
    }
}

/**
 * @class TrimClipInCommand
 * @brief 封装“裁剪片段入点”操作的撤销/重做命令。
 */
TrimClipInCommand::TrimClipInCommand(PlaylistModel &model, int row, int in, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
    , m_oldIn(in) // 初始化为当前值，将在构造函数体中被覆盖
    , m_newIn(in) // 保存新的入点
    , m_out(-1)   // 出点将在构造函数体中获取
{
    setText(QObject::tr("Trim playlist item %1 in").arg(row + 1));
    // 获取当前片段的入点和出点，保存为“旧状态”
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    if (info) {
        m_oldIn = info->frame_in;
        m_out = info->frame_out;
    }
}

void TrimClipInCommand::redo()
{
    LOG_DEBUG() << "row" << m_row << "in" << m_newIn;
    m_model.setInOut(m_row, m_newIn, m_out);
}

void TrimClipInCommand::undo()
{
    LOG_DEBUG() << "row" << m_row << "in" << m_oldIn;
    m_model.setInOut(m_row, m_oldIn, m_out);
}

/**
 * @brief 合并连续的入点裁剪操作。
 */
bool TrimClipInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipInCommand *that = static_cast<const TrimClipInCommand *>(other);
    LOG_DEBUG() << "this row" << m_row << "that row" << that->m_row;
    if (that->id() != id() || that->m_row != m_row)
        return false;
    m_newIn = that->m_newIn; // 更新为最终的入点
    return true;
}

/**
 * @class TrimClipOutCommand
 * @brief 封装“裁剪片段出点”操作的撤销/重做命令。
 */
TrimClipOutCommand::TrimClipOutCommand(PlaylistModel &model, int row, int out, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
    , m_in(-1)     // 入点将在构造函数体中获取
    , m_oldOut(out) // 初始化为当前值，将被覆盖
    , m_newOut(out) // 保存新的出点
{
    setText(QObject::tr("Trim playlist item %1 out").arg(row + 1));
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    if (info) {
        m_in = info->frame_in;
        m_oldOut = info->frame_out;
    }
}

void TrimClipOutCommand::redo()
{
    LOG_DEBUG() << "row" << m_row << "out" << m_newOut;
    m_model.setInOut(m_row, m_in, m_newOut);
}

void TrimClipOutCommand::undo()
{
    LOG_DEBUG() << "row" << m_row << "out" << m_oldOut;
    m_model.setInOut(m_row, m_in, m_oldOut);
}

/**
 * @brief 合并连续的出点裁剪操作。
 */
bool TrimClipOutCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipOutCommand *that = static_cast<const TrimClipOutCommand *>(other);
    LOG_DEBUG() << "this row" << m_row << "that row" << that->m_row;
    if (that->id() != id() || that->m_row != m_row)
        return false;
    m_newOut = that->m_newOut; // 更新为最终的出点
    return true;
}

/**
 * @class ReplaceCommand
 * @brief 封装“替换播放列表项”操作的撤销/重做命令。
 */
ReplaceCommand::ReplaceCommand(PlaylistModel &model,
                               const QString &xml,
                               int row,
                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_newXml(xml)
    , m_row(row)
{
    setText(QObject::tr("Replace playlist item %1").arg(row + 1));
    // 保存被替换项的旧 XML 和 UUID
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    info->producer->set_in_and_out(info->frame_in, info->frame_out);
    m_uuid = MLT.ensureHasUuid(*info->producer);
    m_oldXml = MLT.XML(info->producer);
}

void ReplaceCommand::redo()
{
    LOG_DEBUG() << "row" << m_row;
    Mlt::Producer producer(MLT.profile(), "xml-string", m_newXml.toUtf8().constData());
    m_model.update(m_row, producer, true); // true 表示这是一个替换操作
}

void ReplaceCommand::undo()
{
    LOG_DEBUG() << "row" << m_row;
    // 恢复被替换的项
    Mlt::Producer producer(MLT.profile(), "xml-string", m_oldXml.toUtf8().constData());
    m_model.update(m_row, producer, true);
    MLT.setUuid(producer, m_uuid);
}

// --- Bin (箱子) 相关命令 ---

/**
 * @class NewBinCommand
 * @brief 封装“新建箱子”操作的撤销/重做命令。
 */
NewBinCommand::NewBinCommand(PlaylistModel &model,
                             QTreeWidget *tree,
                             const QString &bin,
                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_binTree(tree)
    , m_bin(bin)
{
    setText(QObject::tr("Add new bin: %1").arg(bin));
    // 保存当前所有箱子的配置
    auto props = m_model.playlist()->get_props(kShotcutBinsProperty);
    if (props && props->is_valid()) {
        m_oldBins.copy(*props, "");
    }
}

void NewBinCommand::redo()
{
    // 在 UI 上添加新箱子项
    auto item = new QTreeWidgetItem(m_binTree, {m_bin});
    auto icon = QIcon::fromTheme("folder", QIcon(":/icons/oxygen/32x32/places/folder.png"));
    item->setIcon(0, icon);

    PlaylistDock::sortBins(m_binTree);
    emit m_binTree->itemSelectionChanged();

    // 更新 MLT 属性中的箱子列表
    std::unique_ptr<Mlt::Properties> props(m_model.playlist()->get_props(kShotcutBinsProperty));
    if (!props || !props->is_valid()) {
        props.reset(new Mlt::Properties);
        m_model.playlist()->set(kShotcutBinsProperty, *props);
    }
    for (int i = PlaylistDock::SmartBinCount; i < m_binTree->topLevelItemCount(); ++i) {
        auto name = m_binTree->topLevelItem(i)->text(0);
        props->set(QString::number(i).toLatin1().constData(), name.toUtf8().constData());
    }
}

void NewBinCommand::undo()
{
    // 恢复旧的箱子配置
    m_model.playlist()->set(kShotcutBinsProperty, m_oldBins);
    // 从 UI 上删除新创建的箱子
    auto items = m_binTree->findItems(m_bin, Qt::MatchExactly);
    if (!items.isEmpty())
        delete items.first();
    // 重建箱子列表以保持同步
    RenameBinCommand::rebuildBinList(m_model, m_binTree);
}

/**
 * @class MoveToBinCommand
 * @brief 封装“移动项目到箱子”操作的撤销/重做命令。
 */
MoveToBinCommand::MoveToBinCommand(PlaylistModel &model,
                                   QTreeWidget *tree,
                                   const QString &bin,
                                   const QList<int> &rows,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_binTree(tree)
    , m_bin(bin)
{
    setText(QObject::tr("Move %n item(s) to bin: %1", "", rows.size()).arg(bin));
    // 保存每个项目移动前的箱子名称
    for (const auto row : rows) {
        auto clip = m_model.playlist()->get_clip(row);
        if (clip && clip->is_valid() && clip->parent().is_valid()) {
            m_oldData.append({row, clip->parent().get(kShotcutBinsProperty)});
        }
    }
}

void MoveToBinCommand::redo()
{
    // 将所有项目移动到新箱子
    for (auto &old : m_oldData) {
        m_model.setBin(old.row, m_bin);
    }
}

void MoveToBinCommand::undo()
{
    // 将所有项目移回原来的箱子
    for (auto &old : m_oldData) {
        m_model.setBin(old.row, old.bin);
    }
}

/**
 * @class RenameBinCommand
 * @brief 封装“重命名/删除箱子”操作的撤销/重做命令。
 */
RenameBinCommand::RenameBinCommand(PlaylistModel &model,
                                   QTreeWidget *tree,
                                   const QString &bin,
                                   const QString &newName,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_binTree(tree)
    , m_bin(bin)
    , m_newName(newName)
{
    // 如果新名称为空，则表示是删除操作
    if (newName.isEmpty()) {
        setText(QObject::tr("Remove bin: %1").arg(bin));
    } else {
        setText(QObject::tr("Rename bin: %1").arg(newName));
    }
}

void RenameBinCommand::redo()
{
    auto items = m_binTree->findItems(m_bin, Qt::MatchExactly);
    if (!items.isEmpty()) {
        m_binTree->blockSignals(true);
        items.first()->setSelected(false);
        m_binTree->blockSignals(false);
        if (m_newName.isEmpty()) {
            // --- 执行删除操作 ---
            delete items.first();

            // 从所有属于该箱子的播放列表项中移除箱子属性
            for (int i = 0; i < m_model.playlist()->count(); ++i) {
                auto clip = m_model.playlist()->get_clip(i);
                if (clip && clip->is_valid() && m_bin == clip->parent().get(kShotcutBinsProperty)) {
                    clip->parent().Mlt::Properties::clear(kShotcutBinsProperty);
                    m_removedRows << i;
                }
            }
            m_model.renameBin(m_bin);
            rebuildBinList(m_model, m_binTree);

            // 选中 "ALL" 箱子
            m_binTree->clearSelection();
        } else {
            // --- 执行重命名操作 ---
            items.first()->setText(0, m_newName);
            items.first()->setSelected(true);
            m_model.renameBin(m_bin, m_newName);
            rebuildBinList(m_model, m_binTree);

            // 重新选择箱子
            PlaylistDock::sortBins(m_binTree);
            emit m_binTree->itemSelectionChanged();
        }
    }
}

void RenameBinCommand::undo()
{
    auto items = m_binTree->findItems(m_newName, Qt::MatchExactly);
    if (m_newName.isEmpty()) {
        // --- 撤销删除操作 ---
        auto item = new QTreeWidgetItem(m_binTree, {m_bin});
        auto icon = QIcon::fromTheme("folder", QIcon(":/icons/oxygen/32x32/places/folder.png"));
        item->setIcon(0, icon);

        PlaylistDock::sortBins(m_binTree);

        // 恢复播放列表项的箱子属性
        for (auto row : m_removedRows) {
            m_model.playlist()->get_clip(row)->parent().set(kShotcutBinsProperty,
                                                            m_bin.toUtf8().constData());
        }
        m_model.renameBin(m_bin);
        rebuildBinList(m_model, m_binTree);
    } else if (!items.isEmpty()) {
        // --- 撤销重命名操作 ---
        m_binTree->blockSignals(true);
        m_binTree->clearSelection();
        m_binTree->blockSignals(false);
        items.first()->setText(0, m_bin);
        items.first()->setSelected(true);
        m_model.renameBin(m_newName, m_bin);
        rebuildBinList(m_model, m_binTree);

        // 重新选择箱子
        PlaylistDock::sortBins(m_binTree);
    }
    emit m_binTree->itemSelectionChanged();
}

/**
 * @brief 静态辅助函数，用于根据
