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

#ifndef FILTERCOMMANDS_H
#define FILTERCOMMANDS_H

#include "models/attachedfiltersmodel.h"
#include "qmltypes/qmlfilter.h"
#include <MltFilter.h>
#include <MltProducer.h>
#include <QUndoCommand>
#include <QString>
#include <QUuid>

class QmlMetadata;
class FilterController;

namespace Filter
{

enum {
    UndoIdMove = 300,
    UndoIdDisable,
    UndoIdChange
};

class AddCommand : public QUndoCommand
{
public:
    AddCommand(AttachedFiltersModel& model, QmlMetadata* metadata, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    AttachedFiltersModel& m_model;
    QmlMetadata* m_metadata;
    int m_index;
    int m_row;
    Mlt::Producer m_producer;
    QUuid m_producerUuid;
    Mlt::Service m_service;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(AttachedFiltersModel& model, const QString& name, int index, int row, QUndoCommand * parent = 0);
    void redo();
    void undo();
private:
    AttachedFiltersModel& m_model;
    int m_index;
    int m_row;
    Mlt::Producer m_producer;
    QUuid m_producerUuid;
    Mlt::Service m_service;
};

class MoveCommand : public QUndoCommand
{
public:
    MoveCommand(AttachedFiltersModel& model, Mlt::Service& service, const QString& name, int fromIndex, int toIndex, int fromRow, int toRow, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const { return UndoIdMove; }
private:
    AttachedFiltersModel& m_model;
    Mlt::Service m_service;
    int m_fromIndex;
    int m_toIndex;
    int m_fromRow;
    int m_toRow;
    Mlt::Producer m_producer;
    QUuid m_producerUuid;
};

class DisableCommand : public QUndoCommand
{
public:
    DisableCommand(AttachedFiltersModel& model, Mlt::Service& service, const QString& name, int index, int row, QUndoCommand * parent = 0);
    void redo();
    void undo();
protected:
    int id() const { return UndoIdDisable; }
    bool mergeWith(const QUndoCommand *other);
private:
    AttachedFiltersModel& m_model;
    Mlt::Service m_service;
    int m_index;
    int m_row;
    Mlt::Producer m_producer;
    QUuid m_producerUuid;
    bool m_isDisable;
};

class ChangeParameterCommand : public QUndoCommand
{
public:
    ChangeParameterCommand(const QString& filterName, Mlt::Service& service, FilterController* controller, QUndoCommand * parent = 0);
    void update(const QString& propertyName);
    void redo();
    void undo();
protected:
    int id() const { return UndoIdChange; }
    bool mergeWith(const QUndoCommand *other);
private:
    QString m_filterName;
    Mlt::Service m_service;
    Mlt::Properties m_before;
    Mlt::Properties m_after;
    FilterController* m_filterController;
    bool m_firstRedo;
};

} // namespace Filter

#endif // FILTERCOMMANDS_H
