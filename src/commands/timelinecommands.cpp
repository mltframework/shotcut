/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "timelinecommands.h"
#include "mltcontroller.h"

Timeline::AppendCommand::AppendCommand(MultitrackModel &model, int trackIndex, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_xml(xml)
{
    setText(QObject::tr("Append to track"));
}

void Timeline::AppendCommand::redo()
{
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_clipIndex = m_model.appendClip(m_trackIndex, producer);
}

void Timeline::AppendCommand::undo()
{
    m_model.removeClip(m_trackIndex, m_clipIndex);
}
