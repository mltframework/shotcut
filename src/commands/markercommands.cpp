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

#include "markercommands.h"
#include <Logger.h>

namespace Markers
{

DeleteCommand::DeleteCommand(MarkersModel& model, const Marker& delMarker, int index)
    : QUndoCommand(0)
    , m_model(model)
    , m_delMarker(delMarker)
    , m_index(index)
{
    setText(QObject::tr("Delete Marker: %1").arg(m_delMarker.text));
}

void DeleteCommand::redo()
{
    m_model.doRemove(m_index);
}

void DeleteCommand::undo()
{
    m_model.doInsert(m_index, m_delMarker);
}

InsertCommand::InsertCommand(MarkersModel& model, const Marker& newMarker, int index)
    : QUndoCommand(0)
    , m_model(model)
    , m_newMarker(newMarker)
    , m_index(index)
{
    setText(QObject::tr("Insert Marker: %1").arg(newMarker.text));
}

void InsertCommand::redo()
{
    m_model.doInsert(m_index, m_newMarker);
}

void InsertCommand::undo()
{
    m_model.doRemove(m_index);
}

AppendCommand::AppendCommand(MarkersModel& model, const Marker& newMarker, int index)
    : QUndoCommand(0)
    , m_model(model)
    , m_newMarker(newMarker)
    , m_index(index)
{
    setText(QObject::tr("Append Marker: %1").arg(m_newMarker.text));
}

void AppendCommand::redo()
{
    m_model.doAppend(m_newMarker);
}

void AppendCommand::undo()
{
    m_model.doRemove(m_index);
}


UpdateCommand::UpdateCommand(MarkersModel& model, const Marker& newMarker, const Marker& oldMarker, int index)
    : QUndoCommand(0)
    , m_model(model)
    , m_newMarker(newMarker)
    , m_oldMarker(oldMarker)
    , m_index(index)
{
    if (m_newMarker.text == m_oldMarker.text && m_newMarker.color == m_oldMarker.color )
    {
        setText(QObject::tr("Move Marker: %1").arg(m_oldMarker.text));
    }
    else
    {
        setText(QObject::tr("Edit Marker: %1").arg(m_oldMarker.text));
    }
}

void UpdateCommand::redo()
{
    m_model.doUpdate(m_index, m_newMarker);
}

void UpdateCommand::undo()
{
    m_model.doUpdate(m_index, m_oldMarker);
}


} // namespace Markers
