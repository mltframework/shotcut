/*
 * Copyright (c) 2013-2025 Meltytech, LLC
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

#ifndef COMMANDS_H
#define COMMANDS_H

#include "docks/timelinedock.h"
#include "models/markersmodel.h"
#include "models/multitrackmodel.h"
#include "undohelper.h"

#include <MltProducer.h>
#include <MltTransition.h>
#include <QObject>
#include <QString>
#include <QUndoCommand>
#include <QUuid>

#include <vector>

namespace Timeline {

/// ####################################################################
/// #                      枚举和结构体                              #
/// ####################################################################

/**
 * @brief 自定义的撤销/重做命令 ID。
 * 这些 ID 用于 `QUndoStack` 的命令合并机制，特别是 `mergeWith()` 函数。
 */
enum {
    UndoIdTrimClipIn = 100,        ///< 修剪片段入点命令的 ID
    UndoIdTrimClipOut,             ///< 修剪片段出点命令的 ID
    UndoIdFadeIn,                  ///< 淡入命令的 ID
    UndoIdFadeOut,                 ///< 淡出命令的 ID
    UndoIdTrimTransitionIn,        ///< 修剪转场入点命令的 ID
    UndoIdTrimTransitionOut,       ///< 修剪转场出点命令的 ID
    UndoIdAddTransitionByTrimIn,   ///< 通过修剪入点添加转场命令的 ID
    UndoIdAddTransitionByTrimOut,  ///< 通过修剪出点添加转场命令的 ID
    UndoIdUpdate,                  ///< 更新片段属性命令的 ID
    UndoIdMoveClip,                ///< 移动片段命令的 ID
    UndoIdChangeGain,              ///< 更改增益/音量命令的 ID
};

/**
 * @brief 一个简单的结构体，用于唯一标识时间线上的一个片段。
 * 它通过轨道索引和片段索引来定位片段。
 */
struct ClipPosition
{
    ClipPosition(int track, int clip)
    {
        trackIndex = track;
        clipIndex = clip;
    }

    /**
     * @brief 重载小于运算符，以便 ClipPosition 可以用作 QMap 或 QSet 的键。
     * 首先按轨道索引排序，然后按片段索引排序。
     */
    bool operator<(const ClipPosition &rhs) const
    {
        if (trackIndex == rhs.trackIndex) {
            return clipIndex < rhs.clipIndex;
        } else {
            return trackIndex < rhs.trackIndex;
        }
    }

    int trackIndex; ///< 片段所在的轨道索引
    int clipIndex;  ///< 片段在轨道上的索引
};

/// ####################################################################
/// #                          命令类声明                              #
/// ####################################################################

/**
 * @class AppendCommand
 * @brief “追加到轨道”命令
 * 将一个或多个片段添加到指定轨道的末尾。
 */
class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(MultitrackModel &model,
                  int trackIndex,
                  const QString &xml,
                  bool skipProxy = false,
                  bool seek = true,
                  QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model; ///< 对多轨道模型的引用
    int m_trackIndex;          ///< 目标轨道的索引
    QString m_xml;             ///< 包含待添加片段数据的 XML 字符串
    UndoHelper m_undoHelper;   ///< 用于记录和恢复状态的辅助类
    bool m_skipProxy;          ///< 是否跳过代理文件的生成
    bool m_seek;               ///< 操作完成后是否跳转到新片段
    QVector<QUuid> m_uuids;    ///< 待添加片段的 UUID 列表
};

/**
 * @class InsertCommand
 * @brief “插入到轨道”命令
 * 将一个或多个片段插入到指定轨道的指定位置，并将该位置之后的片段向后推（波纹编辑）。
 */
class InsertCommand : public QUndoCommand
{
public:
    InsertCommand(MultitrackModel &model,
                  MarkersModel &markersModel,
                  int trackIndex,
                  int position,
                  const QString &xml,
                  bool seek = true,
                  QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    MarkersModel &m_markersModel; ///< 对标记模型的引用，用于波纹标记
    int m_trackIndex;
    int m_position;               ///< 插入的位置（时间点）
    QString m_xml;
    QStringList m_oldTracks;      ///< （此变量在代码中未使用，可能是遗留）
    UndoHelper m_undoHelper;
    bool m_seek;
    bool m_rippleAllTracks;       ///< 是否波纹所有轨道
    bool m_rippleMarkers;         ///< 是否波纹标记
    int m_markersShift;           ///< 记录标记移动的量
    QVector<QUuid> m_uuids;
};

/**
 * @class OverwriteCommand
 * @brief “覆盖到轨道”命令
 * 将一个或多个片段放置到指定轨道的指定位置，覆盖该时间段内原有的所有内容。
 */
class OverwriteCommand : public QUndoCommand
{
public:
    OverwriteCommand(MultitrackModel &model,
                     int trackIndex,
                     int position,
                     const QString &xml,
                     bool seek = true,
                     QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_position;               ///< 覆盖的起始位置
    QString m_xml;
    UndoHelper m_undoHelper;
    bool m_seek;
    QVector<QUuid> m_uuids;
};

/**
 * @class LiftCommand
 * @brief “从轨道提升”命令
 * 从轨道中移除一个片段，但在其位置留下空白（不移动后续片段）。
 */
class LiftCommand : public QUndoCommand
{
public:
    LiftCommand(MultitrackModel &model, int trackIndex, int clipIndex, QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;              ///< 要提升的片段的索引
    UndoHelper m_undoHelper;
};

/**
 * @class RemoveCommand
 * @brief “从轨道移除”命令
 * 从轨道中移除一个片段，并填补其留下的空白（波纹删除）。
 */
class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(MultitrackModel &model,
                  MarkersModel &markersModel,
                  int trackIndex,
                  int clipIndex,
                  QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    MarkersModel &m_markersModel;
    int m_trackIndex;
    int m_clipIndex;              ///< 要移除的片段的索引
    UndoHelper m_undoHelper;
    bool m_rippleAllTracks;
    bool m_rippleMarkers;
    int m_markerRemoveStart;      ///< 被移除片段的起始位置
    int m_markerRemoveEnd;        ///< 被移除片段的结束位置
    QList<Markers::Marker> m_markers; ///< 保存原始标记列表，用于撤销
};

/**
 * @class GroupCommand
 * @brief “组合”命令
 * 将多个片段组合在一起，以便它们可以作为一个整体进行操作。
 */
class GroupCommand : public QUndoCommand
{
public:
    GroupCommand(MultitrackModel &model, QUndoCommand *parent = 0);
    void addToGroup(int trackIndex, int clipIndex); ///< 添加一个片段到待组合的列表
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    QList<ClipPosition> m_clips;      ///< 待组合的片段列表
    QMap<ClipPosition, int> m_prevGroups; ///< 记录每个片段在组合前的组号
};

/**
 * @class UngroupCommand
 * @brief “取消组合”命令
 * 将一个或多个片段从它们的组中移除。
 */
class UngroupCommand : public QUndoCommand
{
public:
    UngroupCommand(MultitrackModel &model, QUndoCommand *parent = 0);
    void removeFromGroup(int trackIndex, int clipIndex); ///< 添加一个片段到待取消组合的列表
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    QMap<ClipPosition, int> m_prevGroups; ///< 记录每个片段在取消组合前的组号
};

/**
 * @class NameTrackCommand
 * @brief “重命名轨道”命令
 * 更改轨道的名称。
 */
class NameTrackCommand : public QUndoCommand
{
public:
    NameTrackCommand(MultitrackModel &model,
                     int trackIndex,
                     const QString &name,
                     QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    QString m_name;                ///< 新的轨道名称
    QString m_oldName;             ///< 旧的轨道名称
};

/**
 * @class MergeCommand
 * @brief “合并相邻片段”命令
 * 将一个片段与其后面的相邻片段合并（如果它们来自同一个源文件）。
 */
class MergeCommand : public QUndoCommand
{
public:
    MergeCommand(MultitrackModel &model, int trackIndex, int clipIndex, QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;              ///< 要合并的片段的索引（通常是前一个）
    UndoHelper m_undoHelper;
};

/**
 * @class MuteTrackCommand
 * @brief “切换轨道静音”命令
 * 静音或取消静音指定轨道。
 */
class MuteTrackCommand : public QUndoCommand
{
public:
    MuteTrackCommand(MultitrackModel &model, int trackIndex, QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    bool m_oldValue;              ///< 轨道在操作前的静音状态
};

/**
 * @class HideTrackCommand
 * @brief “切换轨道隐藏”命令
 * 隐藏或显示指定轨道（在视频轨道中）。
 */
class HideTrackCommand : public QUndoCommand
{
public:
    HideTrackCommand(MultitrackModel &model, int trackIndex, QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    bool m_oldValue;              ///< 轨道在操作前的隐藏状态
};

/**
 * @class CompositeTrackCommand
 * @brief “更改轨道合成”命令
 * 启用或禁用轨道的合成模式（例如，混合模式）。
 */
class CompositeTrackCommand : public QUndoCommand
{
public:
    CompositeTrackCommand(MultitrackModel &model,
                          int trackIndex,
                          bool value,
                          QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    bool m_value;                 ///< 新的合成状态
    bool m_oldValue;              ///< 旧的合成状态
};

/**
 * @class LockTrackCommand
 * @brief “锁定轨道”命令
 * 锁定或解锁指定轨道，防止意外修改。
 */
class LockTrackCommand : public QUndoCommand
{
public:
    LockTrackCommand(MultitrackModel &model, int trackIndex, bool value, QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    bool m_value;                 ///< 新的锁定状态
    bool m_oldValue;              ///< 旧的锁定状态
};

/**
 * @class MoveClipCommand
 * @brief “移动片段”命令
 * 一个复杂的命令，用于在时间线上移动一个或多个片段，可以跨轨道移动，并支持波纹模式。
 */
class MoveClipCommand : public QUndoCommand
{
public:
    MoveClipCommand(TimelineDock &timeline,
                    int trackDelta,
                    int positionDelta,
                    bool ripple,
                    QUndoCommand *parent = 0);
    void addClip(int trackIndex, int clipIndex); ///< 添加一个片段到待移动的列表
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdMoveClip; } ///< 返回命令的唯一 ID
    bool mergeWith(const QUndoCommand *other) override; ///< 尝试与另一个命令合并

private:
    void redoMarkers(); ///< 处理移动操作对标记的影响

    TimelineDock &m_timeline;    ///< 对时间线 Dock 的引用
    MultitrackModel &m_model;
    MarkersModel &m_markersModel;

    /**
     * @brief 一个结构体，用于存储待移动片段的详细信息。
     */
    struct Info
    {
        int trackIndex;
        int clipIndex;
        int frame_in;   ///< 片段的入点
        int frame_out;  ///< 片段的出点
        int start;      ///< 片段在轨道上的起始时间
        int group;      ///< 片段所属的组号
        QUuid uuid;     ///< 片段的唯一标识符

        Info()
            : trackIndex(-1)
            , clipIndex(-1)
            , frame_in(-1)
            , frame_out(-1)
            , start(0)
            , group(-1)
        {}
    };

    int m_trackDelta;              ///< 轨道偏移量
    int m_positionDelta;           ///< 位置偏移量
    bool m_ripple;                 ///< 是否为波纹移动
    bool m_rippleAllTracks;
    bool m_rippleMarkers;
    UndoHelper m_undoHelper;
    QMultiMap<int, Info> m_clips;  ///< 待移动的片段列表，按位置排序
    bool m_redo;                   ///< 标记是否已执行过 redo
    int m_earliestStart;           ///< 记录被移动片段中最靠前的起始位置
    QList<Markers::Marker> m_markers; ///< 保存原始标记列表
};

/**
 * @class TrimCommand
 * @brief 修剪命令的基类。
 * 为各种修剪操作（如修剪片段、修剪转场）提供公共功能，特别是对 UndoHelper 的管理。
 */
class TrimCommand : public QUndoCommand
{
public:
    explicit TrimCommand(QUndoCommand *parent = 0)
        : QUndoCommand(parent)
    {}
    void setUndoHelper(UndoHelper *helper) { m_undoHelper.reset(helper); } ///< 设置 UndoHelper

protected:
    QScopedPointer<UndoHelper> m_undoHelper; ///< 使用智能指针管理 UndoHelper 对象
};

/**
 * @class TrimClipInCommand
 * @brief “修剪片段入点”命令
 * 调整片段的入点，可以波纹删除或覆盖。
 */
class TrimClipInCommand : public TrimCommand
{
public:
    TrimClipInCommand(MultitrackModel &model,
                      MarkersModel &markersModel,
                      int trackIndex,
                      int clipIndex,
                      int delta,
                      bool ripple,
                      bool redo = true,
                      QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdTrimClipIn; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    MultitrackModel &m_model;
    MarkersModel &m_markersModel;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;                  ///< 修剪的量（正数表示向后移动入点）
    bool m_ripple;                 ///< 是否为波纹修剪
    bool m_rippleAllTracks;
    bool m_rippleMarkers;
    bool m_redo;                  ///< 标记是否需要执行 redo
    int m_markerRemoveStart;       ///< 被移除部分的起始位置
    int m_markerRemoveEnd;         ///< 被移除部分的结束位置
    QList<Markers::Marker> m_markers; ///< 保存原始标记列表
};

/**
 * @class TrimClipOutCommand
 * @brief “修剪片段出点”命令
 * 调整片段的出点，可以波纹删除或覆盖。
 */
class TrimClipOutCommand : public TrimCommand
{
public:
    TrimClipOutCommand(MultitrackModel &model,
                       MarkersModel &markersModel,
                       int trackIndex,
                       int clipIndex,
                       int delta,
                       bool ripple,
                       bool redo = true,
                       QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdTrimClipOut; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    MultitrackModel &m_model;
    MarkersModel &m_markersModel;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;                  ///< 修剪的量（正数表示向前移动出点）
    bool m_ripple;
    bool m_rippleAllTracks;
    bool m_rippleMarkers;
    bool m_redo;
    int m_markerRemoveStart;
    int m_markerRemoveEnd;
    QList<Markers::Marker> m_markers;
};

/**
 * @class SplitCommand
 * @brief “分割片段”命令
 * 在指定位置将一个或多个片段分割成两个。
 */
class SplitCommand : public QUndoCommand
{
public:
    SplitCommand(MultitrackModel &model,
                 const std::vector<int> &trackIndex, ///< 轨道索引列表
                 const std::vector<int> &clipIndex,  ///< 片段索引列表
                 int position,
                 QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    std::vector<int> m_trackIndex;
    std::vector<int> m_clipIndex;
    int m_position;               ///< 分割点位置
    UndoHelper m_undoHelper;
};


/**
 * @class FadeInCommand
 * @brief “调整淡入”命令
 * 为片段添加或调整淡入效果。
 */
class FadeInCommand : public QUndoCommand
{
public:
    FadeInCommand(MultitrackModel &model,
                  int trackIndex,
                  int clipIndex,
                  int duration,
                  QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdFadeIn; } ///< 返回命令的唯一 ID
    bool mergeWith(const QUndoCommand *other) override; ///< 尝试与另一个命令合并

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_duration;               ///< 新的淡入时长
    int m_previous;               ///< 旧的淡入时长
};

/**
 * @class FadeOutCommand
 * @brief “调整淡出”命令
 * 为片段添加或调整淡出效果。
 */
class FadeOutCommand : public QUndoCommand
{
public:
    FadeOutCommand(MultitrackModel &model,
                   int trackIndex,
                   int clipIndex,
                   int duration,
                   QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdFadeOut; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_duration;               ///< 新的淡出时长
    int m_previous;               ///< 旧的淡出时长
};

/// ####################################################################
/// #                        转场相关命令                              #
/// ####################################################################

/**
 * @class AddTransitionCommand
 * @brief “添加转场”命令
 * 在两个片段之间添加一个转场效果。
 */
class AddTransitionCommand : public QUndoCommand
{
public:
    AddTransitionCommand(TimelineDock &timeline,
                         int trackIndex,
                         int clipIndex,
                         int position,
                         bool ripple,
                         QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;
    int getTransitionIndex() const { return m_transitionIndex; } ///< 获取创建的转场的索引

private:
    TimelineDock &m_timeline;
    MultitrackModel &m_model;
    MarkersModel &m_markersModel;
    int m_trackIndex;
    int m_clipIndex;
    int m_position;               ///< 转场开始的位置
    int m_transitionIndex;        ///< 存储创建的转场的索引
    bool m_ripple;                ///< 是否为波纹添加
    UndoHelper m_undoHelper;
    bool m_rippleAllTracks;
    bool m_rippleMarkers;
    int m_markerOldStart;         ///< 记录被修改片段的原始起始位置
    int m_markerNewStart;         ///< 记录被修改片段的新起始位置
    QList<Markers::Marker> m_markers; ///< 保存原始标记列表
};

/**
 * @class TrimTransitionInCommand
 * @brief “修剪转场入点”命令
 * 调整转场效果的开始时间。
 */
class TrimTransitionInCommand : public TrimCommand
{
public:
    TrimTransitionInCommand(MultitrackModel &model,
                            int trackIndex,
                            int clipIndex,
                            int delta,
                            bool redo = true,
                            QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdTrimTransitionIn; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;                  ///< 修剪的量
    bool m_notify;                ///< 是否需要通知模型更新
    bool m_redo;
};

/**
 * @class TrimTransitionOutCommand
 * @brief “修剪转场出点”命令
 * 调整转场效果的结束时间。
 */
class TrimTransitionOutCommand : public TrimCommand
{
public:
    TrimTransitionOutCommand(MultitrackModel &model,
                             int trackIndex,
                             int clipIndex,
                             int delta,
                             bool redo = true,
                             QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdTrimTransitionOut; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;                  ///< 修剪的量
    bool m_notify;
    bool m_redo;
};

/**
 * @class AddTransitionByTrimInCommand
 * @brief “通过修剪入点添加转场”命令
 * 一种特殊的添加转场方式，通过修剪前一个片段的出点来创建转场。
 */
class AddTransitionByTrimInCommand : public TrimCommand
{
public:
    AddTransitionByTrimInCommand(TimelineDock &timeline,
                                 int trackIndex,
                                 int clipIndex,
                                 int duration,
                                 int trimDelta,
                                 bool redo = true,
                                 QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdAddTransitionByTrimIn; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    TimelineDock &m_timeline;
    int m_trackIndex;
    int m_clipIndex;
    int m_duration;               ///< 转场时长
    int m_trimDelta;              ///< 对前一个片段的修剪量
    bool m_notify;
    bool m_redo;
};

/**
 * @class RemoveTransitionByTrimInCommand
 * @brief “通过修剪入点移除转场”命令
 * 移除一个转场，并恢复前一个片段的出点。
 */
class RemoveTransitionByTrimInCommand : public TrimCommand
{
public:
    RemoveTransitionByTrimInCommand(MultitrackModel &model,
                                    int trackIndex,
                                    int clipIndex,
                                    int delta,
                                    QString xml,
                                    bool redo = true,
                                    QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;                  ///< 转场的时长
    QString m_xml;                ///< 转场效果的 XML 数据，用于恢复属性
    bool m_redo;
};

/**
 * @class RemoveTransitionByTrimOutCommand
 * @brief “通过修剪出点移除转场”命令
 * 移除一个转场，并恢复后一个片段的入点。
 */
class RemoveTransitionByTrimOutCommand : public TrimCommand
{
public:
    RemoveTransitionByTrimOutCommand(MultitrackModel &model,
                                     int trackIndex,
                                     int clipIndex,
                                     int delta,
                                     QString xml,
                                     bool redo = true,
                                     QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_delta;                  ///< 转场的时长
    QString m_xml;                ///< 转场效果的 XML 数据
    bool m_redo;
};

/**
 * @class AddTransitionByTrimOutCommand
 * @brief “通过修剪出点添加转场”命令
 * 一种特殊的添加转场方式，通过修剪后一个片段的入点来创建转场。
 */
class AddTransitionByTrimOutCommand : public TrimCommand
{
public:
    AddTransitionByTrimOutCommand(MultitrackModel &model,
                                  int trackIndex,
                                  int clipIndex,
                                  int duration,
                                  int trimDelta,
                                  bool redo = true,
                                  QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdAddTransitionByTrimOut; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;
    int m_duration;               ///< 转场时长
    int m_trimDelta;              ///< 对后一个片段的修剪量
    bool m_notify;
    bool m_redo;
};

/// ####################################################################
/// #                        轨道管理命令                              #
/// ####################################################################

/**
 * @class AddTrackCommand
 * @brief “添加轨道”命令
 * 在时间线末尾添加一个新的视频或音频轨道。
 */
class AddTrackCommand : public QUndoCommand
{
public:
    AddTrackCommand(MultitrackModel &model, bool isVideo, QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;             ///< 新添加轨道的索引
    bool m_isVideo;               ///< 是否为视频轨道
    QUuid m_uuid;                 ///< 轨道的 UUID
};

/**
 * @class InsertTrackCommand
 * @brief “插入轨道”命令
 * 在指定位置插入一个新的视频或音频轨道。
 */
class InsertTrackCommand : public QUndoCommand
{
public:
    InsertTrackCommand(MultitrackModel &model,
                       int trackIndex,
                       TrackType trackType = PlaylistTrackType, ///< 轨道类型
                       QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    TrackType m_trackType;        ///< 轨道类型
    QUuid m_uuid;
};

/**
 * @class RemoveTrackCommand
 * @brief “移除轨道”命令
 * 从时间线中移除一个轨道，并保存其所有信息以便恢复。
 */
class RemoveTrackCommand : public QUndoCommand
{
public:
    RemoveTrackCommand(MultitrackModel &model, int trackIndex, QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    TrackType m_trackType;        ///< 保存的轨道类型
    QString m_trackName;          ///< 保存的轨道名称
    UndoHelper m_undoHelper;
    QScopedPointer<Mlt::Producer> m_filtersProducer; ///< 保存的轨道滤镜
    QUuid m_uuid;                 ///< 保存的轨道 UUID
};

/**
 * @class MoveTrackCommand
 * @brief “移动轨道”命令
 * 上移或下移轨道的顺序。
 */
class MoveTrackCommand : public QUndoCommand
{
public:
    MoveTrackCommand(MultitrackModel &model,
                     int fromTrackIndex,
                     int toTrackIndex,
                     QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_fromTrackIndex;         ///< 源轨道索引
    int m_toTrackIndex;           ///< 目标轨道索引
};

/**
 * @class ChangeBlendModeCommand
 * @brief “更改混合模式”命令
 * 更改轨道的混合模式（如正常、叠加、变暗等）。
 * 注意：此类继承自 QObject，因为它需要发射信号。
 */
class ChangeBlendModeCommand : public QObject, public QUndoCommand
{
    Q_OBJECT
public:
    ChangeBlendModeCommand(Mlt::Transition &transition,
                           const QString &propertyName,
                           const QString &mode,
                           QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;
signals:
    void modeChanged(QString &mode); ///< 当混合模式改变时发射的信号

private:
    Mlt::Transition m_transition;  ///< 轨道混合模式的转场服务
    QString m_propertyName;        ///< 控制混合模式的属性名
    QString m_newMode;             ///< 新的混合模式
    QString m_oldMode;             ///< 旧的混合模式
};

/// ####################################################################
/// #                        其他高级命令                              #
/// ####################################################################

/**
 * @class UpdateCommand
 * @brief “更新片段属性”命令
 * 一个通用命令，用于通过新的 XML 数据更新片段的属性（如滤镜、颜色等）。
 */
class UpdateCommand : public QUndoCommand
{
public:
    UpdateCommand(TimelineDock &timeline,
                  int trackIndex,
                  int clipIndex,
                  int position,
                  QUndoCommand *parent = 0);
    void setXmlAfter(const QString &xml); ///< 设置更新后的 XML
    void setPosition(int trackIndex, int clipIndex, int position); ///< 允许在执行前更新目标位置
    void setRippleAllTracks(bool);
    int trackIndex() const { return m_trackIndex; }
    int clipIndex() const { return m_clipIndex; }
    int position() const { return m_position; }
    void redo() override;
    void undo() override;

private:
    TimelineDock &m_timeline;
    int m_trackIndex;
    int m_clipIndex;
    int m_position;
    QString m_xmlAfter;           ///< 包含更新后属性的 XML
    bool m_isFirstRedo;           ///< 标记是否为首次执行
    UndoHelper m_undoHelper;
    bool m_ripple;
    bool m_rippleAllTracks;
};

/**
 * @class DetachAudioCommand
 * @brief “分离音频”命令
 * 将一个音视频片段的视频和音频部分分离到不同的轨道上。
 */
class DetachAudioCommand : public QUndoCommand
{
public:
    DetachAudioCommand(TimelineDock &timeline,
                       int trackIndex,
                       int clipIndex,
                       int position,
                       const QString &xml,
                       QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

private:
    TimelineDock &m_timeline;
    int m_trackIndex;
    int m_clipIndex;
    int m_position;
    int m_targetTrackIndex;       ///< 分离后音频片段的目标轨道索引
    QString m_xml;                ///< 原始片段的 XML
    UndoHelper m_undoHelper;
    bool m_trackAdded;            ///< 标记是否为此操作添加了新轨道
    QUuid m_uuid;
};

/**
 * @class ReplaceCommand
 * @brief “替换片段”命令
 * 用一个新的片段完全替换时间线上的一个旧片段。
 */
class ReplaceCommand : public QUndoCommand
{
public:
    ReplaceCommand(MultitrackModel &model,
                   int trackIndex,
                   int clipIndex,
                   const QString &xml,
                   QUndoCommand *parent = nullptr);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;
    QString m_xml;                ///< 新片段的 XML
    bool m_isFirstRedo;
    UndoHelper m_undoHelper;
};

/**
 * @class AlignClipsCommand
 * @brief “对齐片段”命令
 * 根据参考轨道的音频波形，将其他轨道上的片段与参考轨道对齐。
 */
class AlignClipsCommand : public QUndoCommand
{
public:
    AlignClipsCommand(MultitrackModel &model, QUndoCommand *parent = 0);
    /**
     * @brief 添加一个对齐信息。
     * @param uuid 要对齐的片段的 UUID。
     * @param offset 对齐后的新位置（时间偏移量）。
     * @param speedCompensation 对齐后的新速度（用于变速对齐）。
     */
    void addAlignment(QUuid uuid, int offset, double speedCompensation);
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    UndoHelper m_undoHelper;
    bool m_redo;
    /**
     * @brief 一个结构体，用于存储单个片段的对齐信息。
     */
    struct Alignment
    {
        QUuid uuid;               ///< 片段的唯一标识符
        int offset;                ///< 新的起始位置
        double speed;              ///< 新的速度
    };
    QVector<Alignment> m_alignments; ///< 所有待对齐片段的信息列表
};

/**
 * @class ApplyFiltersCommand
 * @brief “应用复制的滤镜”命令
 * 将一个或多个滤镜从一个片段复制并应用到其他选定的片段上。
 */
class ApplyFiltersCommand : public QUndoCommand
{
public:
    ApplyFiltersCommand(MultitrackModel &model,
                        const QString &filterProducerXml, ///< 包含待应用滤镜的 XML
                        QUndoCommand *parent = 0);
    void addClip(int trackIndex, int clipIndex); ///< 添加一个目标片段
    void redo() override;
    void undo() override;

private:
    MultitrackModel &m_model;
    QString m_xml;
    QMap<ClipPosition, QString> m_prevFilters; ///< 保存每个目标片段应用滤镜前的 XML
};

/**
 * @class ChangeGainCommand
 * @brief “调整增益/音量”命令
 * 更改音频片段的增益（音量）。
 */
class ChangeGainCommand : public QUndoCommand
{
public:
    ChangeGainCommand(MultitrackModel &model,
                      int trackIndex,
                      int clipIndex,
                      double gain,
                      QUndoCommand *parent = 0);
    void redo() override;
    void undo() override;

protected:
    int id() const override { return UndoIdChangeGain; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    MultitrackModel &m_model;
    int m_trackIndex;
    int m_clipIndex;
    double m_gain;                ///< 新的增益值
    double m_previous;            ///< 旧的增益值
};

} // namespace Timeline

#endif // COMMANDS_H
