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

#ifndef MARKERCOMMANDS_H
#define MARKERCOMMANDS_H

#include "models/markersmodel.h"
#include <QUndoCommand>

namespace Markers
{

class DeleteCommand : public QUndoCommand
{
public:
    DeleteCommand(MarkersModel& model, const Marker& delMarker, int index);
    void redo();
    void undo();
private:
    MarkersModel& m_model;
    Marker m_delMarker;
    int m_index;
};

class InsertCommand : public QUndoCommand
{
public:
    InsertCommand(MarkersModel& model, const Marker& newMarker, int index);
    void redo();
    void undo();
private:
    MarkersModel& m_model;
    Marker m_newMarker;
    int m_index;
};

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(MarkersModel& model, const Marker& newMarker, int index);
    void redo();
    void undo();
private:
    MarkersModel& m_model;
    Marker m_newMarker;
    int m_index;
};

class UpdateCommand : public QUndoCommand
{
public:
    UpdateCommand(MarkersModel& model, const Marker& newMarker, const Marker& oldMarker, int index);
    void redo();
    void undo();
private:
    MarkersModel& m_model;
    Marker m_newMarker;
    Marker m_oldMarker;
    int m_index;
};

}

#endif // MARKERCOMMANDS_H
