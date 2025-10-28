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

#include "timelinecommands.h"

#include "Logger.h"
#include "controllers/filtercontroller.h"
#include "dialogs/longuitask.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "proxymanager.h"
#include "qmltypes/qmlmetadata.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <QMetaObject>

namespace Timeline {

/// ####################################################################
/// #                      辅助函数                                #
/// ####################################################################

/**
 * @brief 从 XML 字符串反序列化一个 Mlt::Producer 对象。
 * @param xml 包含 Producer 数据的 XML 字符串。
 * @return 一个新创建的 Mlt::Producer 对象，调用者需要负责释放其内存。
 */
Mlt::Producer *deserializeProducer(QString &xml)
{
    return new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData());
}

/**
 * @brief 获取一个 Producer（或播放列表中的所有 Producer）的 UUID 列表。
 * @param producer 要检查的 Mlt::Producer 对象。
 * @return 一个包含所有相关 UUID 的 QVector。
 */
QVector<QUuid> getProducerUuids(Mlt::Producer *producer)
{
    QVector<QUuid> uuids;
    // 如果 Producer 是一个播放列表（多个片段）
    if (producer->type() == mlt_service_playlist_type) {
        Mlt::Playlist playlist(*producer);
        int count = playlist.count();
        for (int i = 0; i < count; i++) {
            QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(i));
            Mlt::Producer clip = Mlt::Producer(info->producer);
            // 确保每个片段的父 Producer 都有 UUID，并收集起来
            uuids << MLT.ensureHasUuid(clip.parent());
        }
    } else { // 如果是单个片段
        uuids << MLT.ensureHasUuid(*producer);
    }
    return uuids;
}

/**
 * @brief 在整个时间线中找到一个唯一的组编号。
 * @param model 多轨道模型。
 * @return 一个未被使用的整数组编号。
 */
int getUniqueGroupNumber(MultitrackModel &model)
{
    QSet<int> groups; // 用于存储已存在的组编号
    // 遍历所有轨道
    for (int trackIndex = 0; trackIndex < model.trackList().size(); trackIndex++) {
        int i = model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            // 遍历轨道上的所有片段
            for (int clipIndex = 0; clipIndex < playlist.count(); clipIndex++) {
                QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
                // 如果片段存在组属性，则将其编号添加到集合中
                if (info && info->cut && info->cut->property_exists(kShotcutGroupProperty)) {
                    groups.insert(info->cut->get_int(kShotcutGroupProperty));
                }
            }
        }
    }
    // 从 0 开始查找，直到找到一个未被使用的编号
    static const int MAX_GROUPS = 5000;
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (!groups.contains(i)) {
            return i;
        }
    }
    LOG_ERROR() << "More than" << MAX_GROUPS << "groups!";
    return 0; // 如果超过最大值，返回 0
}

/// ####################################################################
/// #                          命令实现                                #
/// ####################################################################

/**
 * @class AppendCommand
 * @brief “追加到轨道”命令
 * 将一个或多个片段添加到指定轨道的末尾。
 */
AppendCommand::AppendCommand(MultitrackModel &model,
                             int trackIndex,
                             const QString &xml,
                             bool skipProxy,
                             bool seek,
                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0))) // 确保轨道索引有效
    , m_xml(xml)
    , m_undoHelper(m_model)
    , m_skipProxy(skipProxy) // 是否跳过代理生成
    , m_seek(seek) // 操作完成后是否跳转到新片段
{
    setText(QObject::tr("Append to track"));
}

void AppendCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex;
    LongUiTask longTask(QObject::tr("Append to Timeline"));
    m_undoHelper.recordBeforeState(); // 记录操作前的状态，用于撤销
    // 在后台线程中反序列化 XML，避免阻塞 UI
    Mlt::Producer *producer = longTask.runAsync<Mlt::Producer *>(QObject::tr("Preparing"), [=]() {
        return deserializeProducer(m_xml);
    });
    if (!producer || !producer->is_valid()) {
        LOG_ERROR() << "Invalid producer";
        m_undoHelper.recordAfterState();
        return;
    }
    // 首次执行时，从 Producer 中提取并保存 UUID
    if (m_uuids.empty()) {
        m_uuids = getProducerUuids(producer);
    }
    // 如果追加的是一个播放列表（多个片段）
    if (producer->type() == mlt_service_playlist_type) {
        Mlt::Playlist playlist(*producer);
        int count = playlist.count();
        for (int i = 0; i < count; i++) {
            longTask.reportProgress(QObject::tr("Appending"), i, count);
            QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(i));
            Mlt::Producer clip = Mlt::Producer(info->producer);
            if (!m_skipProxy)
                ProxyManager::generateIfNotExists(clip); // 生成代理文件（如果需要）
            clip.set_in_and_out(info->frame_in, info->frame_out); // 设置入点和出点
            MLT.setUuid(clip.parent(), m_uuids[i]); // 恢复 UUID
            bool lastClip = i == (count - 1);
            m_model.appendClip(m_trackIndex, clip, false, lastClip); // 调用模型追加片段
        }
    } else { // 如果追加的是单个片段
        MLT.setUuid(*producer, m_uuids[0]);
        if (!m_skipProxy)
            ProxyManager::generateIfNotExists(*producer);
        m_model.appendClip(m_trackIndex, *producer, m_seek);
    }
    longTask.reportProgress(QObject::tr("Finishing"), 0, 0);
    delete producer; // 释放内存
    m_undoHelper.recordAfterState(); // 记录操作后的状态
}

void AppendCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex;
    m_undoHelper.undoChanges(); // 使用 UndoHelper 恢复到操作前的状态
}

/**
 * @class InsertCommand
 * @brief “插入到轨道”命令
 * 将一个或多个片段插入到指定轨道的指定位置，并将该位置之后的片段向后推（波纹编辑）。
 */
InsertCommand::InsertCommand(MultitrackModel &model,
                             MarkersModel &markersModel,
                             int trackIndex,
                             int position,
                             const QString &xml,
                             bool seek,
                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_markersModel(markersModel)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(m_model)
    , m_seek(seek)
    , m_rippleAllTracks(Settings.timelineRippleAllTracks()) // 是否波纹所有轨道
    , m_rippleMarkers(Settings.timelineRippleMarkers())     // 是否波纹标记
    , m_markersShift(0) // 记录标记移动的量
{
    setText(QObject::tr("Insert into track"));
    m_undoHelper.setHints(UndoHelper::RestoreTracks); // 提示 UndoHelper 需要恢复轨道结构
}

void InsertCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "position" << m_position;
    int shift = 0; // 记录插入的总时长，用于移动标记
    m_undoHelper.recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    if (!clip.is_valid()) {
        LOG_ERROR() << "Invalid producer";
        m_undoHelper.recordAfterState();
        return;
    }
    if (m_uuids.empty()) {
        m_uuids = getProducerUuids(&clip);
    }
    // 如果插入的是一个播放列表（多个片段）
    if (clip.type() == mlt_service_playlist_type) {
        LongUiTask longTask(QObject::tr("Add Files"));
        Mlt::Playlist playlist(clip);
        int n = playlist.count();
        int i = n;
        // 从后往前插入，这样位置计算更简单
        while (i--) {
            QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(i));
            clip = Mlt::Producer(info->producer);
            longTask.reportProgress(QFileInfo(ProxyManager::resource(clip)).fileName(),
                                    n - i - 1,
                                    n);
            ProxyManager::generateIfNotExists(clip);
            clip.set_in_and_out(info->frame_in, info->frame_out);
            MLT.setUuid(clip.parent(), m_uuids[i]);
            bool lastClip = i == 0;
            m_model.insertClip(m_trackIndex, clip, m_position, m_rippleAllTracks, false, lastClip);
            shift += info->frame_count; // 累加移动的时长
        }
    } else { // 插入单个片段
        shift = clip.get_playtime();
        MLT.setUuid(clip, m_uuids[0]);
        ProxyManager::generateIfNotExists(clip);
        m_model.insertClip(m_trackIndex, clip, m_position, m_rippleAllTracks, m_seek);
    }
    m_undoHelper.recordAfterState();
    // 如果启用了标记波纹，则移动插入点之后的所有标记
    if (m_rippleMarkers && shift > 0) {
        m_markersShift = shift;
        m_markersModel.doShift(m_position, m_markersShift);
    }
}

void InsertCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "position" << m_position;
    m_undoHelper.undoChanges();
    // 撤销时，反向移动标记
    if (m_rippleMarkers && m_markersShift > 0) {
        m_markersModel.doShift(m_position + m_markersShift, -m_markersShift);
    }
}

/**
 * @class OverwriteCommand
 * @brief “覆盖到轨道”命令
 * 将一个或多个片段放置到指定轨道的指定位置，覆盖该时间段内原有的所有内容。
 */
OverwriteCommand::OverwriteCommand(MultitrackModel &model,
                                   int trackIndex,
                                   int position,
                                   const QString &xml,
                                   bool seek,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(m_model)
    , m_seek(seek)
{
    setText(QObject::tr("Overwrite onto track"));
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
}

void OverwriteCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "position" << m_position;
    m_undoHelper.recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    if (m_uuids.empty()) {
        m_uuids = getProducerUuids(&clip);
    }
    // 如果覆盖的是一个播放列表（多个片段）
    if (clip.type() == mlt_service_playlist_type) {
        LongUiTask longTask(QObject::tr("Add Files"));
        Mlt::Playlist playlist(clip);
        int position = m_position;
        int n = playlist.count();
        // 按顺序覆盖
        for (int i = 0; i < n; i++) {
            QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(i));
            clip = Mlt::Producer(info->producer);
            longTask.reportProgress(QFileInfo(ProxyManager::resource(clip)).fileName(), i, n);
            ProxyManager::generateIfNotExists(clip);
            clip.set_in_and_out(info->frame_in, info->frame_out);
            MLT.setUuid(clip.parent(), m_uuids[i]);
            bool lastClip = i == (n - 1);
            m_model.overwrite(m_trackIndex, clip, position, false, lastClip);
            position += info->frame_count; // 更新下一个片段的覆盖位置
        }
    } else { // 覆盖单个片段
        MLT.setUuid(clip, m_uuids[0]);
        ProxyManager::generateIfNotExists(clip);
        m_model.overwrite(m_trackIndex, clip, m_position, m_seek);
    }
    m_undoHelper.recordAfterState();
}

void OverwriteCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "position" << m_position;
    m_undoHelper.undoChanges(); // 恢复被覆盖的片段
}

/**
 * @class LiftCommand
 * @brief “从轨道提升”命令
 * 从轨道中移除一个片段，但在其位置留下空白（不移动后续片段）。
 */
LiftCommand::LiftCommand(MultitrackModel &model, int trackIndex, int clipIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Lift from track"));
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
}

void LiftCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    m_undoHelper.recordBeforeState();
    m_model.liftClip(m_trackIndex, m_clipIndex); // 调用模型移除片段
    m_undoHelper.recordAfterState();
}

void LiftCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    m_undoHelper.undoChanges(); // 恢复被提升的片段
}

/**
 * @class RemoveCommand
 * @brief “从轨道移除”命令
 * 从轨道中移除一个片段，并填补其留下的空白（波纹删除）。
 */
RemoveCommand::RemoveCommand(MultitrackModel &model,
                             MarkersModel &markersModel,
                             int trackIndex,
                             int clipIndex,
                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_markersModel(markersModel)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_undoHelper(m_model)
    , m_rippleAllTracks(Settings.timelineRippleAllTracks())
    , m_rippleMarkers(Settings.timelineRippleMarkers())
    , m_markerRemoveStart(-1) // 记录被移除片段的起始位置
    , m_markerRemoveEnd(-1)   // 记录被移除片段的结束位置
{
    setText(QObject::tr("Remove from track"));
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
}

void RemoveCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;

    if (m_rippleMarkers) {
        // 如果启用了标记波纹，需要处理标记的移除和移动
        bool markersModified = false;
        m_markers = m_markersModel.getMarkers(); // 保存所有标记的当前状态
        if (m_markers.size() > 0) {
            auto mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(mlt_index));
            if (track && track->is_valid()) {
                Mlt::Playlist playlist(*track);
                // 获取被移除片段的时间范围
                m_markerRemoveStart = playlist.clip_start(m_clipIndex);
                m_markerRemoveEnd = m_markerRemoveStart + playlist.clip_length(m_clipIndex) - 1;
            }
        }
        if (m_markers.size() > 0 && m_markerRemoveStart >= 0) {
            QList<Markers::Marker> newMarkers = m_markers;
            for (int i = 0; i < newMarkers.size(); i++) {
                Markers::Marker &marker = newMarkers[i];
                if (marker.start >= m_markerRemoveStart && marker.start <= m_markerRemoveEnd) {
                    // 如果标记在被移除的片段范围内，则删除该标记
                    newMarkers.removeAt(i);
                    i--;
                    markersModified = true;
                } else if (marker.start > m_markerRemoveEnd) {
                    // 如果标记在被移除的片段之后，则向前移动
                    marker.start -= (m_markerRemoveEnd - m_markerRemoveStart + 1);
                    marker.end -= (m_markerRemoveEnd - m_markerRemoveStart + 1);
                    markersModified = true;
                }
            }
            if (markersModified) {
                m_markersModel.doReplace(newMarkers); // 应用新的标记列表
            }
        }
        if (!markersModified) {
            // 如果没有标记被修改，则清除记录
            m_markerRemoveStart = -1;
            m_markers.clear();
        }
    }

    m_undoHelper.recordBeforeState();
    m_model.removeClip(m_trackIndex, m_clipIndex, m_rippleAllTracks); // 调用模型移除片段
    m_undoHelper.recordAfterState();
}

void RemoveCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    m_undoHelper.undoChanges(); // 恢复被移除的片段
    // 撤销时，恢复原始的标记列表
    if (m_rippleMarkers && m_markerRemoveStart >= 0) {
        m_markersModel.doReplace(m_markers);
    }
}

/**
 * @class GroupCommand
 * @brief “组合”命令
 * 将多个片段组合在一起，以便它们可以作为一个整体进行操作。
 */
GroupCommand::GroupCommand(MultitrackModel &model, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
{}

/**
 * @brief 将一个片段添加到待组合的列表中。
 * @param trackIndex 片段所在的轨道索引。
 * @param clipIndex 片段的索引。
 */
void GroupCommand::addToGroup(int trackIndex, int clipIndex)
{
    auto clipInfo = m_model.getClipInfo(trackIndex, clipIndex);
    if (clipInfo && clipInfo->cut && !clipInfo->cut->is_blank()) {
        ClipPosition position(trackIndex, clipIndex);
        m_clips.append(position);
        // 如果片段已经有组，记录其旧的组号
        if (clipInfo->cut->property_exists(kShotcutGroupProperty)) {
            m_prevGroups.insert(position, clipInfo->cut->get_int(kShotcutGroupProperty));
        }
    }
}

void GroupCommand::redo()
{
    int groupNumber = getUniqueGroupNumber(m_model); // 获取一个唯一的组号
    setText(QObject::tr("Group %n clips", nullptr, m_clips.size()));
    // 遍历所有待组合的片段，设置新的组号
    for (auto &clip : m_clips) {
        auto clipInfo = m_model.getClipInfo(clip.trackIndex, clip.clipIndex);
        if (clipInfo && clipInfo->cut) {
            clipInfo->cut->set(kShotcutGroupProperty, groupNumber);
            // 通知模型数据已更改，以便更新 UI
            QModelIndex modelIndex = m_model.index(clip.clipIndex,
                                                   0,
                                                   m_model.index(clip.trackIndex));
            emit m_model.dataChanged(modelIndex,
                                     modelIndex,
                                     QVector<int>() << MultitrackModel::GroupRole);
        }
    }
}

void GroupCommand::undo()
{
    // 恢复每个片段的原始组状态（或取消组合）
    for (auto &clip : m_clips) {
        auto clipInfo = m_model.getClipInfo(clip.trackIndex, clip.clipIndex);
        if (clipInfo && clipInfo->cut) {
            if (m_prevGroups.contains(clip)) {
                // 如果之前有组，恢复它
                clipInfo->cut->set(kShotcutGroupProperty, m_prevGroups[clip]);
            } else {
                // 如果之前没有组，清除组属性
                clipInfo->cut->Mlt::Properties::clear(kShotcutGroupProperty);
            }
            QModelIndex modelIndex = m_model.index(clip.clipIndex,
                                                   0,
                                                   m_model.index(clip.trackIndex));
            emit m_model.dataChanged(modelIndex,
                                     modelIndex,
                                     QVector<int>() << MultitrackModel::GroupRole);
        }
    }
}

/**
 * @class UngroupCommand
 * @brief “取消组合”命令
 * 将一个或多个片段从它们的组中移除。
 */
UngroupCommand::UngroupCommand(MultitrackModel &model, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
{}

/**
 * @brief 将一个片段添加到待取消组合的列表中。
 * @param trackIndex 片段所在的轨道索引。
 * @param clipIndex 片段的索引。
 */
void UngroupCommand::removeFromGroup(int trackIndex, int clipIndex)
{
    auto clipInfo = m_model.getClipInfo(trackIndex, clipIndex);
    if (clipInfo && clipInfo->cut) {
        ClipPosition position(trackIndex, clipIndex);
        // 如果片段有组，记录其组号
        if (clipInfo->cut->property_exists(kShotcutGroupProperty)) {
            m_prevGroups.insert(position, clipInfo->cut->get_int(kShotcutGroupProperty));
        }
    }
}

void UngroupCommand::redo()
{
    setText(QObject::tr("Ungroup %n clips", nullptr, m_prevGroups.size()));
    // 遍历所有待取消组合的片段，清除其组属性
    for (auto &clip : m_prevGroups.keys()) {
        auto clipInfo = m_model.getClipInfo(clip.trackIndex, clip.clipIndex);
        if (clipInfo && clipInfo->cut) {
            clipInfo->cut->Mlt::Properties::clear(kShotcutGroupProperty);
            QModelIndex modelIndex = m_model.index(clip.clipIndex,
                                                   0,
                                                   m_model.index(clip.trackIndex));
            emit m_model.dataChanged(modelIndex,
                                     modelIndex,
                                     QVector<int>() << MultitrackModel::GroupRole);
        }
    }
}

/**
 * @class MoveClipCommand
 * @brief “移动片段”命令
 * 一个复杂的命令，用于在时间线上移动一个或多个片段，可以跨轨道移动，并支持波纹模式。
 */
// (MoveClipCommand 的构造函数和 redo 方法已在之前的代码片段中)

void MoveClipCommand::undo()
{
    LOG_DEBUG() << "track delta" << m_trackDelta;
    m_undoHelper.undoChanges(); // 使用 UndoHelper 恢复
    if (m_rippleMarkers && m_markers.size() >= 0) {
        m_markersModel.doReplace(m_markers); // 恢复标记
    }
    // 撤销后，选择原始位置的片段
    QList<QPoint> selection;
    for (auto &clip : m_clips) {
        selection << QPoint(clip.clipIndex, clip.trackIndex);
    }
    m_timeline.setSelection(selection);
}

/**
 * @brief 尝试将此命令与另一个 MoveClipCommand 合并。
 * 这用于连续的微小移动（如按住鼠标拖动），将它们合并为一个撤销步骤。
 */
bool MoveClipCommand::mergeWith(const QUndoCommand *other)
{
    const MoveClipCommand *that = static_cast<const MoveClipCommand *>(other);
    LOG_DEBUG() << "this delta" << m_positionDelta << "that delta" << that->m_positionDelta;
    // 检查各种条件，确保两个命令是兼容的（移动的片段相同、模式相同等）
    if (that->id() != id() || that->m_clips.size() != m_clips.size() || that->m_ripple != m_ripple
        || that->m_rippleAllTracks != m_rippleAllTracks || that->m_rippleMarkers != m_rippleMarkers)
        return false;
    if (that->m_undoHelper.affectedTracks() != m_undoHelper.affectedTracks()) {
        return false;
    }
    if (that->m_trackDelta || m_trackDelta) {
        // 如果移动涉及跨轨道，则不合并。
        // 特别是，如果一个片段被移动到另一个轨道然后又移回来，
        // 最终没有变化，但一个撤销命令会变得无效。
        return false;
    }
    // 检查移动的片段是否完全相同
    auto thisIterator = m_clips.begin();
    auto thatIterator = that->m_clips.begin();
    while (thisIterator != m_clips.end() && thatIterator != that->m_clips.end()) {
        if (thisIterator.value().uuid != thatIterator.value().uuid)
            return false;
        thisIterator++;
        thatIterator++;
    }
    // 累加位置偏移量
    m_positionDelta += that->m_positionDelta;
    m_undoHelper.recordAfterState();
    return true;
}

/**
 * @brief 处理移动操作对标记的影响。
 */
void MoveClipCommand::redoMarkers()
{
    if (m_rippleMarkers) {
        if (m_markers.size() == 0) {
            m_markers = m_markersModel.getMarkers(); // 首次执行时保存所有标记
        }
        QList<Markers::Marker> newMarkers = m_markers;
        bool markersModified = false;
        for (int i = 0; i < newMarkers.size(); i++) {
            Markers::Marker &marker = newMarkers[i];
            if (marker.start < m_earliestStart
                && marker.start > (m_earliestStart + m_positionDelta)) {
                // 如果标记在被覆盖的范围内，则删除它
                newMarkers.removeAt(i);
                i--;
                markersModified = true;
            } else if (marker.start >= m_earliestStart) {
                // 如果标记在被移动片段的起始位置之后，则随其一起移动
                marker.start += m_positionDelta;
                marker.end += m_positionDelta;
                markersModified = true;
            }
        }
        if (markersModified) {
            m_markersModel.doReplace(newMarkers);
        } else {
            m_markers.clear(); // 如果没有标记被修改，清除保存的数据
        }
    }
}

/**
 * @class TrimClipInCommand
 * @brief “修剪片段入点”命令
 * 调整片段的入点，可以波纹删除或覆盖。
 */
// (TrimClipInCommand 的构造函数已在之前的代码片段中)

void TrimClipInCommand::redo()
{
    // 暂停过滤器的撤销跟踪，避免修剪操作产生的内部修改被记录
    MAIN.filterController()->pauseUndoTracking();

    if (m_rippleMarkers) {
        // 处理波纹修剪对标记的影响
        // ... (逻辑与 Remove 命令类似，删除或移动标记)
    }

    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta"
                    << m_delta;
        // 首次执行时，创建并记录 UndoHelper
        m_undoHelper.reset(new UndoHelper(m_model));
        if (m_ripple) {
            m_undoHelper->setHints(UndoHelper::SkipXML); // 波纹时可以跳过 XML 保存以提高性能
        } else {
            m_undoHelper->setHints(UndoHelper::RestoreTracks);
        }
        m_undoHelper->recordBeforeState();
        m_model.trimClipIn(m_trackIndex, m_clipIndex, m_delta, m_ripple, m_rippleAllTracks);
        m_undoHelper->recordAfterState();
    } else {
        // 如果不是首次执行（即由 undo 触发的 redo），则直接使用已有的 UndoHelper
        Q_ASSERT(m_undoHelper);
        m_undoHelper->recordAfterState();
        m_redo = true;
    }
    // 恢复过滤器的撤销跟踪
    MAIN.filterController()->resumeUndoTracking();
}

void TrimClipInCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
    Q_ASSERT(m_undoHelper);
    MAIN.filterController()->pauseUndoTracking();
    m_undoHelper->undoChanges(); // 恢复到修剪前的状态
    if (m_rippleMarkers && m_markerRemoveStart >= 0) {
        m_markersModel.doReplace(m_markers); // 恢复标记
    }
    MAIN.filterController()->resumeUndoTracking();
}

bool TrimClipInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipInCommand *that = static_cast<const TrimClipInCommand *>(other);
    LOG_DEBUG() << "this clipIndex" << m_clipIndex << "that clipIndex" << that->m_clipIndex;
    // 检查两个命令是否作用于同一个片段且模式相同
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex
        || that->m_ripple != m_ripple || that->m_rippleAllTracks != m_rippleAllTracks
        || that->m_rippleMarkers != m_rippleMarkers)
        return false;
    Q_ASSERT(m_undoHelper);
    m_undoHelper->recordAfterState();
    // 累加修剪的量
    m_delta += static_cast<const TrimClipInCommand *>(other)->m_delta;
    return true;
}

/**
 * @class TrimClipOutCommand
 * @brief “修剪片段出点”命令
 * 调整片段的出点，可以波纹删除或覆盖。
 */
// (TrimClipOutCommand 的构造函数已在之前的代码片段中)

void TrimClipOutCommand::redo()
{
    MAIN.filterController()->pauseUndoTracking();
    if (m_rippleMarkers) {
        // 处理波纹修剪对标记的影响
        // ... (逻辑与 TrimClipInCommand 类似，但计算被移除部分的位置不同)
    }

    if (m_redo) {
        // 首次执行时，创建并记录 UndoHelper
        m_undoHelper.reset(new UndoHelper(m_model));
        if (!m_ripple)
            m_undoHelper->setHints(UndoHelper::SkipXML); // 非波纹时可以跳过 XML 保存
        m_undoHelper->recordBeforeState();
        // 调用模型执行修剪，并更新可能变化的片段索引
        m_clipIndex
            = m_model.trimClipOut(m_trackIndex, m_clipIndex, m_delta, m_ripple, m_rippleAllTracks);
        m_undoHelper->recordAfterState();
    } else {
        // 如果不是首次执行，则直接使用已有的 UndoHelper
        Q_ASSERT(m_undoHelper);
        m_undoHelper->recordAfterState();
        m_redo = true;
    }
    MAIN.filterController()->resumeUndoTracking();
}

void TrimClipOutCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
    Q_ASSERT(m_undoHelper);
    MAIN.filterController()->pauseUndoTracking();
    m_undoHelper->undoChanges();
    if (m_rippleMarkers && m_markerRemoveStart >= 0) {
        m_markersModel.doReplace(m_markers);
    }
    MAIN.filterController()->resumeUndoTracking();
}

bool TrimClipOutCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipOutCommand *that = static_cast<const TrimClipOutCommand *>(other);
    // 检查合并条件
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex
        || that->m_ripple != m_ripple || that->m_rippleAllTracks != m_rippleAllTracks
        || that->m_rippleMarkers != m_rippleMarkers)
        return false;
    Q_ASSERT(m_undoHelper);
    m_undoHelper->recordAfterState();
    // 累加修剪的量
    m_delta += static_cast<const TrimClipOutCommand *>(other)->m_delta;
    return true;
}

/**
 * @class SplitCommand
 * @brief “分割片段”命令
 * 在指定位置将一个或多个片段分割成两个。
 */
// (SplitCommand 的构造函数已在之前的代码片段中)

void SplitCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex[0] << "clipIndex" << m_clipIndex[0] << "position"
                << m_position;
    MAIN.filterController()->pauseUndoTracking();
    m_undoHelper.recordBeforeState();
    // 遍历所有待分割的片段并执行分割
    for (int i = 0; i < m_trackIndex.size(); i++) {
        m_model.splitClip(m_trackIndex[i], m_clipIndex[i], m_position);
    }
    m_undoHelper.recordAfterState();
    MAIN.filterController()->resumeUndoTracking();
}

void SplitCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex[0] << "clipIndex" << m_clipIndex[0] << "position"
                << m_position;
    MAIN.filterController()->pauseUndoTracking();
    m_undoHelper.undoChanges(); // 恢复分割前的片段
    MAIN.filterController()->resumeUndoTracking();
}

/**
 * @class FadeInCommand
 * @brief “调整淡入”命令
 * 为片段添加或调整淡入效果。
 */
// (FadeInCommand 的构造函数已在之前的代码片段中)

void FadeInCommand::redo()
{
    m_model.fadeIn(m_trackIndex, m_clipIndex, m_duration); // 应用新的淡入时长
}

void FadeInCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "duration"
                << m_duration;
    m_model.fadeIn(m_trackIndex, m_clipIndex, m_previous); // 恢复旧的淡入时长
}

bool FadeInCommand::mergeWith(const QUndoCommand *other)
{
    const FadeInCommand *that = static_cast<const FadeInCommand *>(other);
    // 检查是否为同一片段的淡入操作
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex
        || (!that->m_duration && m_duration != that->m_duration))
        return false;
    // 更新为最新的淡入时长
    m_duration = static_cast<const FadeInCommand *>(other)->m_duration;
    return true;
}

/**
 * @class FadeOutCommand
 * @brief “调整淡出”命令
 * 为片段添加或调整淡出效果。
 */
// (FadeOutCommand 的构造函数已在之前的代码片段中)

void FadeOutCommand::redo()
{
    m_model.fadeOut(m_trackIndex, m_clipIndex, m_duration); // 应用新的淡出时长
}

void FadeOutCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "duration"
                << m_duration;
    m_model.fadeOut(m_trackIndex, m_clipIndex, m_previous); // 恢复旧的淡出时长
}

bool FadeOutCommand::mergeWith(const QUndoCommand *other)
{
    const FadeOutCommand *that = static_cast<const FadeOutCommand *>(other);
    // 检查是否为同一片段的淡出操作
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex
        || (!that->m_duration && m_duration != that->m_duration))
        return false;
    // 更新为最新的淡出时长
    m_duration = static_cast<const FadeOutCommand *>(other)->m_duration;
    return true;
}

/**
 * @class AddTransitionCommand
 * @brief “添加转场”命令
 * 在两个片段之间添加一个转场效果。
 */
// (AddTransitionCommand 的构造函数已在之前的代码片段中)

void AddTransitionCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position"
                << m_position;

    if (m_rippleMarkers) {
        // 在移动任何东西之前计算标记的偏移量
        auto mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(mlt_index));
        if (track && track->is_valid()) {
            Mlt::Playlist playlist(*track);
            m_markerOldStart = playlist.clip_start(m_clipIndex); // 记录片段原始起始位置
            m_markerNewStart = m_position;                       // 记录转场的新起始位置
        }
    }

    m_undoHelper.recordBeforeState();
    // 调用模型添加转场，并获取转场的索引
    m_transitionIndex
        = m_model.addTransition(m_trackIndex, m_clipIndex, m_position, m_ripple, m_rippleAllTracks);
    LOG_DEBUG() << "m_transitionIndex" << m_transitionIndex;
    m_undoHelper.recordAfterState();

    // 根据需要删除或移动标记
    bool markersModified = false;
    if (m_transitionIndex >= 0 && m_rippleMarkers && m_markerOldStart >= 0) {
        m_markers = m_markersModel.getMarkers();
        QList<Markers::Marker> newMarkers = m_markers;
        int startDelta = m_markerNewStart - m_markerOldStart;
        for (int i = 0; i < newMarkers.size(); i++) {
            Markers::Marker &marker = newMarkers[i];
            if (marker.start <= m_markerOldStart && marker.start > m_markerNewStart) {
                // 如果标记在被覆盖的范围内，则删除
                newMarkers.removeAt(i);
                i--;
                markersModified = true;
            } else if (marker.start >= m_markerOldStart) {
                // 如果标记在被修改的片段之后，则移动它
                marker.start += startDelta;
                marker.end += startDelta;
                markersModified = true;
            }
        }
        if (markersModified) {
            m_markersModel.doReplace(newMarkers);
        }
    }
    if (!markersModified) {
        m_markerOldStart = -1;
        m_markers.clear();
    }
}
// 注意：AddTransitionCommand 的 undo() 方法未在提供的代码片段中显示，
// 但根据模式，它会调用 m_undoHelper.undoChanges() 并恢复标记。


/**
 * @class AddTransitionCommand
 * @brief “添加转场”命令
 * 在两个片段之间添加一个转场效果。
 */
// (AddTransitionCommand 的构造函数和 redo 方法已在之前的代码片段中)

void AddTransitionCommand::undo()
{
    if (m_transitionIndex >= 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position"
                    << m_position;
        m_undoHelper.undoChanges(); // 移除转场，恢复片段
        // 恢复选择到原来的片段上
        m_timeline.setSelection(QList<QPoint>() << QPoint(m_clipIndex, m_trackIndex));

        if (m_rippleMarkers && m_markerOldStart >= 0) {
            m_markersModel.doReplace(m_markers); // 恢复标记
        }
    }
}

/**
 * @class TrimTransitionInCommand
 * @brief “修剪转场入点”命令
 * 调整转场效果的开始时间。
 */
// (TrimTransitionInCommand 的构造函数已在之前的代码片段中)

void TrimTransitionInCommand::redo()
{
    if (m_redo) {
        MAIN.filterController()->pauseUndoTracking();
        m_model.trimTransitionIn(m_trackIndex, m_clipIndex, m_delta);
        // 在某些情况下需要通知模型更新片段的入点信息
        if (m_notify && m_clipIndex >= 0)
            m_model.notifyClipIn(m_trackIndex, m_clipIndex);
        MAIN.filterController()->resumeUndoTracking();
    } else {
        m_redo = true;
    }
}

void TrimTransitionInCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta" << m_delta;
    if (m_clipIndex >= 0) {
        MAIN.filterController()->pauseUndoTracking();
        // 执行反向操作
        m_model.trimTransitionIn(m_trackIndex, m_clipIndex, -m_delta);
        m_model.notifyClipIn(m_trackIndex, m_clipIndex);
        m_notify = true; // 标记下次 redo 需要通知
        MAIN.filterController()->resumeUndoTracking();
    } else
        LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

bool TrimTransitionInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimTransitionInCommand *that = static_cast<const TrimTransitionInCommand *>(other);
    // 检查是否为同一转场的修剪操作
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    // 累加修剪的量
    m_delta += static_cast<const TrimTransitionInCommand *>(other)->m_delta;
    return true;
}

/**
 * @class TrimTransitionOutCommand
 * @brief “修剪转场出点”命令
 * 调整转场效果的结束时间。
 */
// (TrimTransitionOutCommand 的构造函数已在之前的代码片段中)

void TrimTransitionOutCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
        MAIN.filterController()->pauseUndoTracking();
        m_model.trimTransitionOut(m_trackIndex, m_clipIndex, m_delta);
        if (m_notify && m_clipIndex >= 0)
            m_model.notifyClipOut(m_trackIndex, m_clipIndex);
        MAIN.filterController()->resumeUndoTracking();
    } else {
        m_redo = true;
    }
}

void TrimTransitionOutCommand::undo()
{
    if (m_clipIndex >= 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta"
                    << m_delta;
        MAIN.filterController()->pauseUndoTracking();
        // 执行反向操作
        m_model.trimTransitionOut(m_trackIndex, m_clipIndex, -m_delta);
        m_model.notifyClipOut(m_trackIndex, m_clipIndex);
        m_notify = true;
        MAIN.filterController()->resumeUndoTracking();
    } else
        LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

bool TrimTransitionOutCommand::mergeWith(const QUndoCommand *other)
{
    const TrimTransitionOutCommand *that = static_cast<const TrimTransitionOutCommand *>(other);
    // 检查是否为同一转场的修剪操作
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    // 累加修剪的量
    m_delta += static_cast<const TrimTransitionOutCommand *>(other)->m_delta;
    return true;
}

/**
 * @class AddTransitionByTrimInCommand
 * @brief “通过修剪入点添加转场”命令
 * 一种特殊的添加转场方式，通过修剪前一个片段的出点来创建转场。
 */
// (AddTransitionByTrimInCommand 的构造函数已在之前的代码片段中)

void AddTransitionByTrimInCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta"
                    << m_trimDelta << "duration" << m_duration;
        // 如果有修剪量，先修剪后一个片段的入点
        if (m_trimDelta)
            m_timeline.model()->trimClipIn(m_trackIndex, m_clipIndex + 1, m_trimDelta, false, false);
        // 添加转场
        m_timeline.model()->addTransitionByTrimIn(m_trackIndex, m_clipIndex, m_duration);
        if (m_notify && m_clipIndex > 0)
            m_timeline.model()->notifyClipOut(m_trackIndex, m_clipIndex - 1);
        // 选择后一个片段
        m_timeline.setSelection(QList<QPoint>() << QPoint(m_clipIndex + 1, m_trackIndex));
    } else {
        m_redo = true;
    }
}

void AddTransitionByTrimInCommand::undo()
{
    if (m_clipIndex > 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta"
                    << m_trimDelta;
        // 调用模型移除通过修剪创建的转场，并恢复被修剪的片段
        m_timeline.model()->removeTransitionByTrimIn(m_trackIndex, m_clipIndex, -m_trimDelta);
        m_notify = true;
        // 恢复选择到原来的片段上
        m_timeline.setSelection(QList<QPoint>() << QPoint(m_clipIndex, m_trackIndex));
    } else
        LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

bool AddTransitionByTrimInCommand::mergeWith(const QUndoCommand *other)
{
    const AddTransitionByTrimInCommand *that = static_cast<const AddTransitionByTrimInCommand *>(
        other);
    // 检查是否为同一位置的连续操作
    if (that->id() != id() || that->m_trackIndex != m_trackIndex
        || (that->m_clipIndex != m_clipIndex && m_clipIndex != that->m_clipIndex - 1))
        return false;
    return true;
}

/**
 * @class RemoveTransitionByTrimInCommand
 * @brief “通过修剪入点移除转场”命令
 * 移除一个转场，并恢复前一个片段的出点。
 */
// (RemoveTransitionByTrimInCommand 的构造函数已在之前的代码片段中)

void RemoveTransitionByTrimInCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
        QModelIndex modelIndex = m_model.makeIndex(m_trackIndex, m_clipIndex);
        int n = m_model.data(modelIndex, MultitrackModel::DurationRole).toInt(); // 获取转场时长
        m_model.liftClip(m_trackIndex, m_clipIndex); // 提升（移除）转场片段
        // 将后一个片段的入点向前移动，以填补空缺
        m_model.trimClipIn(m_trackIndex, m_clipIndex + 1, -n, false, false);
        m_model.notifyClipIn(m_trackIndex, m_clipIndex + 1);
    } else {
        m_redo = true;
    }
}

void RemoveTransitionByTrimInCommand::undo()
{
    if (m_clipIndex > 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta"
                    << m_delta;
        // 通过修剪后一个片段的出点来重新添加转场
        m_model.addTransitionByTrimOut(m_trackIndex, m_clipIndex - 1, m_delta);
        // 从保存的 XML 中恢复转场的属性
        auto clipInfo = m_model.getClipInfo(m_trackIndex, m_clipIndex);
        Mlt::Service oldService = Mlt::Producer(MLT.profile(),
                                                "xml-string",
                                                m_xml.toUtf8().constData());
        while (oldService.is_valid()) {
            if (oldService.type() == mlt_service_transition_type) {
                Mlt::Service newService(clipInfo->producer);
                while (newService.is_valid()) {
                    // 找到对应类型的转场服务
                    if (newService.type() == mlt_service_transition_type
                        && QString(oldService.get("mlt_service"))
                               == QString(newService.get("mlt_service"))) {
                        newService.inherit(oldService); // 继承旧转场的所有属性
                        break;
                    }
                    Mlt::Service *tmpNewService = newService.producer();
                    newService = Mlt::Service(*tmpNewService);
                    delete tmpNewService;
                }
            }
            Mlt::Service *tmpOldService = oldService.producer();
            oldService = Mlt::Service(*tmpOldService);
            delete tmpOldService;
        }
        m_model.notifyClipIn(m_trackIndex, m_clipIndex + 1);
    } else
        LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

/**
 * @class RemoveTransitionByTrimOutCommand
 * @brief “通过修剪出点移除转场”命令
 * 移除一个转场，并恢复后一个片段的入点。
 */
// (RemoveTransitionByTrimOutCommand 的构造函数已在之前的代码片段中)

void RemoveTransitionByTrimOutCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
        QModelIndex modelIndex = m_model.makeIndex(m_trackIndex, m_clipIndex);
        int n = m_model.data(modelIndex, MultitrackModel::DurationRole).toInt();
        m_model.liftClip(m_trackIndex, m_clipIndex); // 提升（移除）转场片段
        // 将前一个片段的出点向后移动，以填补空缺
        m_model.trimClipOut(m_trackIndex, m_clipIndex - 1, -n, false, false);
        m_model.notifyClipOut(m_trackIndex, m_clipIndex - 1);
    } else {
        m_redo = true;
    }
}

void RemoveTransitionByTrimOutCommand::undo()
{
    if (m_clipIndex > 0) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta"
                    << m_delta;
        // 通过修剪前一个片段的入点来重新添加转场
        m_model.addTransitionByTrimIn(m_trackIndex, m_clipIndex, m_delta);
        // 从保存的 XML 中恢复转场的属性
        auto clipInfo = m_model.getClipInfo(m_trackIndex, m_clipIndex);
        Mlt::Service oldService = Mlt::Producer(MLT.profile(),
                                                "xml-string",
                                                m_xml.toUtf8().constData());
        while (oldService.is_valid()) {
            if (oldService.type() == mlt_service_transition_type) {
                Mlt::Service newService(clipInfo->producer);
                while (newService.is_valid()) {
                    if (newService.type() == mlt_service_transition_type
                        && QString(oldService.get("mlt_service"))
                               == QString(newService.get("mlt_service"))) {
                        newService.inherit(oldService);
                        break;
                    }
                    Mlt::Service *tmpNewService = newService.producer();
                    newService = Mlt::Service(*tmpNewService);
                    delete tmpNewService;
                }
            }
            Mlt::Service *tmpOldService = oldService.producer();
            oldService = Mlt::Service(*tmpOldService);
            delete tmpOldService;
        }
        m_model.notifyClipOut(m_trackIndex, m_clipIndex - 1);
    } else
        LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

/**
 * @class AddTransitionByTrimOutCommand
 * @brief “通过修剪出点添加转场”命令
 * 一种特殊的添加转场方式，通过修剪后一个片段的入点来创建转场。
 */
// (AddTransitionByTrimOutCommand 的构造函数已在之前的代码片段中)

void AddTransitionByTrimOutCommand::redo()
{
    if (m_redo) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta"
                    << m_trimDelta << "duration" << m_duration;
        // 如果有修剪量，先修剪前一个片段的出点
        if (m_trimDelta)
            m_model.trimClipOut(m_trackIndex, m_clipIndex, m_trimDelta, false, false);
        // 添加转场
        m_model.addTransitionByTrimOut(m_trackIndex, m_clipIndex, m_duration);
        if (m_notify)
            m_model.notifyClipIn(m_trackIndex, m_clipIndex + 2);
    } else {
        m_redo = true;
    }
}

void AddTransitionByTrimOutCommand::undo()
{
    if (m_clipIndex + 2 < m_model.rowCount(m_model.index(m_trackIndex))) {
        LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "delta"
                    << m_trimDelta;
        // 调用模型移除通过修剪创建的转场，并恢复被修剪的片段
        m_model.removeTransitionByTrimOut(m_trackIndex, m_clipIndex, -m_trimDelta);
        m_notify = true;
    } else
        LOG_WARNING() << "invalid clip index" << m_clipIndex;
}

bool AddTransitionByTrimOutCommand::mergeWith(const QUndoCommand *other)
{
    const AddTransitionByTrimOutCommand *that = static_cast<const AddTransitionByTrimOutCommand *>(
        other);
    // 检查是否为同一位置的连续操作
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    return true;
}

/// ####################################################################
/// #                        轨道管理命令                              #
/// ####################################################################

/**
 * @class AddTrackCommand
 * @brief “添加轨道”命令
 * 在时间线末尾添加一个新的视频或音频轨道。
 */
// (AddTrackCommand 的构造函数和 redo 方法已在之前的代码片段中)

void AddTrackCommand::undo()
{
    LOG_DEBUG() << (m_isVideo ? "video" : "audio") << m_uuid;
    m_model.removeTrack(m_trackIndex); // 移除轨道
}

/**
 * @class InsertTrackCommand
 * @brief “插入轨道”命令
 * 在指定位置插入一个新的视频或音频轨道。
 */
InsertTrackCommand::InsertTrackCommand(MultitrackModel &model,
                                       int trackIndex,
                                       TrackType trackType,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_trackType(trackType)
{
    // 确保轨道类型有效
    if (trackType != AudioTrackType && trackType != VideoTrackType) {
        m_trackType = model.trackList().size() > 0 ? model.trackList().at(m_trackIndex).type
                                                   : VideoTrackType;
    }
    if (m_trackType == AudioTrackType)
        setText(QObject::tr("Insert audio track"));
    else if (m_trackType == VideoTrackType)
        setText(QObject::tr("Insert video track"));
}

void InsertTrackCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "type"
                << (m_trackType == AudioTrackType ? "audio" : "video");
    m_model.insertTrack(m_trackIndex, m_trackType); // 调用模型插入轨道
    // 为新轨道分配 UUID
    int mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
    std::unique_ptr<Mlt::Multitrack> multitrack(m_model.tractor()->multitrack());
    if (!multitrack || !multitrack->is_valid())
        return;
    std::unique_ptr<Mlt::Producer> producer(multitrack->track(mlt_index));
    if (producer && producer->is_valid()) {
        if (m_uuid.isNull()) {
            m_uuid = MLT.ensureHasUuid(*producer);
        } else {
            MLT.setUuid(*producer, m_uuid);
        }
    }
}

void InsertTrackCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "type"
                << (m_trackType == AudioTrackType ? "audio" : "video");
    m_model.removeTrack(m_trackIndex); // 移除轨道
}

/**
 * @class RemoveTrackCommand
 * @brief “移除轨道”命令
 * 从时间线中移除一个轨道，并保存其所有信息以便恢复。
 */
RemoveTrackCommand::RemoveTrackCommand(MultitrackModel &model, int trackIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_trackType(model.trackList().at(m_trackIndex).type) // 保存轨道类型
    , m_undoHelper(model)
{
    if (m_trackType == AudioTrackType)
        setText(QObject::tr("Remove audio track"));
    else if (m_trackType == VideoTrackType)
        setText(QObject::tr("Remove video track"));

    // 在构造时保存轨道的所有信息
    int mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> producer(m_model.tractor()->track(mlt_index));
    if (producer && producer->is_valid()) {
        // 保存轨道名称
        m_trackName = QString::fromUtf8(producer->get(kTrackNameProperty));
        // 保存轨道 Producer 的 UUID
        m_uuid = MLT.ensureHasUuid(*producer);
        // 保存轨道上的所有滤镜
        if (producer->filter_count() > 0) {
            m_filtersProducer.reset(new Mlt::Producer(MLT.profile(), "color"));
            if (m_filtersProducer->is_valid())
                MLT.copyFilters(*producer, *m_filtersProducer);
        }
    }
}

void RemoveTrackCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "type"
                << (m_trackType == AudioTrackType ? "audio" : "video");
    m_undoHelper.recordBeforeState();
    int mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> producer(m_model.tractor()->track(mlt_index));
    Mlt::Playlist playlist(*producer);
    // 在移除轨道前，发出信号通知模型将要移除哪些片段
    for (int i = 0; i < playlist.count(); ++i) {
        if (!playlist.is_blank(i))
            emit m_model.removing(playlist.get_clip(i));
    }
    playlist.clear(); // 清空轨道内容
    m_undoHelper.recordAfterState();
    m_model.removeTrack(m_trackIndex); // 从模型中移除轨道
}
// 注意：RemoveTrackCommand 的 undo() 方法未在提供的代码片段中显示，
// 但根据模式，它会使用保存的信息（类型、名称、UUID、滤镜）重新创建轨道。


/**
 * @class RemoveTrackCommand
 * @brief “移除轨道”命令
 * 从时间线中移除一个轨道，并保存其所有信息以便恢复。
 */
// (RemoveTrackCommand 的构造函数和 redo 方法已在之前的代码片段中)

void RemoveTrackCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "type"
                << (m_trackType == AudioTrackType ? "audio" : "video");
    // 重新插入轨道，并恢复其类型
    m_model.insertTrack(m_trackIndex, m_trackType);
    m_model.setTrackName(m_trackIndex, m_trackName); // 恢复轨道名称

    // 使用 UndoHelper 恢复轨道上的所有片段
    m_undoHelper.undoChanges();

    // 重新附加滤镜
    int mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> producer(m_model.tractor()->track(mlt_index));
    Mlt::Playlist playlist(*producer);
    if (playlist.is_valid() && m_filtersProducer && m_filtersProducer->is_valid()) {
        MLT.setUuid(playlist, m_uuid); // 恢复 UUID
        MLT.copyFilters(*m_filtersProducer, playlist); // 复制滤镜
        // 通知模型轨道的滤镜状态已更改
        QModelIndex modelIndex = m_model.index(m_trackIndex);
        emit m_model.dataChanged(modelIndex,
                                 modelIndex,
                                 QVector<int>() << MultitrackModel::IsFilteredRole);
    }
}

/**
 * @class MoveTrackCommand
 * @brief “移动轨道”命令
 * 上移或下移轨道的顺序。
 */
MoveTrackCommand::MoveTrackCommand(MultitrackModel &model,
                                   int fromTrackIndex,
                                   int toTrackIndex,
                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_fromTrackIndex(qBound(0, fromTrackIndex, qMax(model.rowCount() - 1, 0)))
    , m_toTrackIndex(qBound(0, toTrackIndex, qMax(model.rowCount() - 1, 0)))
{
    // 根据移动方向设置命令文本
    if (m_toTrackIndex > m_fromTrackIndex)
        setText(QObject::tr("Move track down"));
    else
        setText(QObject::tr("Move track up"));
}

void MoveTrackCommand::redo()
{
    LOG_DEBUG() << "fromTrackIndex" << m_fromTrackIndex << "toTrackIndex" << m_toTrackIndex;
    m_model.moveTrack(m_fromTrackIndex, m_toTrackIndex); // 调用模型执行移动
}

void MoveTrackCommand::undo()
{
    LOG_DEBUG() << "fromTrackIndex" << m_fromTrackIndex << "toTrackIndex" << m_toTrackIndex;
    // 撤销时，反向移动
    m_model.moveTrack(m_toTrackIndex, m_fromTrackIndex);
}

/**
 * @class ChangeBlendModeCommand
 * @brief “更改混合模式”命令
 * 更改轨道的混合模式（如正常、叠加、变暗等）。
 */
ChangeBlendModeCommand::ChangeBlendModeCommand(Mlt::Transition &transition,
                                               const QString &propertyName,
                                               const QString &mode,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_transition(transition) // 轨道混合模式的转场服务
    , m_propertyName(propertyName)
    , m_newMode(mode) // 新的混合模式
{
    setText(QObject::tr("Change track blend mode"));
    m_oldMode = m_transition.get(m_propertyName.toLatin1().constData()); // 保存旧的混合模式
}

void ChangeBlendModeCommand::redo()
{
    LOG_DEBUG() << "mode" << m_newMode;
    if (!m_newMode.isEmpty()) {
        m_transition.set("disable", 0); // 启用转场
        m_transition.set(m_propertyName.toLatin1().constData(), m_newMode.toUtf8().constData());
    } else {
        m_transition.set("disable", 1); // 禁用转场（无混合模式）
    }
    MLT.refreshConsumer(); // 刷新播放器以显示效果
    emit modeChanged(m_newMode);
}

void ChangeBlendModeCommand::undo()
{
    LOG_DEBUG() << "mode" << m_newMode;
    if (!m_oldMode.isEmpty()) {
        m_transition.set("disable", 0);
        m_transition.set(m_propertyName.toLatin1().constData(), m_oldMode.toUtf8().constData());
    } else {
        m_transition.set("disable", 1);
    }
    MLT.refreshConsumer();
    emit modeChanged(m_oldMode);
}

/**
 * @class UpdateCommand
 * @brief “更新片段属性”命令
 * 一个通用命令，用于通过新的 XML 数据更新片段的属性（如滤镜、颜色等）。
 */
UpdateCommand::UpdateCommand(
    TimelineDock &timeline, int trackIndex, int clipIndex, int position, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_timeline(timeline)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_position(position)
    , m_isFirstRedo(true) // 标记是否为首次执行
    , m_undoHelper(*timeline.model())
    , m_ripple(Settings.timelineRipple()) // 读取当前波纹设置
    , m_rippleAllTracks(Settings.timelineRippleAllTracks())
{
    setText(QObject::tr("Change clip properties"));
    m_undoHelper.recordBeforeState(); // 在构造时记录状态
}

void UpdateCommand::setXmlAfter(const QString &xml)
{
    m_xmlAfter = xml; // 设置更新后的 XML
    m_ripple = Settings.timelineRipple(); // 重新读取波纹设置
    m_rippleAllTracks = Settings.timelineRippleAllTracks();
}

void UpdateCommand::setPosition(int trackIndex, int clipIndex, int position)
{
    // 允许在执行前更新目标位置
    if (trackIndex >= 0)
        m_trackIndex = trackIndex;
    if (clipIndex >= 0)
        m_clipIndex = clipIndex;
    if (position >= 0)
        m_position = position;
    m_undoHelper.recordBeforeState();
}

void UpdateCommand::setRippleAllTracks(bool ripple)
{
    m_rippleAllTracks = ripple;
}

void UpdateCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position"
                << m_position;
    if (!m_isFirstRedo)
        m_undoHelper.recordBeforeState(); // 非首次执行时，记录当前状态
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xmlAfter.toUtf8().constData());
    if (m_ripple) {
        // 如果是波纹模式，先移除旧片段，再插入新片段
        m_timeline.model()->removeClip(m_trackIndex, m_clipIndex, m_rippleAllTracks);
        m_timeline.model()->insertClip(m_trackIndex, clip, m_position, m_rippleAllTracks, false);
    } else {
        // 如果是覆盖模式，先提升旧片段，再覆盖新片段
        m_timeline.model()->liftClip(m_trackIndex, m_clipIndex);
        m_timeline.model()->overwrite(m_trackIndex, clip, m_position, false);
    }
    m_undoHelper.recordAfterState();
}

void UpdateCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position"
                << m_position;
    m_undoHelper.undoChanges(); // 恢复到更新前的状态
    m_timeline.setSelection(QList<QPoint>() << QPoint(m_clipIndex, m_trackIndex));
    m_isFirstRedo = false;
}

/**
 * @class DetachAudioCommand
 * @brief “分离音频”命令
 * 将一个音视频片段的视频和音频部分分离到不同的轨道上。
 */
DetachAudioCommand::DetachAudioCommand(TimelineDock &timeline,
                                       int trackIndex,
                                       int clipIndex,
                                       int position,
                                       const QString &xml,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_timeline(timeline)
    , m_trackIndex(qBound(0, trackIndex, qMax(timeline.model()->rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_position(position)
    , m_targetTrackIndex(-1) // 目标音频轨道索引
    , m_xml(xml) // 原始片段的 XML
    , m_undoHelper(*timeline.model())
    , m_trackAdded(false) // 标记是否添加了新轨道
{
    setText(QObject::tr("Detach Audio"));
}

void DetachAudioCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position"
                << m_position;
    Mlt::Producer audioClip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    Mlt::Producer videoClip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    int groupNumber = -1;
    if (audioClip.is_valid() && videoClip.is_valid()) {
        auto model = m_timeline.model();

        // 保存原始片段的组号
        {
            auto videoClipInfo = model->getClipInfo(m_trackIndex, m_clipIndex);
            if (videoClipInfo && videoClipInfo->cut
                && videoClipInfo->cut->property_exists(kShotcutGroupProperty)) {
                groupNumber = videoClipInfo->cut->get_int(kShotcutGroupProperty);
            }
        }

        // 在视频片段上禁用音频流和音频滤镜
        videoClip.set("astream", -1);
        videoClip.set("audio_index", -1);
        // ... (遍历并移除音频滤镜)

        // 在音频片段上禁用视频流和视频滤镜
        audioClip.set("vstream", -1);
        audioClip.set("video_index", -1);
        // ... (遍历并移除视频滤镜)

        // 寻找一个合适的音频轨道来放置音频片段
        int n = model->trackList().size();
        for (int i = 0; i < n; i++) {
            // ... (检查轨道是否为空且为音频轨道)
        }
        if (m_targetTrackIndex == -1) {
            // 如果没有找到合适的轨道，则添加一个新的音频轨道
            m_targetTrackIndex = model->addAudioTrack();
            m_trackAdded = m_targetTrackIndex > -1;
        }

        if (m_targetTrackIndex > -1) {
            // ... (为新轨道设置 UUID)
            m_undoHelper.recordBeforeState();
            // 将音频片段覆盖到目标音频轨道
            model->overwrite(m_targetTrackIndex, audioClip, m_position, false);
            // 用仅视频的片段替换原始片段
            model->overwrite(m_trackIndex, videoClip, m_position, false);
            // 恢复视频片段的组
            if (groupNumber >= 0) {
                // ... (设置组号)
            }
            m_undoHelper.recordAfterState();
            // ... (通知模型数据更改)
        }
    }
}

void DetachAudioCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex << "position"
                << m_position;
    auto model = m_timeline.model();
    m_undoHelper.undoChanges(); // 恢复轨道内容
    if (m_trackAdded) {
        // 如果添加了新轨道，则移除它
        model->removeTrack(m_targetTrackIndex);
        m_targetTrackIndex = -1;
    }
    // 用原始片段替换当前的视频片段
    Mlt::Producer originalClip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    model->overwrite(m_trackIndex, originalClip, m_position, true);
    // ... (通知模型数据更改)
    m_timeline.setSelection(QList<QPoint>() << QPoint(m_clipIndex, m_trackIndex));
}

/**
 * @class ReplaceCommand
 * @brief “替换片段”命令
 * 用一个新的片段完全替换时间线上的一个旧片段。
 */
ReplaceCommand::ReplaceCommand(
    MultitrackModel &model, int trackIndex, int clipIndex, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_clipIndex(clipIndex)
    , m_xml(xml) // 新片段的 XML
    , m_isFirstRedo(true)
    , m_undoHelper(model)
{
    setText(QObject::tr("Replace timeline clip"));
    m_undoHelper.recordBeforeState();
}

void ReplaceCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    if (!m_isFirstRedo)
        m_undoHelper.recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.replace(m_trackIndex, m_clipIndex, clip); // 调用模型执行替换
    m_undoHelper.recordAfterState();
}

void ReplaceCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex << "clipIndex" << m_clipIndex;
    m_undoHelper.undoChanges(); // 恢复被替换的片段
    m_isFirstRedo = false;
}

/**
 * @class AlignClipsCommand
 * @brief “对齐片段”命令
 * 根据参考轨道的音频波形，将其他轨道上的片段与参考轨道对齐。
 */
AlignClipsCommand::AlignClipsCommand(MultitrackModel &model, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_undoHelper(m_model)
    , m_redo(false)
{
    m_undoHelper.setHints(UndoHelper::RestoreTracks);
    m_undoHelper.recordBeforeState(); // 在构造时记录对齐前的状态
    setText(QObject::tr("Align clips to reference track"));
}
// 注意：AlignClipsCommand 的 redo() 和 undo() 方法未在提供的代码片段中显示，
// 但根据模式，redo() 会调用模型的对齐逻辑，undo() 会使用 m_undoHelper 恢复状态。


/**
 * @class AlignClipsCommand
 * @brief “对齐片段”命令
 * 根据参考轨道的音频波形，将其他轨道上的片段与参考轨道对齐。
 */
// (AlignClipsCommand 的构造函数已在之前的代码片段中)

/**
 * @brief 添加一个对齐信息。
 * @param uuid 要对齐的片段的 UUID。
 * @param offset 对齐后的新位置（时间偏移量）。
 * @param speed 对齐后的新速度（用于变速对齐）。
 */
void AlignClipsCommand::addAlignment(QUuid uuid, int offset, double speed)
{
    Alignment alignment;
    alignment.uuid = uuid;
    alignment.offset = offset;
    alignment.speed = speed;
    m_alignments.append(alignment);
}

void AlignClipsCommand::redo()
{
    LOG_DEBUG() << "Alignment Clips:" << m_alignments.size();
    struct ClipItem
    {
        Mlt::Producer *clip; // 处理后的片段
        int track;           // 目标轨道
        int start;           // 目标起始位置
    };
    QVector<ClipItem> clipMemory; // 用于临时存储所有待重新放置的片段

    // 移除所有需要对齐的片段，并记住它们的信息。
    for (auto &alignment : m_alignments) {
        int trackIndex, clipIndex;
        auto info = m_model.findClipByUuid(alignment.uuid, trackIndex, clipIndex);
        if (!info || !info->cut || !info->cut->is_valid()) {
            continue;
        }
        ClipItem item;
        // 如果需要改变速度
        if (alignment.speed != 1.0) {
            // 计算新的速度
            double warpspeed = Util::GetSpeedFromProducer(info->producer) * alignment.speed;
            QString filename = Util::GetFilenameFromProducer(info->producer, false);
            // 创建一个 "timewarp" Producer 来实现变速
            QString s = QStringLiteral("%1:%2:%3").arg("timewarp").arg(warpspeed).arg(filename);
            item.clip = new Mlt::Producer(MLT.profile(), s.toUtf8().constData());
            if (!item.clip || !item.clip->is_valid()) {
                delete item.clip;
                continue;
            }
            // 复制原始片段的属性和滤镜到新的变速片段
            Util::passProducerProperties(info->producer, item.clip);
            Util::updateCaption(item.clip);
            // 根据速度调整时长、入点和出点
            int length = qRound(info->producer->get_length() / alignment.speed);
            int in = qRound(info->cut->get_in() / alignment.speed);
            int out = qRound(info->cut->get_out() / alignment.speed);
            item.clip->set("length", item.clip->frames_to_time(length, mlt_time_clock));
            item.clip->set_in_and_out(in, out);
            MLT.copyFilters(*info->producer, *item.clip);
        } else {
            // 如果不需要变速，直接复制原始片段
            item.clip = new Mlt::Producer(info->cut);
        }
        item.track = trackIndex;
        item.start = alignment.offset; // 使用计算出的新位置
        clipMemory.append(item);
        m_model.liftClip(trackIndex, clipIndex); // 从时间线上移除
    }

    // 将所有片段放回新的位置。
    for (auto &item : clipMemory) {
        m_model.overwrite(item.track, *item.clip, item.start, false, false);
        delete item.clip; // 释放内存
    }

    if (!m_redo) {
        m_redo = true;
        m_undoHelper.recordAfterState(); // 首次执行后记录状态
    }
}

void AlignClipsCommand::undo()
{
    m_undoHelper.undoChanges(); // 使用 UndoHelper 恢复到对齐前的状态
}

/**
 * @class ApplyFiltersCommand
 * @brief “应用复制的滤镜”命令
 * 将一个或多个滤镜从一个片段复制并应用到其他选定的片段上。
 */
ApplyFiltersCommand::ApplyFiltersCommand(MultitrackModel &model,
                                         const QString &filterProducerXml,
                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_xml(filterProducerXml) // 包含待应用滤镜的 XML
{
    setText(QObject::tr("Apply copied filters"));
}
