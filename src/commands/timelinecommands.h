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

#ifndef COMMANDS_H
#define COMMANDS_H

#include "models/multitrackmodel.h"
#include <QUndoCommand>
#include <QString>

namespace Timeline
{

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(MultitrackModel& model, int trackIndex, const QString& xml, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    MultitrackModel& m_model;
    int m_trackIndex;
    int m_clipIndex;
    QString m_xml;
};

}

#endif
