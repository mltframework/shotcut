/*
 * Copyright (c) 2015-2020 Meltytech, LLC
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

#ifndef UNDOHELPER_H
#define UNDOHELPER_H

#include "models/multitrackmodel.h"

#include <MltPlaylist.h>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>

/**
 * @class UndoHelper
 * @brief 撤销/重做操作的辅助类。
 * 
 * 这个类通过捕获操作前后的时间线状态，来提供精确的撤销功能。
 * 它记录了每个片段的详细信息，包括其 XML 表示、位置、入出点、组信息等。
 * 
 * 使用流程：
 * 1. 创建 UndoHelper 实例。
 * 2. 调用 `recordBeforeState()` 记录操作前的状态。
 * 3. 执行时间线修改操作。
 * 4. 调用 `recordAfterState()` 记录操作后的状态。
 * 5. 当需要撤销时，调用 `undoChanges()` 恢复到操作前的状态。
 */
class UndoHelper
{
public:
    /**
     * @brief 优化提示枚举。
     * 用于向 UndoHelper 提供如何更高效地记录状态的提示。
     */
    enum OptimizationHints {
        NoHints,      ///< 无提示，记录所有信息。
        SkipXML,      ///< 跳过保存片段的 XML。适用于只改变片段位置或入出点的操作，可提高性能。
        RestoreTracks ///< 使用“恢复轨道”模式。适用于复杂操作（如分割），它会清空受影响的轨道并完全重建。
    };

    /**
     * @brief 构造函数。
     * @param model 对多轨道模型的引用，UndoHelper 将通过此模型与时间线交互。
     */
    UndoHelper(MultitrackModel &model);

    /**
     * @brief 记录操作前的状态。
     * 遍历整个时间线，将所有片段的当前信息保存到内部数据结构中。
     */
    void recordBeforeState();

    /**
     * @brief 记录操作后的状态。
     * 遍历整个时间线，与 `recordBeforeState` 保存的状态进行比较，
     * 以确定哪些片段被添加、移动、修改或删除。
     */
    void recordAfterState();

    /**
     * @brief 撤销更改，恢复到 `recordBeforeState` 记录的状态。
     * 这是核心的撤销逻辑，它会执行一系列模型操作来精确恢复时间线。
     */
    void undoChanges();

    /**
     * @brief 设置优化提示。
     * @param hints 要应用的提示标志。
     */
    void setHints(OptimizationHints hints);

    /**
     * @brief 获取受影响的轨道索引集合。
     * @return 一个包含所有在操作中发生变化的轨道索引的 QSet。
     */
    QSet<int> affectedTracks() const { return m_affectedTracks; }

private:
    /**
     * @brief 调试函数：打印当前时间线的详细状态。
     * @param title 打印的标题。
     */
    void debugPrintState(const QString &title);

    /**
     * @brief 使用“恢复轨道”模式来撤销更改。
     * 当设置了 `RestoreTracks` 提示时，此函数会被 `undoChanges` 调用。
     */
    void restoreAffectedTracks();

    /**
     * @brief 修复与新插入片段相邻的转场。
     * 当一个片段被重新插入时，它旁边的转场可能仍然引用着旧的片段。
     * 此函数确保转场正确地连接到新的片段上。
     * @param playlist 片段所在的播放列表（轨道）。
     * @param clipIndex 片段在播放列表中的索引。
     * @param clip 新插入的片段 Producer。
     */
    void fixTransitions(Mlt::Playlist playlist, int clipIndex, Mlt::Producer clip);

    /**
     * @brief 变化标志枚举。
     * 用于位运算，以指示一个片段发生了哪些类型的变化。
     */
    enum ChangeFlags {
        NoChange = 0x0,        ///< 无变化。
        ClipInfoModified = 0x1, ///< 片段信息（入出点）被修改。
        XMLModified = 0x2,      ///< 片段的 XML（属性、滤镜等）被修改。
        Moved = 0x4,            ///< 片段被移动。
        Removed = 0x8           ///< 片段被移除。
    };

    /**
     * @brief 存储单个片段所有相关信息的结构体。
     */
    struct Info
    {
        int oldTrackIndex;      ///< 操作前的轨道索引。
        int oldClipIndex;       ///< 操作前的片段索引。
        int newTrackIndex;      ///< 操作后的轨道索引。
        int newClipIndex;       ///< 操作后的片段索引。
        bool isBlank;           ///< 是否为空白。
        QString xml;            ///< 片段的完整 XML 表示（如果未跳过）。
        int frame_in;           ///< 片段的入点。
        int frame_out;          ///< 片段的出点。
        int in_delta;           ///< 入点的变化量（用于撤销时调整滤镜）。
        int out_delta;          ///< 出点的变化量。
        int group;              ///< 片段所属的组号。
        int changes;            ///< 变化标志的位掩码。

        /// 构造函数，初始化所有成员为默认值。
        Info()
            : oldTrackIndex(-1)
            , oldClipIndex(-1)
            , newTrackIndex(-1)
            , newClipIndex(-1)
            , isBlank(false)
            , frame_in(-1)
            , frame_out(-1)
            , in_delta(0)
            , out_delta(0)
            , changes(NoChange)
            , group(-1)
        {}
    };

    QMap<QUuid, Info> m_state;     ///< 核心数据结构：将片段的 UUID 映射到其信息。
    QList<QUuid> m_clipsAdded;     ///< 在操作中被新添加的片段的 UUID 列表。
    QList<QUuid> m_insertedOrder;  ///< 记录操作前片段的原始插入顺序，用于撤销时恢复顺序。
    QSet<int> m_affectedTracks;    ///< 记录所有受影响的轨道索引。
    MultitrackModel &m_model;      ///< 对多轨道模型的引用。
    OptimizationHints m_hints;     ///< 当前设置的优化提示。
};

#endif // UNDOHELPER_H
