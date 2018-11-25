/*
 * Copyright (c) 2013-2018 Meltytech, LLC
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

#include "playlistcommands.h"
#include "mltcontroller.h"
#include "mainwindow.h"
#include <Logger.h>

namespace Playlist
{

AppendCommand::AppendCommand(PlaylistModel& model, const QString& xml, bool emitModified, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_xml(xml)
    , m_emitModified(emitModified)
{
    setText(QObject::tr("Append playlist item %1").arg(m_model.rowCount() + 1));
}

void AppendCommand::redo()
{
    LOG_DEBUG() << "";
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.append(producer, m_emitModified);
}

void AppendCommand::undo()
{
    LOG_DEBUG() << "";
    m_model.remove(m_model.rowCount() - 1);
}

InsertCommand::InsertCommand(PlaylistModel& model, const QString& xml, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_xml(xml)
    , m_row(row)
{
    setText(QObject::tr("Insert playist item %1").arg(row + 1));
}

void InsertCommand::redo()
{
    LOG_DEBUG() << "row" << m_row;
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.insert(producer, m_row);
}

void InsertCommand::undo()
{
    LOG_DEBUG() << "row" << m_row;
    m_model.remove(m_row);
}

UpdateCommand::UpdateCommand(PlaylistModel& model, const QString& xml, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_newXml(xml)
    , m_row(row)
{
    setText(QObject::tr("Update playlist item %1").arg(row + 1));
    m_oldXml = MLT.XML(m_model.playlist()->get_clip(m_row));
}

void UpdateCommand::redo()
{
    LOG_DEBUG() << "row" << m_row;
    Mlt::Producer producer(MLT.profile(), "xml-string", m_newXml.toUtf8().constData());
    m_model.update(m_row, producer);
}

void UpdateCommand::undo()
{
    LOG_DEBUG() << "row" << m_row;
    Mlt::Producer producer(MLT.profile(), "xml-string", m_oldXml.toUtf8().constData());
    m_model.update(m_row, producer);
}

RemoveCommand::RemoveCommand(PlaylistModel& model, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
{
    m_xml = MLT.XML(m_model.playlist()->get_clip(m_row));
    setText(QObject::tr("Remove playlist item %1").arg(row + 1));
}

void RemoveCommand::redo()
{
    LOG_DEBUG() << "row" << m_row;
    m_model.remove(m_row);
}

void RemoveCommand::undo()
{
    LOG_DEBUG() << "row" << m_row;
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.insert(producer, m_row);
}

ClearCommand::ClearCommand(PlaylistModel& model, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
{
    m_xml = MLT.XML(m_model.playlist());
    setText(QObject::tr("Clear playlist"));
}

void ClearCommand::redo()
{
    LOG_DEBUG() << "";
    m_model.clear();
}

void ClearCommand::undo()
{
    LOG_DEBUG() << "";
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    if (producer->is_valid()) {
        producer->set("resource", "<playlist>");
        if (!MLT.setProducer(producer)) {
            m_model.load();
            MLT.pause();
            MAIN.seekPlaylist(0);
        }
    } else {
        LOG_ERROR() << "failed to restore playlist from XML";
    }
}

MoveCommand::MoveCommand(PlaylistModel &model, int from, int to, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_from(from)
    , m_to(to)
{
    setText(QObject::tr("Move item from %1 to %2").arg(from + 1).arg(to + 1));
}

void MoveCommand::redo()
{
    LOG_DEBUG() << "from" << m_from << "to" << m_to;
    m_model.move(m_from, m_to);
}

void MoveCommand::undo()
{
    LOG_DEBUG() << "from" << m_from << "to" << m_to;
    m_model.move(m_to, m_from);
}

} // namespace Playlist
