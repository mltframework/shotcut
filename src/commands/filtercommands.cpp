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
#include <Logger.h>

namespace Filter {

AddCommand::AddCommand(AttachedFiltersModel& model, QmlMetadata* metadata, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_metadata(metadata)
    , m_index(-1)
    , m_producer(model.producer())
{
    Q_ASSERT(m_metadata);
    setText(QObject::tr("Add %1 filter").arg(metadata->name()));
}

void AddCommand::redo()
{
    LOG_DEBUG() << text() << m_index;
    if (m_filter.is_valid()) {
        m_model.addFilter(m_producer, m_filter, m_index);
    } else {
        m_index = m_model.addFilter(m_metadata, m_producer, m_index);
    }
}

void AddCommand::undo()
{
    LOG_DEBUG() << text() << m_index;
    m_filter = m_model.removeFilter(m_producer, m_index);
}

RemoveCommand::RemoveCommand(AttachedFiltersModel& model, const QString& name, int index, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_index(index)
    , m_producer(model.producer())
{
    setText(QObject::tr("Remove %1 filter").arg(name));
}

void RemoveCommand::redo()
{
    Q_ASSERT(m_producer.is_valid());
    LOG_DEBUG() << text() << m_index;
    m_filter = m_model.removeFilter(m_producer, m_index);
}

void RemoveCommand::undo()
{
    Q_ASSERT(m_filter.is_valid());
    LOG_DEBUG() << text() << m_index;
    m_model.addFilter(m_producer, m_filter, m_index);
}

MoveCommand::MoveCommand(AttachedFiltersModel& model, const QString& name, int fromIndex, int toIndex, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_fromIndex(fromIndex)
    , m_toIndex(toIndex)
    , m_producer(model.producer())
{
    setText(QObject::tr("Move %1 filter").arg(name));
}

void MoveCommand::redo()
{
    LOG_DEBUG() << text() << "from" << m_fromIndex << "to" << m_toIndex;
    m_model.moveFilter(m_producer, m_fromIndex, m_toIndex);
}

void MoveCommand::undo()
{
    LOG_DEBUG() << text() << "from" << m_toIndex << "to" << m_fromIndex;
    m_model.moveFilter(m_producer, m_toIndex, m_fromIndex);
}

DisableCommand::DisableCommand(AttachedFiltersModel& model, const QString& name, int index, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_index(index)
    , m_producer(model.producer())
{
    if (m_model.isDisabled(m_producer, m_index)) {
        setText(QObject::tr("Enable %1 filter").arg(name));
    } else {
        setText(QObject::tr("Disable %1 filter").arg(name));
    }
}

void DisableCommand::redo()
{
    Q_ASSERT(m_producer.is_valid());
    LOG_DEBUG() << text() << m_index;
    m_model.toggleDisable(m_producer, m_index);
}

void DisableCommand::undo()
{
    Q_ASSERT(m_producer.is_valid());
    LOG_DEBUG() << text() << m_index;
    m_model.toggleDisable(m_producer, m_index);
}

ChangeParameterCommand::ChangeParameterCommand(const QString& filterName, Mlt::Filter& filter, FilterController* controller, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_filterName(filterName)
    , m_filter(filter)
    , m_filterController(controller)
    , m_firstRedo(true)
{
    setText(QObject::tr("Change %1 filter").arg(m_filterName));
    m_before.inherit(m_filter);
    m_after.inherit(m_filter);
}

void ChangeParameterCommand::update(const QString& parameter)
{
    m_after.pass_property(m_filter, parameter.toUtf8().constData());
}

void ChangeParameterCommand::redo()
{
    LOG_DEBUG() << m_filterName;
    if (m_firstRedo) {
        m_firstRedo = false;
    } else {
        m_filter.inherit(m_after);
        if (m_filterController) {
            m_filterController->onUndoOrRedo(m_filter);
        }
    }
}

void ChangeParameterCommand::undo()
{
    LOG_DEBUG() << m_filterName;
    m_filter.inherit(m_before);
    m_filterController->onUndoOrRedo(m_filter);
}

} // namespace Filter
