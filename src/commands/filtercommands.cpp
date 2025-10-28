/*
 * Copyright (c) 2021-2025 Meltytech, LLC
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

#include "filtercommands.h"

#include "Logger.h"
#include "controllers/filtercontroller.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "qmltypes/qmlapplication.h"

/**
 * @class FindProducerParser
 * @brief 一个自定义的 MLT 解析器，用于在整个项目结构中根据 UUID 查找特定的 Producer。
 *
 * MLT 的服务（如 Producer, Playlist, Tractor）可以嵌套构成一个复杂的图结构。
 * 这个类通过遍历这个图来找到一个具有特定 UUID 的服务。
 */
class FindProducerParser : public Mlt::Parser
{
private:
    QUuid m_uuid;      ///< 要查找的目标 UUID
    Mlt::Producer m_producer; ///< 找到的 Producer 对象

public:
    /// 构造函数，接收要查找的 UUID
    FindProducerParser(QUuid uuid)
        : Mlt::Parser()
        , m_uuid(uuid)
    {}

    /// 返回找到的 Producer
    Mlt::Producer producer() { return m_producer; }

    // 以下是 Mlt::Parser 的虚函数重写
    // 我们只关心 Producer, Playlist, Tractor, Chain，因为它们都继承自 Producer
    // 并且可以附加滤镜。

    int on_start_filter(Mlt::Filter *) { return 0; } // 不关心滤镜
    int on_start_producer(Mlt::Producer *producer)
    {
        // 当遇到一个 Producer 时，检查其 UUID 是否匹配
        if (MLT.uuid(*producer) == m_uuid) {
            m_producer = producer; // 匹配成功，保存它
            return 1;              // 返回 1 停止继续解析
        }
        return 0; // 继续解析
    }
    int on_end_producer(Mlt::Producer *) { return 0; }
    // Playlist, Tractor, Chain 也是 Producer 的一种，所以它们的处理逻辑与 on_start_producer 相同
    int on_start_playlist(Mlt::Playlist *playlist) { return on_start_producer(playlist); }
    int on_end_playlist(Mlt::Playlist *) { return 0; }
    int on_start_tractor(Mlt::Tractor *tractor) { return on_start_producer(tractor); }
    int on_end_tractor(Mlt::Tractor *) { return 0; }
    int on_start_multitrack(Mlt::Multitrack *) { return 0; } // 不关心
    int on_end_multitrack(Mlt::Multitrack *) { return 0; }
    int on_start_track() { return 0; }
    int on_end_track() { return 0; }
    int on_end_filter(Mlt::Filter *) { return 0; }
    int on_start_transition(Mlt::Transition *) { return 0; } // 不关心
    int on_end_transition(Mlt::Transition *) { return 0; }
    int on_start_chain(Mlt::Chain *chain) { return on_start_producer(chain); }
    int on_end_chain(Mlt::Chain *) { return 0; }
    int on_start_link(Mlt::Link *) { return 0; } // 不关心
    int on_end_link(Mlt::Link *) { return 0; }
};

/**
 * @brief 全局辅助函数，用于在整个项目中查找具有指定 UUID 的 Producer。
 *
 * 它会依次在多轨道、播放列表和当前选中的片段中查找。
 * @param uuid 要查找的 Producer 的 UUID。
 * @return 找到的 Mlt::Producer 对象，如果未找到则返回无效对象。
 */
static Mlt::Producer findProducer(const QUuid &uuid)
{
    FindProducerParser graphParser(uuid);
    // 1. 首先在多轨道中查找
    if (MAIN.isMultitrackValid()) {
        graphParser.start(*MAIN.multitrack());
        if (graphParser.producer().is_valid()) {
            return graphParser.producer();
        }
    }
    // 2. 然后在主播放列表中查找
    if (MAIN.playlist() && MAIN.playlist()->count() > 0) {
        graphParser.start(*MAIN.playlist());
        if (graphParser.producer().is_valid()) {
            return graphParser.producer();
        }
    }
    // 3. 最后在当前源片段中查找
    Mlt::Producer producer(MLT.isClip() ? MLT.producer() : MLT.savedProducer());
    if (producer.is_valid()) {
        graphParser.start(producer);
        if (graphParser.producer().is_valid()) {
            return graphParser.producer();
        }
    }
    return Mlt::Producer(); // 未找到，返回空 Producer
}

namespace Filter {

/**
 * @class AddCommand
 * @brief 撤销/重做“添加滤镜”操作的命令。
 *
 * 支持添加单个滤镜或一组滤镜（滤镜集）。
 */
AddCommand::AddCommand(AttachedFiltersModel &model,
                       const QString &name,
                       Mlt::Service &service,
                       int row,
                       AddCommand::AddType type,
                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_producer(*model.producer()) // 初始时持有 Producer 的引用
    , m_producerUuid(MLT.ensureHasUuid(m_producer)) // 获取并确保 Producer 有 UUID
    , m_type(type)
{
    // 根据类型设置撤销/重做历史中显示的文本
    if (m_type == AddCommand::AddSingle) {
        setText(QObject::tr("Add %1 filter").arg(name));
    } else {
        setText(QObject::tr("Add %1 filter set").arg(name));
    }
    // 保存要添加的服务和它们的目标行号
    m_rows.push_back(row);
    m_services.push_back(service);
}

/// 执行“添加滤镜”操作
void AddCommand::redo()
{
    LOG_DEBUG() << text() << m_rows[0];
    Mlt::Producer producer = m_producer;
    // 如果 m_producer 无效（例如在第一次 redo 之后），则通过 UUID 重新查找
    if (!producer.is_valid()) {
        producer = findProducer(m_producerUuid);
    }
    Q_ASSERT(producer.is_valid());
    // 记录添加前的滤镜数量，用于后续调整（例如元数据）
    int adjustFrom = producer.filter_count();
    // 遍历并添加所有服务
    for (int i = 0; i < m_rows.size(); i++) {
        m_model.doAddService(producer, m_services[i], m_rows[i]);
    }
    // 如果是添加滤镜集的最后一个滤镜，则执行一些调整操作
    if (AddSetLast == m_type)
        MLT.adjustFilters(producer, adjustFrom);
    // 在第一次 redo 后，释放对 Producer 的直接引用，后续都通过 UUID 查找
    m_producer = Mlt::Producer();
}

/// 撤销“添加滤镜”操作
void AddCommand::undo()
{
    LOG_DEBUG() << text() << m_rows[0];
    // 通过 UUID 查找 Producer
    Mlt::Producer producer(findProducer(m_producerUuid));
    Q_ASSERT(producer.is_valid());
    // 以相反的顺序移除服务，以保证索引的正确性
    for (int i = m_rows.size() - 1; i >= 0; i--) {
        m_model.doRemoveService(producer, m_rows[i]);
    }
}

/**
 * @brief 合并多个连续的“添加滤镜集”命令。
 *
 * 这样，在添加一个包含多个滤镜的滤镜集时，撤销/重做历史中只会显示一个条目。
 */
bool AddCommand::mergeWith(const QUndoCommand *other)
{
    AddCommand *that = const_cast<AddCommand *>(static_cast<const AddCommand *>(other));
    if (!that || that->id() != id()) {
        LOG_ERROR() << "Invalid merge";
        return false;
    }
    // 只有当两个命令都是添加滤镜集时才合并
    if (m_type != AddSet || !(that->m_type == AddSet || that->m_type == AddSetLast)) {
        return false;
    }
    // 将另一个命令的信息合并到当前命令中
    m_type = that->m_type; // 更新类型（例如，从 AddSet 变为 AddSetLast）
    m_rows.push_back(that->m_rows.front());
    m_services.push_back(that->m_services.front());
    return true;
}

/**
 * @class RemoveCommand
 * @brief 撤销/重做“删除滤镜”操作的命令。
 */
RemoveCommand::RemoveCommand(AttachedFiltersModel &model,
                             const QString &name,
                             Mlt::Service &service,
                             int row,
                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
    , m_producer(*model.producer())
    , m_producerUuid(MLT.ensureHasUuid(m_producer))
    , m_service(service) // 保存被删除的服务，以便 undo 时恢复
{
    setText(QObject::tr("Remove %1 filter").arg(name));
}

/// 执行“删除滤镜”操作
void RemoveCommand::redo()
{
    LOG_DEBUG() << text() << m_row;
    Mlt::Producer producer = m_producer;
    if (!producer.is_valid()) {
        producer = findProducer(m_producerUuid);
    }
    Q_ASSERT(producer.is_valid());
    m_model.doRemoveService(producer, m_row);
    m_producer = Mlt::Producer(); // 释放引用
}

/// 撤销“删除滤镜”操作（即重新添加滤镜）
void RemoveCommand::undo()
{
    Q_ASSERT(m_service.is_valid());
    LOG_DEBUG() << text() << m_row;
    Mlt::Producer producer(findProducer(m_producerUuid));
    Q_ASSERT(producer.is_valid());
    m_model.doAddService(producer, m_service, m_row);
}

/**
 * @class MoveCommand
 * @brief 撤销/重做“移动滤镜”操作的命令。
 */
MoveCommand::MoveCommand(
    AttachedFiltersModel &model, const QString &name, int fromRow, int toRow, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_fromRow(fromRow)
    , m_toRow(toRow)
    , m_producer(*model.producer())
    , m_producerUuid(MLT.ensureHasUuid(m_producer))
{
    setText(QObject::tr("Move %1 filter").arg(name));
}

/// 执行“移动滤镜”操作
void MoveCommand::redo()
{
    LOG_DEBUG() << text() << "from" << m_fromRow << "to" << m_toRow;
    Mlt::Producer producer = m_producer;
    if (!producer.is_valid()) {
        producer = findProducer(m_producerUuid);
    }
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_model.doMoveService(producer, m_fromRow, m_toRow);
    }
    if (m_producer.is_valid()) {
        m_producer = Mlt::Producer(); // 释放引用
    }
}

/// 撤销“移动滤镜”操作（即移回原位）
void MoveCommand::undo()
{
    LOG_DEBUG() << text() << "from" << m_toRow << "to" << m_fromRow;
    Mlt::Producer producer(findProducer(m_producerUuid));
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_model.doMoveService(producer, m_toRow, m_fromRow);
    }
}

/**
 * @class DisableCommand
 * @brief 撤销/重做“禁用/启用滤镜”操作的命令。
 */
DisableCommand::DisableCommand(
    AttachedFiltersModel &model, const QString &name, int row, bool disabled, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
    , m_producer(*model.producer())
    , m_producerUuid(MLT.ensureHasUuid(m_producer))
    , m_disabled(disabled) // 保存目标状态
{
    // 根据目标状态设置显示文本
    if (disabled) {
        setText(QObject::tr("Disable %1 filter").arg(name));
    } else {
        setText(QObject::tr("Enable %1 filter").arg(name));
    }
}

/// 执行“禁用/启用滤镜”操作
void DisableCommand::redo()
{
    LOG_DEBUG() << text() << m_row;
    Mlt::Producer producer = m_producer;
    if (!producer.is_valid()) {
        producer = findProducer(m_producerUuid);
    }
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_model.doSetDisabled(producer, m_row, m_disabled);
    }
    if (m_producer.is_valid()) {
        m_producer = Mlt::Producer();
    }
}

/// 撤销“禁用/启用滤镜”操作（即恢复到相反状态）
void DisableCommand::undo()
{
    LOG_DEBUG() << text() << m_row;
    Mlt::Producer producer(findProducer(m_producerUuid));
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_model.doSetDisabled(producer, m_row, !m_disabled); // 恢复到相反状态
    }
}

/**
 * @brief 合并连续的禁用/启用操作。
 *
 * 注释中解释了为什么这个功能被禁用：合并两次切换会导致撤销后的状态与用户预期不符。
 * 因此，这里直接返回 false，不进行合并。
 */
bool DisableCommand::mergeWith(const QUndoCommand *other)
{
    // TODO: This doesn't always provide expected results.
    // If you toggle twice and then undo, you get the opposite of the original state.
    // It would make sense to merge three toggles in a row, but not two.
    // Do not implement for now.
    return false;
    /*
        DisableCommand *that = const_cast<DisableCommand *>(static_cast<const DisableCommand *>(other));
        if (!that || that->id() != id())
            return false;
        m_disabled = that->m_disabled;
        setText(that->text());
        return true;
    */
}

/**
 * @class PasteCommand
 * @brief 撤销/重做“粘贴滤镜”操作的命令。
 *
 * 它通过保存操作前后的 XML 状态来实现撤销。
 */
PasteCommand::PasteCommand(AttachedFiltersModel &model,
                           const QString &filterProducerXml,
                           QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_xml(filterProducerXml) // 要粘贴的滤镜的 XML
    , m_producerUuid(MLT.ensureHasUuid(*model.producer()))
{
    setText(QObject::tr("Paste filters"));
    // 保存粘贴前的所有滤镜状态到 XML，用于 undo
    m_beforeXml = MLT.XML(model.producer());
}

/// 执行“粘贴滤镜”操作
void PasteCommand::redo()
{
    LOG_DEBUG() << text();
    Mlt::Producer producer = findProducer(m_producerUuid);
    Q_ASSERT(producer.is_valid());
    // 从 XML 字符串创建一个临时的 Producer，它包含了所有要粘贴的滤镜
    Mlt::Profile profile(kDefaultMltProfile);
    Mlt::Producer filtersProducer(profile, "xml-string", m_xml.toUtf8().constData());
    if (filtersProducer.is_valid() && filtersProducer.filter_count() > 0) {
        // 使用 MLT 的 pasteFilters 函数将滤镜复制到目标 Producer
        MLT.pasteFilters(&producer, &filtersProducer);
    }
    // 发出信号，通知 QML 界面滤镜已粘贴
    emit QmlApplication::singleton().filtersPasted(&producer);
}

/// 撤销“粘贴滤镜”操作
void PasteCommand::undo()
{
    LOG_DEBUG() << text();
    Mlt::Producer producer = findProducer(m_producerUuid);
    Q_ASSERT(producer.is_valid());
    // 撤销逻辑：先清空所有滤镜，然后恢复到粘贴前的状态
    // 1. 移除所有滤镜（除了内部加载器和隐藏的滤镜）
    for (int i = 0; i < producer.filter_count(); i++) {
        Mlt::Filter *filter = producer.filter(i);
        if (filter && filter->is_valid() && !filter->get_int("_loader")
            && !filter->get_int(kShotcutHiddenProperty)) {
            producer.detach(*filter);
            i--; // 因为 detach 后索引会变化，所以 i--
        }
        delete filter;
    }
    // 2. 从保存的 m_beforeXml 中恢复滤镜
    Mlt::Profile profile(kDefaultMltProfile);
    Mlt::Producer filtersProducer(profile, "xml-string", m_beforeXml.toUtf8().constData());
    if (filtersProducer.is_valid() && filtersProducer.filter_count() > 0) {
        MLT.pasteFilters(&producer, &filtersProducer);
    }
    // 发出信号，通知 QML 界面
    emit QmlApplication::singleton().filtersPasted(&producer);
}

/**
 * @class UndoParameterCommand
 * @brief 撤销/重做“修改滤镜参数”操作的命令。
 *
 * 它通过保存修改前后的参数状态来实现撤销。
 */
UndoParameterCommand::UndoParameterCommand(const QString &name,
                                           FilterController *controller,
                                           int row,
                                           Mlt::Properties &before,
                                           const QString &desc,
                                           QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_filterController(controller)
    , m_row(row)
    , m_producerUuid(MLT.ensureHasUuid(*controller->attachedModel()->producer()))
    , m_firstRedo(true) // 标记是否是第一次 redo
{
    // 设置显示文本
    if (desc.isEmpty()) {
        setText(QObject::tr("Change %1 filter").arg(name));
    } else {
        setText(QObject::tr("Change %1 filter: %2").arg(name, desc));
    }
    // 保存修改前的所有参数
    m_before.inherit(before);
    // 保存修改后的所有参数
    Mlt::Service *service = controller->attachedModel()->getService(m_row);
    m_after.inherit(*service);
}

/**
 * @brief 更新命令中“修改后”的状态。
 *
 * 当用户连续调整一个参数时（例如拖动滑块），可以调用此方法来更新最终值，
 * 而不是为每个微小的变化都创建一个新的撤销命令。
 */
void UndoParameterCommand::update(const QString &propertyName)
{
    Mlt::Service *service = m_filterController->attachedModel()->getService(m_row);
    // 只更新指定的属性，而不是全部覆盖
    m_after.pass_property(*service, propertyName.toUtf8().constData());
}

/// 执行“修改参数”操作
void UndoParameterCommand::redo()
{
    LOG_DEBUG() << text();
    // 第一次 redo 时，操作实际上已经发生了，所以什么都不做，只是标记一下
    if (m_firstRedo) {
        m_firstRedo = false;
    } else {
        // 后续的 redo（即 undo 之后的重做）才需要应用 m_after 的状态
        Mlt::Producer producer = findProducer(m_producerUuid);
        Q_ASSERT(producer.is_valid());
        if (producer.is_valid() && m_filterController) {
            Mlt::Service service = m_filterController->attachedModel()->doGetService(producer,
                                                                                     m_row);
            service.inherit(m_after); // 应用修改后的参数
            m_filterController->onUndoOrRedo(service); // 通知控制器更新 UI
        }
    }
}

/// 撤销“修改参数”操作
void UndoParameterCommand::undo()
{
    LOG_DEBUG() << text();
    Mlt::Producer producer = findProducer(m_producerUuid);
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid() && m_filterController) {
        Mlt::Service service = m_filterController->attachedModel()->doGetService(producer, m_row);
        service.inherit(m_before); // 恢复修改前的参数
        m_filterController->onUndoOrRedo(service); // 通知控制器更新 UI
    }
}

/**
 * @brief 合并连续的参数修改命令。
 *
 * 当用户快速连续修改同一个滤镜的参数时，可以将这些修改合并为一个撤销步骤。
 */
bool UndoParameterCommand::mergeWith(const QUndoCommand *other)
{
    UndoParameterCommand *that = const_cast<UndoParameterCommand *>(
        static_cast<const UndoParameterCommand *>(other));
    LOG_DEBUG() << "this filter" << m_row << "that filter" << that->m_row;
    // 合并条件：必须是同一类型的命令、作用于同一个滤镜、同一个 Producer，并且显示文本相同
    if (that->id() != id() || that->m_row != m_row || that->m_producerUuid != m_producerUuid
        || that->text() != text())
        return false;
    // 合并：用新命令的“修改后”状态覆盖当前命令的“修改后”状态
    m_after = that->m_after;
    return true;
}

} // namespace Filter
