/*
 * Copyright (c) 2021 Meltytech, LLC
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
#include "qmltypes/qmlmetadata.h"
#include "controllers/filtercontroller.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include <Logger.h>

class FindProducerParser : public Mlt::Parser
{
private:
    QUuid m_uuid;
    Mlt::Producer m_producer;

public:
    FindProducerParser(QUuid uuid)
        : Mlt::Parser()
        , m_uuid(uuid)
    {}

    Mlt::Producer producer() { return m_producer; }

    int on_start_filter(Mlt::Filter*) { return 0; }
    int on_start_producer(Mlt::Producer* producer) {
        if (MLT.uuid(*producer) == m_uuid) {
            m_producer = producer;
            return 1;
        }
        return 0;
    }
    int on_end_producer(Mlt::Producer*) { return 0; }
    int on_start_playlist(Mlt::Playlist*) { return 0; }
    int on_end_playlist(Mlt::Playlist*) { return 0; }
    int on_start_tractor(Mlt::Tractor*) { return 0; }
    int on_end_tractor(Mlt::Tractor*) { return 0; }
    int on_start_multitrack(Mlt::Multitrack*) { return 0; }
    int on_end_multitrack(Mlt::Multitrack*) { return 0; }
    int on_start_track() { return 0; }
    int on_end_track() { return 0; }
    int on_end_filter(Mlt::Filter*) { return 0; }
    int on_start_transition(Mlt::Transition*) { return 0; }
    int on_end_transition(Mlt::Transition*) { return 0; }
    int on_start_chain(Mlt::Chain*) { return 0; }
    int on_end_chain(Mlt::Chain*) { return 0; }
    int on_start_link(Mlt::Link*) { return 0; }
    int on_end_link(Mlt::Link*) { return 0; }
};

static Mlt::Producer findProducer(const QUuid& uuid)
{
    FindProducerParser graphParser(uuid);
    if (MAIN.isMultitrackValid()) {
        graphParser.start(*MAIN.multitrack());
        if (graphParser.producer().is_valid()) {
            return graphParser.producer();
        }
    }
    if (MAIN.playlist() && MAIN.playlist()->count() > 0) {
        graphParser.start(*MAIN.playlist());
        if (graphParser.producer().is_valid()) {
            return graphParser.producer();
        }
    }
    Mlt::Producer producer(MLT.isClip()? MLT.producer() : MLT.savedProducer());
    if (producer.is_valid()) {
        graphParser.start(producer);
        if (graphParser.producer().is_valid()) {
            return graphParser.producer();
        }
    }
    return Mlt::Producer();
}

namespace Filter {

AddCommand::AddCommand(AttachedFiltersModel& model, QmlMetadata* metadata, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_metadata(metadata)
    , m_index(-1)
    , m_row(-1)
    , m_producer(*model.producer())
    , m_producerUuid(MLT.ensureHasUuid(m_producer))
{
    Q_ASSERT(m_metadata);
    setText(QObject::tr("Add %1 filter").arg(metadata->name()));
}

void AddCommand::redo()
{
    LOG_DEBUG() << text() << m_row;
    if (m_producer.is_valid()) {
        m_row = m_model.newService(m_metadata, m_producer, m_index);
        // Only hold the producer reference for the first redo and lookup by UUID thereafter.
        m_producer = Mlt::Producer();
    } else {
        Mlt::Producer producer = m_producer;
        if (!producer.is_valid()) {
            producer = findProducer(m_producerUuid);
        }
        Q_ASSERT(producer.is_valid());
        Q_ASSERT(m_service.is_valid());
        if (producer.is_valid()) {
            m_model.restoreService(producer, m_service, m_index);
        }
    }
}

void AddCommand::undo()
{
    LOG_DEBUG() << text() << m_row;
    Mlt::Producer producer(findProducer(m_producerUuid));
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_service = m_model.removeService(producer, m_index, m_row);
    }
}

RemoveCommand::RemoveCommand(AttachedFiltersModel& model, const QString& name, int index, int row, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_index(index)
    , m_row(row)
    , m_producer(*model.producer())
    , m_producerUuid(MLT.ensureHasUuid(m_producer))
{
    setText(QObject::tr("Remove %1 filter").arg(name));
}

void RemoveCommand::redo()
{
    LOG_DEBUG() << text() << m_row;
    Mlt::Producer producer = m_producer;
    if (!producer.is_valid()) {
        producer = findProducer(m_producerUuid);
    }
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_service = m_model.removeService(producer, m_index, m_row);
    }
    if (m_producer.is_valid()) {
        // Only hold the producer reference for the first redo and lookup by UUID thereafter.
        m_producer = Mlt::Producer();
    }
}

void RemoveCommand::undo()
{
    Q_ASSERT(m_service.is_valid());
    LOG_DEBUG() << text() << m_row;
    Mlt::Producer producer(findProducer(m_producerUuid));
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_model.restoreService(producer, m_service, m_index);
    }
}

MoveCommand::MoveCommand(AttachedFiltersModel& model, Mlt::Service& service, const QString& name, int fromIndex, int toIndex, int fromRow, int toRow, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_service(service)
    , m_fromIndex(fromIndex)
    , m_toIndex(toIndex)
    , m_fromRow(fromRow)
    , m_toRow(toRow)
    , m_producer(*model.producer())
    , m_producerUuid(MLT.ensureHasUuid(m_producer))
{
    setText(QObject::tr("Move %1 filter").arg(name));
}

void MoveCommand::redo()
{
    LOG_DEBUG() << text() << "from" << m_fromRow << "to" << m_toRow;
    Mlt::Producer producer = m_producer;
    if (!producer.is_valid()) {
        producer = findProducer(m_producerUuid);
    }
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_model.moveService(producer, m_fromIndex, m_toIndex, m_fromRow, m_toRow);
    }
    if (m_producer.is_valid()) {
        // Only hold the producer reference for the first redo and lookup by UUID thereafter.
        m_producer = Mlt::Producer();
    }
}

void MoveCommand::undo()
{
    LOG_DEBUG() << text() << "from" << m_toRow << "to" << m_fromRow;
    Mlt::Producer producer(findProducer(m_producerUuid));
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_model.moveService(producer, m_toIndex, m_fromIndex, m_toRow, m_fromRow);
    }
}

DisableCommand::DisableCommand(AttachedFiltersModel& model, Mlt::Service& service, const QString& name, int index, int row, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_service(service)
    , m_index(index)
    , m_row(row)
    , m_producer(*model.producer())
    , m_producerUuid(MLT.ensureHasUuid(m_producer))
    , m_isDisable(!m_model.isDisabled(m_producer, m_index))
{
    if (m_isDisable) {
        setText(QObject::tr("Disable %1 filter").arg(name));
    } else {
        setText(QObject::tr("Enable %1 filter").arg(name));
    }
}

void DisableCommand::redo()
{
    LOG_DEBUG() << text() << m_index;
    Mlt::Producer producer = m_producer;
    if (!producer.is_valid()) {
        producer = findProducer(m_producerUuid);
    }
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_model.setDisabled(producer, m_index, m_row, m_isDisable);
    }
    if (m_producer.is_valid()) {
        // Only hold the producer reference for the first redo and lookup by UUID thereafter.
        m_producer = Mlt::Producer();
    }
}

void DisableCommand::undo()
{
    LOG_DEBUG() << text() << m_index;
    Mlt::Producer producer(findProducer(m_producerUuid));
    Q_ASSERT(producer.is_valid());
    if (producer.is_valid()) {
        m_model.setDisabled(producer, m_index, m_row, !m_isDisable);
    }
}

bool DisableCommand::mergeWith(const QUndoCommand* other)
{
    DisableCommand* that = const_cast<DisableCommand*>(static_cast<const DisableCommand*>(other));
    LOG_DEBUG() << "this service" << m_service.get_service() << "that service" << that->m_service.get_service();
    if (that->id() != id() || that->m_service.get_service() != m_service.get_service())
        return false;
    m_isDisable = that->m_isDisable;
    setText(that->text());
    return true;
}

ChangeParameterCommand::ChangeParameterCommand(const QString& filterName, Mlt::Service& service, FilterController* controller, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_filterName(filterName)
    , m_service(service)
    , m_filterController(controller)
    , m_firstRedo(true)
{
    setText(QObject::tr("Change %1 filter").arg(m_filterName));
    m_before.inherit(m_service);
    if (!m_before.property_exists(kShotcutAnimInProperty)) {
        m_before.set(kShotcutAnimInProperty, 0);
    }
    if (!m_before.property_exists(kShotcutAnimOutProperty)) {
        m_before.set(kShotcutAnimOutProperty, 0);
    }
    m_after.inherit(m_service);
}

void ChangeParameterCommand::update(const QString& propertyName)
{
    m_after.pass_property(m_service, propertyName.toUtf8().constData());
}

void ChangeParameterCommand::redo()
{
    LOG_DEBUG() << m_filterName;
    if (m_firstRedo) {
        m_firstRedo = false;
    } else {
        m_service.inherit(m_after);
        if (m_filterController) {
            m_filterController->onUndoOrRedo(m_service);
        }
    }
}

void ChangeParameterCommand::undo()
{
    LOG_DEBUG() << m_filterName;
    m_service.inherit(m_before);
    m_filterController->onUndoOrRedo(m_service);
}

bool ChangeParameterCommand::mergeWith(const QUndoCommand* other)
{
    ChangeParameterCommand* that = const_cast<ChangeParameterCommand*>(static_cast<const ChangeParameterCommand*>(other));
    LOG_DEBUG() << "this filter" << m_filterName << "that filter" << that->m_filterName;
    if (that->id() != id() || that->m_service.get_service() != m_service.get_service())
        return false;
    m_after = that->m_after;
    return true;
}

} // namespace Filter
