/*
 * Copyright (c) 2014-2025 Meltytech, LLC
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

#ifndef FILTERCONTROLLER_H
#define FILTERCONTROLLER_H

#include "models/attachedfiltersmodel.h"
#include "models/metadatamodel.h"
#include "models/motiontrackermodel.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlmetadata.h"

#include <QFuture>
#include <QObject>
#include <QScopedPointer>

class QTimerEvent;

/**
 * @class FilterController
 * @brief 滤镜控制器
 * 
 * 这是管理 Shotcut 中所有滤镜功能的核心类。它作为 C++ 后端和 QML 前端之间的桥梁，
 * 负责加载滤镜元数据、管理当前选中的滤镜以及处理滤镜与时间线/片段的交互。
 * 
 * 主要功能：
 * - 提供对所有可用滤镜元数据的访问（`metadataModel`）。
 * - 管理当前附加到某个 Producer 上的滤镜列表（`attachedModel`）。
 * - 提供对当前选中滤镜的访问（`currentFilter`）。
 * - 处理滤镜的添加、移除和更新。
 * - 与撤销/重做系统集成。
 */
class FilterController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数。
     * @param parent 父 QObject。
     */
    explicit FilterController(QObject *parent = 0);

    /**
     * @brief 返回元数据模型。
     * @return 指向 MetadataModel 的指针，该模型包含所有可用滤镜的信息。
     */
    MetadataModel *metadataModel();

    /**
     * @brief 返回运动跟踪模型。
     * @return 指向 MotionTrackerModel 的指针。
     */
    MotionTrackerModel *motionTrackerModel() { return &m_motionTrackerModel; }

    /**
     * @brief 返回附加滤镜模型。
     * @return 指向 AttachedFiltersModel 的指针，该模型包含当前 Producer 上的所有滤镜。
     */
    AttachedFiltersModel *attachedModel();

    /**
     * @brief 根据唯一 ID 获取滤镜元数据。
     * @param id 滤镜的唯一标识符。
     * @return 指向 QmlMetadata 的指针，如果未找到则返回 nullptr。
     */
    QmlMetadata *metadata(const QString &id);

    /**
     * @brief 根据 MLT 服务获取滤镜元数据。
     * @param service 指向 MLT 服务的指针。
     * @return 指向 QmlMetadata 的指针。
     */
    QmlMetadata *metadataForService(Mlt::Service *service);

    /**
     * @brief 返回当前选中的滤镜对象。
     * @return 指向 QmlFilter 的智能指针，如果没有选中滤镜则为 nullptr。
     */
    QmlFilter *currentFilter() const { return m_currentFilter.data(); }

    /**
     * @brief 检查是否选中了输出轨道（即主时间线轨道）。
     * @return 如果选中了输出轨道且其上没有滤镜，则返回 true。
     */
    bool isOutputTrackSelected() const;

    /**
     * @brief 当发生撤销或重做操作时调用。
     * @param service 受影响的 MLT 服务。
     */
    void onUndoOrRedo(Mlt::Service &service);

    /**
     * @brief 返回当前选中滤镜在附加模型中的索引。
     * @return 当前滤镜的索引。
     */
    int currentIndex() const { return m_currentFilterIndex; }

    /**
     * @brief 添加或编辑一个滤镜。
     * 如果已存在具有相同关键属性的滤镜，则编辑它；否则，添加一个新的。
     * @param filter 要添加或编辑的 MLT 滤镜。
     * @param key_properties 用于判断是否为同一滤镜的关键属性列表。
     */
    void addOrEditFilter(Mlt::Filter *filter, const QStringList &key_properties);

    /**
     * @brief 根据轨道转场服务设置混合模式的可见性。
     * @param service 转场服务的名称。
     */
    void setTrackTransitionService(const QString &service);

protected:
    /**
     * @brief 重写 QObject 的定时器事件。
     * 用于在事件循环启动后立即加载滤镜元数据。
     */
    void timerEvent(QTimerEvent *) override;

signals:
    /**
     * @brief 当前选中的滤镜发生变化时发射。
     * @param filter 新的当前滤镜对象。
     * @param meta 新的当前滤镜的元数据。
     * @param index 新的当前滤镜的索引。
     */
    void currentFilterChanged(QmlFilter *filter, QmlMetadata *meta, int index);

    /**
     * @brief 状态消息发生变化时发射（例如，添加重复滤镜失败）。
     * @param 状态消息字符串。
     */
    void statusChanged(QString);

    /**
     * @brief 当前滤镜的属性发生变化时发射。
     * @param 指向已更改的 MLT 服务的指针。
     */
    void filterChanged(Mlt::Service *);

    /**
     * @brief 发生了撤销或重做操作时发射。
     */
    void undoOrRedo();

public slots:
    /**
     * @brief 设置当前正在操作的 Producer（例如，时间线上选中的片段）。
     * @param producer 指向 MLT Producer 的指针。
     */
    void setProducer(Mlt::Producer *producer = 0);

    /**
     * @brief 设置当前选中的滤镜。
     * @param attachedIndex 在附加滤镜模型中的索引。
     */
    void setCurrentFilter(int attachedIndex);

    /**
     * @brief 当淡入效果发生变化时调用。
     */
    void onFadeInChanged();

    /**
     * @brief 当淡出效果发生变化时调用。
     */
    void onFadeOutChanged();

    /**
     * @brief 当增益（音量）发生变化时调用。
     */
    void onGainChanged();

    /**
     * @brief 当服务的入点发生变化时调用。
     * @param delta 入点的变化量。
     * @param service 受影响的服务，如果为 nullptr，则应用于当前滤镜。
     */
    void onServiceInChanged(int delta, Mlt::Service *service = 0);

    /**
     * @brief 当服务的出点发生变化时调用。
     * @param delta 出点的变化量。
     * @param service 受影响的服务，如果为 nullptr，则应用于当前滤镜。
     */
    void onServiceOutChanged(int delta, Mlt::Service *service = 0);

    /**
     * @brief 移除当前选中的滤镜。
     */
    void removeCurrent();

    /**
     * @brief 当 Producer 发生变化时调用（例如，轨道名称改变）。
     */
    void onProducerChanged();

    /**
     * @brief 暂停当前滤镜的撤销跟踪。
     * 在执行一系列不应被记录为单独撤销步骤的操作时使用。
     */
    void pauseUndoTracking();

    /**
     * @brief 恢复当前滤镜的撤销跟踪。
     */
    void resumeUndoTracking();

private slots:
    /**
     * @brief 处理附加滤镜模型的变化（例如滤镜被禁用）。
     */
    void handleAttachedModelChange();

    /**
     * @brief 处理附加滤镜模型即将重置的信号。
     */
    void handleAttachedModelAboutToReset();

    /**
     * @brief 向元数据模型添加一个新的元数据对象。
     * @param meta 要添加的 QmlMetadata 对象。
     */
    void addMetadata(QmlMetadata *);

    /**
     * @brief 处理附加滤镜模型中行即将被移除的信号。
     */
    void handleAttachedRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);

    /**
     * @brief 处理附加滤镜模型中行被移除后的信号。
     */
    void handleAttachedRowsRemoved(const QModelIndex &parent, int first, int last);

    /**
     * @brief 处理附加滤镜模型中行被插入后的信号。
     */
    void handleAttachedRowsInserted(const QModelIndex &parent, int first, int last);

    /**
     * @brief 处理添加重复滤镜失败的情况。
     * @param index 失败的滤镜索引。
     */
    void handleAttachDuplicateFailed(int index);

    /**
     * @brief 当 QML 滤镜的属性发生变化时调用。
     * @param name 发生变化的属性名。
     */
    void onQmlFilterChanged(const QString &name);

private:
    /**
     * @brief 从文件加载滤镜组合。
     */
    void loadFilterSets();

    /**
     * @brief 从 QML 文件加载所有滤镜的元数据。
     */
    void loadFilterMetadata();

    QFuture<void> m_future; ///< 用于异步加载的 QFuture 对象（在代码中未使用）。
    QScopedPointer<QmlFilter> m_currentFilter; ///< 当前选中的滤镜对象，使用智能指针管理。
    Mlt::Service m_mltService; ///< 当前选中滤镜的 MLT 服务。
    MetadataModel m_metadataModel; ///< 元数据模型实例。
    MotionTrackerModel m_motionTrackerModel; ///< 运动跟踪模型实例。
    AttachedFiltersModel m_attachedModel; ///< 附加滤镜模型实例。
    int m_currentFilterIndex; ///< 当前选中滤镜在附加模型中的索引。
};

#endif // FILTERCONTROLLER_H
