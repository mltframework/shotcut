/*
 * Copyright (c) 2013-2020 Meltytech, LLC
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
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    info->producer->set_in_and_out(info->frame_in, info->frame_out);
    m_oldXml = MLT.XML(info->producer);
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

bool UpdateCommand::mergeWith(const QUndoCommand *other)
{
    const UpdateCommand* that = static_cast<const UpdateCommand*>(other);
    LOG_DEBUG() << "this row" << m_row << "that row" << that->m_row;
    if (that->id() != id() || that->m_row != m_row)
        return false;
    m_newXml = that->m_newXml;
    return true;
}

RemoveCommand::RemoveCommand(PlaylistModel& model, int row, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
{
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    info->producer->set_in_and_out(info->frame_in, info->frame_out);
    m_xml = MLT.XML(info->producer);
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

SortCommand::SortCommand(PlaylistModel& model, int column, Qt::SortOrder order, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_column(column)
    , m_order(order)
{
    m_xml = MLT.XML(m_model.playlist());
    QString columnName = m_model.headerData(m_column, Qt::Horizontal, Qt::DisplayRole).toString();
    setText(QObject::tr("Sort playlist by %1").arg(columnName));
}

void SortCommand::redo()
{
    LOG_DEBUG() << m_column;
    m_model.sort(m_column, m_order);
}

void SortCommand::undo()
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

TrimClipInCommand::TrimClipInCommand(PlaylistModel& model, int row, int in, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
    , m_oldIn(in)
    , m_newIn(in)
    , m_out(-1)
{
    setText(QObject::tr("Trim playlist item %1 in").arg(row + 1));
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    if (info) {
        m_oldIn = info->frame_in;
        m_out = info->frame_out;
    }
}

void TrimClipInCommand::redo()
{
    LOG_DEBUG() << "row" << m_row << "in" << m_newIn;
    m_model.setInOut(m_row, m_newIn, m_out);
}

void TrimClipInCommand::undo()
{
    LOG_DEBUG() << "row" << m_row << "in" << m_oldIn;
    m_model.setInOut(m_row, m_oldIn, m_out);
}

bool TrimClipInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipInCommand* that = static_cast<const TrimClipInCommand*>(other);
    LOG_DEBUG() << "this row" << m_row << "that row" << that->m_row;
    if (that->id() != id() || that->m_row != m_row)
        return false;
    m_newIn = that->m_newIn;
    return true;
}

TrimClipOutCommand::TrimClipOutCommand(PlaylistModel& model, int row, int out, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_row(row)
    , m_in(-1)
    , m_oldOut(out)
    , m_newOut(out)
{
    setText(QObject::tr("Trim playlist item %1 out").arg(row + 1));
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    if (info) {
        m_in = info->frame_in;
        m_oldOut = info->frame_out;
    }
}

void TrimClipOutCommand::redo()
{
    LOG_DEBUG() << "row" << m_row << "out" << m_newOut;
    m_model.setInOut(m_row, m_in, m_newOut);
}

void TrimClipOutCommand::undo()
{
    LOG_DEBUG() << "row" << m_row << "out" << m_oldOut;
    m_model.setInOut(m_row, m_in, m_oldOut);
}

bool TrimClipOutCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipOutCommand* that = static_cast<const TrimClipOutCommand*>(other);
    LOG_DEBUG() << "this row" << m_row << "that row" << that->m_row;
    if (that->id() != id() || that->m_row != m_row)
        return false;
    m_newOut = that->m_newOut;
    return true;
}

ReplaceCommand::ReplaceCommand(PlaylistModel& model, const QString& xml, int row, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_newXml(xml)
    , m_row(row)
{
    setText(QObject::tr("Replace playlist item %1").arg(row + 1));
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
    info->producer->set_in_and_out(info->frame_in, info->frame_out);
    m_oldXml = MLT.XML(info->producer);
}

void ReplaceCommand::redo()
{
    LOG_DEBUG() << "row" << m_row;
    Mlt::Producer producer(MLT.profile(), "xml-string", m_newXml.toUtf8().constData());
    m_model.update(m_row, producer, true);
}

void ReplaceCommand::undo()
{
    LOG_DEBUG() << "row" << m_row;
    Mlt::Producer producer(MLT.profile(), "xml-string", m_oldXml.toUtf8().constData());
    m_model.update(m_row, producer, true);
}

} // namespace Playlist
