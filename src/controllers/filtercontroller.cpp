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

#include "filtercontroller.h"

#include "Logger.h"
#include "mltcontroller.h"
#include "qmltypes/qmlapplication.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlmetadata.h"
#include "qmltypes/qmlutilities.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"

#include <MltLink.h>
#include <QDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTimerEvent>

/**
 * @class FilterController
 * @brief 滤镜控制器
 * 
 * 这是管理 Shotcut 中所有滤镜功能的核心类。它的主要职责包括：
 * 1. 在启动时扫描并加载所有可用的滤镜元数据（从 QML 文件）。
 * 2. 管理当前附加到某个 Producer（片段、轨道等）上的滤镜列表。
 * 3. 处理当前选中的滤镜，提供其 QML 接口，并响应其属性变化。
 * 4. 与撤销/重做系统集成，暂停和恢复滤镜的撤销跟踪。
 * 5. 处理滤镜的添加、移除和更新。
 */

FilterController::FilterController(QObject *parent)
    : QObject(parent)
    , m_metadataModel(this) // 元数据模型，存储所有可用滤镜的信息
    , m_attachedModel(this) // 附加滤镜模型，存储当前 Producer 上的滤镜列表
    , m_currentFilterIndex(QmlFilter::NoCurrentFilter) // 初始化时没有选中的滤镜
{
    startTimer(0); // 启动一个 0 毫秒的定时器，以便在事件循环启动后立即加载数据
    // 连接附加滤镜模型的信号，以便在其变化时做出响应
    connect(&m_attachedModel, SIGNAL(changed()), this, SLOT(handleAttachedModelChange()));
    connect(&m_attachedModel,
            SIGNAL(modelAboutToBeReset()),
            this,
            SLOT(handleAttachedModelAboutToReset()));
    connect(&m_attachedModel,
            SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
            this,
            SLOT(handleAttachedRowsAboutToBeRemoved(const QModelIndex &, int, int)));
    connect(&m_attachedModel,
            SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            this,
            SLOT(handleAttachedRowsRemoved(const QModelIndex &, int, int)));
    connect(&m_attachedModel,
            SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this,
            SLOT(handleAttachedRowsInserted(const QModelIndex &, int, int)));
    connect(&m_attachedModel,
            SIGNAL(duplicateAddFailed(int)),
            this,
            SLOT(handleAttachDuplicateFailed(int)));
}

/**
 * @brief 从 QML 文件加载所有滤镜的元数据。
 * 这个函数在构造函数中通过定时器调用，以确保在 QML 引擎完全准备好后执行。
 */
void FilterController::loadFilterMetadata()
{
    // 获取 MLT 中所有可用的滤镜、链接和制作者服务
    QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
    QScopedPointer<Mlt::Properties> mltLinks(MLT.repository()->links());
    QScopedPointer<Mlt::Properties> mltProducers(MLT.repository()->producers());
    // 定位到存放滤镜元数据 QML 文件的目录
    QDir dir = QmlUtilities::qmlDir();
    dir.cd("filters");
    // 遍历每个滤镜的子目录
    foreach (QString dirName,
             dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable)) {
        QDir subdir = dir;
        subdir.cd(dirName);
        subdir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        subdir.setNameFilters(QStringList("meta*.qml")); // 查找名为 meta*.qml 的元数据文件
        foreach (QString fileName, subdir.entryList()) {
            LOG_DEBUG() << "reading filter metadata" << dirName << fileName;
            // 使用 QML 引擎创建元数据对象
            QQmlComponent component(QmlUtilities::sharedEngine(), subdir.absoluteFilePath(fileName));
            QmlMetadata *meta = qobject_cast<QmlMetadata *>(component.create());
            if (meta) {
                // 检查 MLT 版本兼容性
                QScopedPointer<Mlt::Properties> mltMetadata(
                    MLT.repository()->metadata(mlt_service_filter_type,
                                               meta->mlt_service().toLatin1().constData()));
                QString version;
                if (mltMetadata && mltMetadata->is_valid() && mltMetadata->get("version")) {
                    version = QString::fromLatin1(mltMetadata->get("version"));
                    if (version.startsWith("lavfi"))
                        version.remove(0, 5);
                }

                // 检查 mlt_service 是否在 MLT 中可用，以及特殊依赖（如 glaxnimate）是否存在
                if (mltFilters->get_data(meta->mlt_service().toLatin1().constData()) &&
                    ("maskGlaxnimate" != meta->objectName() || mltProducers->get_data("glaxnimate"))
                    && (version.isEmpty() || meta->isMltVersion(version))) {
                    LOG_DEBUG() << "added filter" << meta->name();
                    meta->loadSettings(); // 加载滤镜的默认设置
                    meta->setPath(subdir);
                    meta->setParent(0);
                    addMetadata(meta); // 将元数据添加到模型中

                    // 检查关键帧动画是否需要特定版本
                    if (!version.isEmpty() && meta->keyframes()) {
                        meta->setProperty("version", version);
                        meta->keyframes()->checkVersion(version);
                    }
                } else if (meta->type() == QmlMetadata::Link
                           && mltLinks->get_data(meta->mlt_service().toLatin1().constData())) {
                    // 处理链接类型
                    LOG_DEBUG() << "added link" << meta->name();
                    meta->loadSettings();
                    meta->setPath(subdir);
                    meta->setParent(0);
                    addMetadata(meta);
                }

                // 如果滤镜已弃用，在其名称后添加标记
                if (meta->isDeprecated())
                    meta->setName(meta->name() + " " + tr("(DEPRECATED)"));
            } else if (!meta) {
                LOG_WARNING() << component.errorString(); // 打印 QML 创建错误
            }
        }
    };
}

/// ... (其他公共和私有方法的实现，如 metadata, metadataForService, isOutputTrackSelected 等)

/**
 * @brief 当发生撤销或重做操作时调用。
 * @param service 受影响的 MLT 服务。
 */
void FilterController::onUndoOrRedo(Mlt::Service &service)
{
    MLT.refreshConsumer(); // 刷新播放器以显示结果
    // 如果撤销/重做影响了当前选中的滤镜，则刷新其 UI
    if (m_currentFilter && m_mltService.is_valid()
        && service.get_service() == m_mltService.get_service()) {
        emit undoOrRedo();
        // 使用异步调用确保 UI 能正确更新
        QMetaObject::invokeMethod(this,
                                  "setCurrentFilter",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, m_currentFilterIndex));
    }
}

/**
 * @brief 定时器事件，用于在构造后加载数据。
 */
void FilterController::timerEvent(QTimerEvent *event)
{
    loadFilterMetadata();
    loadFilterSets(); // 加载滤镜组合
    killTimer(event->timerId()); // 执行一次后停止定时器
}

/// ... (其他 getter 方法，如 metadataModel, attachedModel)

/**
 * @brief 设置当前正在操作的 Producer（例如，时间线上选中的片段）。
 * @param producer 指向 MLT Producer 的指针。
 */
void FilterController::setProducer(Mlt::Producer *producer)
{
    m_attachedModel.setProducer(producer); // 通知附加滤镜模型更新其列表
    if (producer && producer->is_valid()) {
        // 根据 Producer 的类型更新元数据模型的可见性掩码
        m_metadataModel.updateFilterMask(!MLT.isTrackProducer(*producer),
                                         producer->type() == mlt_service_chain_type,
                                         producer->type() == mlt_service_playlist_type,
                                         producer->type() == mlt_service_tractor_type,
                                         producer->get("mlt_service") != QString("xml-clip"));
    } else {
        // 如果没有 Producer，则取消选择当前滤镜
        setCurrentFilter(QmlFilter::DeselectCurrentFilter);
    }
}

/**
 * @brief 设置当前选中的滤镜。
 * @param attachedIndex 在附加滤镜模型中的索引。
 */
void FilterController::setCurrentFilter(int attachedIndex)
{
    if (attachedIndex == m_currentFilterIndex) {
        return;
    }
    m_currentFilterIndex = attachedIndex;

    // 某些滤镜的 UI（如富文本）可能会自行渲染，需要告诉 MLT 不要隐藏它
    if (m_mltService.is_valid()) {
        if (m_mltService.get_int("_hide")) {
            m_mltService.clear("_hide");
            MLT.refreshConsumer();
        }
    }

    // 获取新选中滤镜的元数据
    QmlMetadata *meta = m_attachedModel.getMetadata(m_currentFilterIndex);
    QmlFilter *filter = 0;
    if (meta) {
        emit currentFilterChanged(nullptr, nullptr, QmlFilter::NoCurrentFilter); // 先通知旧滤镜已取消
        m_mltService = m_attachedModel.getService(m_currentFilterIndex); // 获取 MLT 服务
        if (!m_mltService.is_valid())
            return;
        // 创建一个新的 QmlFilter 对象来封装 MLT 服务和元数据
        filter = new QmlFilter(m_mltService, meta);
        filter->setIsNew(m_mltService.get_int(kNewFilterProperty)); // 检查是否为新添加的滤镜
        m_mltService.clear(kNewFilterProperty);
        connect(filter, SIGNAL(changed(QString)), SLOT(onQmlFilterChanged(const QString &)));
    }

    // 发出信号，通知 UI 当前滤镜已更改
    emit currentFilterChanged(filter, meta, m_currentFilterIndex);
    m_currentFilter.reset(filter); // 使用智能指针管理 QmlFilter 对象的生命周期
    // 如果不是源片段，则开始撤销跟踪
    if (filter && !m_attachedModel.isSourceClip()) {
        filter->startUndoTracking();
    }
}

/// ... (其他槽函数，如 onGainChanged, onFadeInChanged, onFadeOutChanged 等)

/**
 * @brief 处理附加滤镜模型的变化（例如滤镜被禁用）。
 */
void FilterController::handleAttachedModelChange()
{
    if (m_currentFilter) {
        emit m_currentFilter->changed("disable");
    }
}

/**
 * @brief 处理附加滤镜模型即将重置的信号。
 */
void FilterController::handleAttachedModelAboutToReset()
{
    setCurrentFilter(QmlFilter::NoCurrentFilter); // 取消选择当前滤镜
}

/**
 * @brief 处理滤镜行被移除后的信号。
 */
void FilterController::handleAttachedRowsRemoved(const QModelIndex &, int first, int)
{
    m_currentFilterIndex = QmlFilter::DeselectCurrentFilter; // 强制更新
    // 尝试选择一个合理的相邻滤镜
    setCurrentFilter(qBound(0, first, qMax(m_attachedModel.rowCount() - 1, 0)));
}

/**
 * @brief 处理滤镜行被插入后的信号。
 */
void FilterController::handleAttachedRowsInserted(const QModelIndex &, int first, int)
{
    m_currentFilterIndex = QmlFilter::DeselectCurrentFilter; // 强制更新
    // 通常会选择新插入的滤镜
    setCurrentFilter(qBound(0, first, qMax(m_attachedModel.rowCount() - 1, 0)));
}

/**
 * @brief 处理添加重复滤镜失败的情况。
 */
void FilterController::handleAttachDuplicateFailed(int index)
{
    const QmlMetadata *meta = m_attachedModel.getMetadata(index);
    emit statusChanged(tr("Only one %1 filter is allowed.").arg(meta->name()));
    setCurrentFilter(index);
}

/**
 * @brief 当 QML 滤镜的属性发生变化时调用。
 */
void FilterController::onQmlFilterChanged(const QString &name)
{
    if (name == "disable") {
        // 如果是禁用状态改变，更新附加模型中对应项的复选框状态
        QModelIndex index = m_attachedModel.index(m_currentFilterIndex);
        emit m_attachedModel.dataChanged(index, index, QVector<int>() << Qt::CheckStateRole);
    }
    emit filterChanged(&m_mltService); // 通知其他部分滤镜已更改
}

/**
 * @brief 移除当前选中的滤镜。
 */
void FilterController::removeCurrent()
{
    if (m_currentFilterIndex > QmlFilter::NoCurrentFilter)
        m_attachedModel.remove(m_currentFilterIndex);
}

/// ... (其他方法，如 pauseUndoTracking, resumeUndoTracking, addMetadata)

/**
 * @brief 添加或编辑一个滤镜。
 * 如果已存在具有相同关键属性的滤镜，则编辑它；否则，添加一个新的。
 */
void FilterController::addOrEditFilter(Mlt::Filter *filter, const QStringList &key_properties)
{
    int rows = m_attachedModel.rowCount();
    int serviceIndex = -1;
    // 遍历已附加的滤镜，查找匹配项
    for (int i = 0; i < rows; i++) {
        QScopedPointer<Mlt::Service> service(m_attachedModel.getService(i));
        bool servicesMatch = true;
        if (metadataForService(service.data())->uniqueId()
            != metadataForService(filter)->uniqueId()) {
            continue;
        }
        // 检查所有关键属性是否都匹配
        for (auto &k : key_properties) {
            const auto keyByteArray = k.toUtf8();
            const char *key = keyByteArray.constData();
            if (!service->property_exists(key) || !filter->property_exists(key)) {
                servicesMatch = false;
                break;
            } else if (QString(service->get(key)) != QString(filter->get(key))) {
                servicesMatch = false;
                break;
            }
        }
        if (servicesMatch) {
            serviceIndex = i;
            break;
        }
    }
    if (serviceIndex < 0) {
        // 如果没有找到匹配的滤镜，则添加新的
        serviceIndex = m_attachedModel.addService(filter);
    }
    setCurrentFilter(serviceIndex); // 选中新添加或编辑的滤镜
}

/**
 * @brief 根据轨道转场服务设置混合模式的可见性。
 * @param service 转场服务的名称（如 "qtblend" 或 "frei0r.cairoblend"）。
 */
void FilterController::setTrackTransitionService(const QString &service)
{
    if (service == QStringLiteral("qtblend")) {
        m_metadataModel.setHidden("qtBlendMode", false); // 显示 Qt 混合模式
        m_metadataModel.setHidden("blendMode", true);    // 隐藏 frei0r 混合模式
    } else if (service == QStringLiteral("frei0r.cairoblend")) {
        m_metadataModel.setHidden("qtBlendMode", true);
        m_metadataModel.setHidden("blendMode", false);
    } else {
        m_metadataModel.setHidden("qtBlendMode", true);
        m_metadataModel.setHidden("blendMode", true);
    }
}
