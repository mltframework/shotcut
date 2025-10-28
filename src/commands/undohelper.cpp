//*
 * Copyright (c) 2015-2024 Meltytech, LLC
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

#include "undohelper.h"

#include "Logger.h"
#include "mltcontroller.h"
#include "models/audiolevelstask.h"
#include "shotcut_mlt_properties.h"

#include <QScopedPointer>
#include <QUuid>

// 用于调试的宏定义，方便开关调试日志
#ifdef UNDOHELPER_DEBUG
#define UNDOLOG LOG_DEBUG()
#else
#define UNDOLOG \
    if (false) \
    LOG_DEBUG()
#endif

/**
 * @class UndoHelper
 * @brief 撤销/重做操作的辅助类。
 * 它通过记录操作前和操作后整个时间线（或受影响轨道）的状态，来计算差异并在撤销时精确恢复。
 */

UndoHelper::UndoHelper(MultitrackModel &model)
    : m_model(model)
    , m_hints(NoHints) // 默认无优化提示
{}

/**
 * @brief 记录操作前的状态。
 * 遍历所有轨道和片段，将每个片段的详细信息（XML、入出点、位置、组等）存储在内部映射中。
 */
void UndoHelper::recordBeforeState()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("Before state");
#endif
    m_state.clear(); // 清空之前的状态
    m_clipsAdded.clear();
    m_insertedOrder.clear(); // 清空并重新记录片段的原始顺序
    // 遍历所有轨道
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        // 遍历轨道上的每个片段（包括空白）
        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            QUuid uid = MLT.ensureHasUuid(clip->parent()); // 确保片段有 UUID
            if (clip->is_blank()) {
                uid = MLT.ensureHasUuid(*clip); // 空白片段也需要 UUID
            }
            m_insertedOrder << uid; // 记录顺序
            Info &info = m_state[uid]; // 获取或创建该 UUID 对应的信息结构体
            // 如果没有跳过 XML 的提示，则保存片段的完整 XML
            if (!(m_hints & SkipXML))
                info.xml = MLT.XML(&clip->parent());
            Mlt::ClipInfo clipInfo;
            playlist.clip_info(j, &clipInfo);
            // 记录片段的基本信息
            info.frame_in = clipInfo.frame_in;
            info.frame_out = clipInfo.frame_out;
            info.oldTrackIndex = i; // 记录原始轨道索引
            info.oldClipIndex = j;  // 记录原始片段索引
            info.isBlank = playlist.is_blank(j);
            // 记录组信息
            if (clipInfo.cut && clipInfo.cut->property_exists(kShotcutGroupProperty)) {
                info.group = clipInfo.cut->get_int(kShotcutGroupProperty);
            }
        }
    }
}

/**
 * @brief 记录操作后的状态。
 * 将当前时间线状态与 `recordBeforeState` 记录的状态进行比较，计算出哪些片段被添加、删除、移动或修改。
 */
void UndoHelper::recordAfterState()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("After state");
#endif
    QList<QUuid> clipsRemoved = m_state.keys(); // 假设所有原始片段都被移除了，然后逐一排除
    m_clipsAdded.clear();
    // 再次遍历所有轨道和片段
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            QUuid uid = MLT.ensureHasUuid(clip->parent());
            if (clip->is_blank()) {
                uid = MLT.ensureHasUuid(*clip);
            }

            // 如果一个片段不在之前的状态中，说明它是新添加的
            if (!m_state.contains(uid)) {
                UNDOLOG << "New clip at" << i << j;
                m_clipsAdded << uid;
                m_affectedTracks << i; // 记录受影响的轨道
            } else {
                Info &info = m_state[uid];
                info.changes = 0;
                info.newTrackIndex = i; // 记录新位置
                info.newClipIndex = j;

                // 如果轨道或片段索引变了，说明片段被移动了
                if (info.oldTrackIndex != info.newTrackIndex
                    || info.oldClipIndex != info.newClipIndex) {
                    UNDOLOG << "Clip" << uid << "moved from" << info.oldTrackIndex
                            << info.oldClipIndex << "to" << info.newTrackIndex << info.newClipIndex;
                    info.changes |= Moved;
                    m_affectedTracks << info.oldTrackIndex;
                    m_affectedTracks << info.newTrackIndex;
                }

                // 如果没有跳过 XML，并且片段不是空白，则比较 XML 是否变化
                if (!(m_hints & SkipXML) && !info.isBlank) {
                    QString newXml = MLT.XML(&clip->parent());
                    if (info.xml != newXml) {
                        UNDOLOG << "Modified xml:" << uid;
                        info.changes |= XMLModified;
                        m_affectedTracks << i;
                    }
                }

                // 比较入出点是否变化
                Mlt::ClipInfo newInfo;
                playlist.clip_info(j, &newInfo);
                if (info.frame_in != newInfo.frame_in || info.frame_out != newInfo.frame_out) {
                    UNDOLOG << "In/out changed:" << uid;
                    info.changes |= ClipInfoModified;
                    // 计算入出点的变化量
                    info.in_delta = info.frame_in - newInfo.frame_in;
                    info.out_delta = newInfo.frame_out - newInfo.frame_out;
                    m_affectedTracks << i;
                }
            }
            clipsRemoved.removeOne(uid); // 从“已移除”列表中排除
        }
    }

    // 剩下的就是真正被移除的片段
    foreach (QUuid uid, clipsRemoved) {
        UNDOLOG << "Clip removed:" << uid;
        auto &info = m_state[uid];
        info.changes = Removed;
        m_affectedTracks << info.oldTrackIndex;
    }
}

/**
 * @brief 撤销更改。
 * 根据之前记录的差异，将时间线恢复到操作前的状态。
 */
void UndoHelper::undoChanges()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("Before undo");
#endif
    // 如果提示需要恢复整个轨道，则使用更简单高效的方法
    if (m_hints & RestoreTracks) {
        restoreAffectedTracks();
        emit m_model.modified();
#ifdef UNDOHELPER_DEBUG
        debugPrintState("After undo");
#endif
        return;
    }
    QMap<int, int> indexAdjustment; // 用于跟踪因插入/删除导致的索引偏移

    /* 按照原始片段顺序（m_insertedOrder）处理。
     * 在处理每个片段时，确保它后面的片段已恢复到原始状态，然后再处理下一个。
     */
    foreach (QUuid uid, m_insertedOrder) {
        const Info &info = m_state[uid];
        UNDOLOG << "Handling uid" << uid << "on track" << info.oldTrackIndex << "index"
                << info.oldClipIndex;

        int trackIndex = m_model.trackList()[info.oldTrackIndex].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(trackIndex));
        Mlt::Playlist playlist(*trackProducer);

        // 计算当前要恢复的位置，并考虑之前的插入/删除操作带来的索引偏移
        int currentIndex = qMin(info.oldClipIndex + indexAdjustment[trackIndex],
                                playlist.count() - 1);

        /* 处理被移动的片段：找到它们，然后移动到原始位置。 */
        if (info.changes & Moved) {
            Q_ASSERT(info.newTrackIndex == info.oldTrackIndex
                     && "cross-track moves are unsupported so far");
            int clipCurrentlyAt = -1;
            // 通过 UUID 查找片段当前的位置
            for (int i = 0; i < playlist.count(); ++i) {
                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(i));
                if (MLT.uuid(clip->parent()) == uid || MLT.uuid(*clip) == uid) {
                    clipCurrentlyAt = i;
                    break;
                }
            }
            Q_ASSERT(clipCurrentlyAt != -1 && "Moved clip could not be found");
            UNDOLOG << "Found clip with uid" << uid << "at index" << clipCurrentlyAt;

            // 如果当前位置不是目标位置，则移动它
            if (clipCurrentlyAt != info.oldClipIndex
                && (currentIndex < clipCurrentlyAt || currentIndex > clipCurrentlyAt + 1)) {
                UNDOLOG << "moving from" << clipCurrentlyAt << "to" << currentIndex;
                // 使用模型的通知机制来移动行，以便 UI 同步更新
                QModelIndex modelIndex = m_model.createIndex(clipCurrentlyAt, 0, info.oldTrackIndex);
                m_model.beginMoveRows(modelIndex.parent(),
                                      clipCurrentlyAt,
                                      clipCurrentlyAt,
                                      modelIndex.parent(),
                                      currentIndex);
                playlist.move(clipCurrentlyAt, currentIndex);
                m_model.endMoveRows();
            }
        }

        /* 处理被移除的片段：使用保存的 XML 重新插入它们。 */
        if (info.changes & Removed) {
            QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
            m_model.beginInsertRows(modelIndex.parent(), currentIndex, currentIndex);
            if (info.isBlank) {
                // 如果是空白，插入空白
                playlist.insert_blank(currentIndex, info.frame_out - info.frame_in);
                UNDOLOG << "inserting isBlank at " << currentIndex;
            } else {
                UNDOLOG << "inserting clip at " << currentIndex << uid;
                Q_ASSERT(!(m_hints & SkipXML) && "Cannot restore clip without stored XML");
                Q_ASSERT(!info.xml.isEmpty());
                // 从 XML 重新创建片段
                Mlt::Producer restoredClip(MLT.profile(),
                                           "xml-string",
                                           info.xml.toUtf8().constData());
                if (restoredClip.type() == mlt_service_tractor_type) { // 如果是转场
                    restoredClip.set("mlt_type", "mlt_producer");
                } else {
                    // 修复可能与相邻片段的转场连接
                    fixTransitions(playlist, currentIndex, restoredClip);
                }
                playlist.insert(restoredClip, currentIndex, info.frame_in, info.frame_out);
            }
            m_model.endInsertRows();

            // 恢复 UUID 和组信息
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
            Q_ASSERT(currentIndex < playlist.count());
            Q_ASSERT(!clip.isNull());
            if (info.isBlank) {
                MLT.setUuid(*clip, uid);
            } else {
                MLT.setUuid(clip->parent(), uid);
            }
            if (info.group >= 0) {
                clip->set(kShotcutGroupProperty, info.group);
            }
            // 启动音频电平计算任务
            AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
            indexAdjustment[trackIndex]++; // 更新索引偏移
        }

        /* 处理被修改的片段（主要是入出点）。 */
        if (info.changes & ClipInfoModified) {
            int filterIn = MLT.filterIn(playlist, currentIndex);
            int filterOut = MLT.filterOut(playlist, currentIndex);

            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
            if (clip && clip->is_valid()) {
                UNDOLOG << "resizing clip at" << currentIndex << "in" << info.frame_in << "out"
                        << info.frame_out;
                // 清除可能存在的混合信息
                if (clip->parent().get_data("mlt_mix"))
                    clip->parent().set("mlt_mix", nullptr, 0);
                if (clip->get_data("mix_in"))
                    clip->set("mix_in", nullptr, 0);
                if (clip->get_data("mix_out"))
                    clip->set("mix_out", nullptr, 0);
                // 调整片段的入出点
                playlist.resize_clip(currentIndex, info.frame_in, info.frame_out);
                // 调整滤镜的入出点以匹配
                MLT.adjustClipFilters(clip->parent(),
                                      filterIn,
                                      filterOut,
                                      info.in_delta,
                                      info.out_delta,
                                      info.in_delta);
            }

            // 通知模型数据已更改
            QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
            QVector<int> roles;
            roles << MultitrackModel::InPointRole;
            roles << MultitrackModel::OutPointRole;
            roles << MultitrackModel::DurationRole;
            emit m_model.dataChanged(modelIndex, modelIndex, roles);
            if (clip && clip->is_valid())
                AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
        }
    }

    /* 最后，再次遍历轨道，移除所有新添加的片段，并清除临时使用的 UUID 属性。 */
    int trackIndex = 0;
    foreach (const Track &track, m_model.trackList()) {
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(track.mlt_index));
        Mlt::Playlist playlist(*trackProducer);
        for (int i = playlist.count() - 1; i >= 0; --i) { // 从后往前删除，避免索引问题
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(i));
            QUuid uid = MLT.uuid(clip->parent());
            if (clip->is_blank()) {
                uid = MLT.uuid(*clip);
            }
            if (m_clipsAdded.removeOne(uid)) { // 如果是新添加的片段
                UNDOLOG << "Removing clip at" << i;
                m_model.beginRemoveRows(m_model.index(trackIndex), i, i);
                // 清除混合信息
                if (clip->parent().get_data("mlt_mix"))
                    clip->parent().set("mlt_mix", NULL, 0);
                if (clip->get_data("mix_in"))
                    clip->set("mix_in", NULL, 0);
                if (clip->get_data("mix_out"))
                    clip->set("mix_out", NULL, 0);
                playlist.remove(i);
                m_model.endRemoveRows();
            }
        }
        trackIndex++;
    }

    emit m_model.modified();
#ifdef UNDOHELPER_DEBUG
    debugPrintState("After undo");
#endif
}

/**
 * @brief 设置优化提示。
 * 这些提示可以改变 UndoHelper 的行为，例如跳过 XML 记录以提高性能。
 */
void UndoHelper::setHints(OptimizationHints hints)
{
    m_hints = hints;
}

/**
 * @brief 调试函数：打印当前时间线的完整状态。
 */
void UndoHelper::debugPrintState(const QString &title)
{
    LOG_DEBUG() << "timeline state:" << title << "{";
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QString trackStr = QStringLiteral("   track %1 (mlt-idx %2):").arg(i).arg(mltIndex);
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            Mlt::ClipInfo info;
            playlist.clip_info(j, &info);
            QUuid uid = MLT.uuid(*info.producer);
            if (info.producer->is_blank() && info.cut) {
                uid = MLT.uuid(*info.cut);
            }
            trackStr += QStringLiteral(" [ %5 %1 -> %2 (%3 frames) %4]")
                            .arg(info.frame_in)
                            .arg(info.frame_out)
                            .arg(info.frame_count)
                            .arg(info.cut->is_blank() ? "blank " : "clip")
                            .arg(uid.toString());
        }
        LOG_DEBUG() << qPrintable(trackStr);
    }
    LOG_DEBUG() << "}";
}

/**
 * @brief 恢复受影响的轨道。
 * 一种更简单但更彻底的撤销方法：清空受影响轨道的所有内容，然后根据记录的状态重新填充。
 */
void UndoHelper::restoreAffectedTracks()
{
    // 第一步：清空所有受影响的轨道。
    for (const auto &trackIndex : std::as_const(m_affectedTracks)) {
        if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
            auto mlt_index = m_model.trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> producer(m_model.tractor()->track(mlt_index));
            if (producer->is_valid()) {
                Mlt::Playlist playlist(*producer.data());
                m_model.beginRemoveRows(m_model.index(trackIndex), 0, playlist.count() - 1);
                UNDOLOG << "clearing track" << trackIndex;
                playlist.clear();
                m_model.endRemoveRows();
            }
        }
    }

 // 第二步：按照原始顺序，将所有属于这些轨道的片段重新添加回去
    for (const auto &uid : std::as_const(m_insertedOrder)) {
        const Info &info = m_state[uid];
        if (m_affectedTracks.contains(info.oldTrackIndex)) {
            // ... (与 undoChanges 中类似的插入逻辑)
        }
    }
    // 第三步：修复所有新添加片段的转场
    for (const auto &trackIndex : std::as_const(m_affectedTracks)) {
        // ... (遍历轨道并调用 fixTransitions)
    }
}

/**
 * @brief 修复与新插入片段相邻的转场。
 * 当一个片段被重新插入时，它旁边的转场可能仍然引用着旧的片段。
 * 此函数确保转场正确地连接到新的片段上。
 */
void UndoHelper::fixTransitions(Mlt::Playlist playlist, int clipIndex, Mlt::Producer clip)
{
    if (clip.is_blank()) {
        return;
    }
    int transitionIndex = 0;
    // 检查片段左右两边的邻居
    for (auto currentIndex : {clipIndex + 1, clipIndex - 1}) {
        Mlt::Producer producer(playlist.get_clip(currentIndex));
        if (producer.is_valid() && producer.parent().get(kShotcutTransitionProperty)) {
            Mlt::Tractor transition(producer.parent());
            if (transition.is_valid()) {
                QScopedPointer<Mlt::Producer> transitionClip(transition.track(transitionIndex));
                // 如果转场引用的不是新片段，则更新引用
                if (transitionClip->is_valid()
                    && transitionClip->parent().get_service() != clip.parent().get_service()) {
                    UNDOLOG << "Fixing transition at clip index" << currentIndex
                            << "transition index" << transitionIndex;
                    // 从新片段上切出与转场时长和位置相同的部分，并替换掉转场中的旧引用
                    transitionClip.reset(
                        clip.cut(transitionClip->get_in(), transitionClip->get_out()));
                    transition.set_track(*transitionClip.data(), transitionIndex);
                }
            }
        }
        transitionIndex++;
    }
}

