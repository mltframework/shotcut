/*
 * Copyright (c) 2021-2024 Meltytech, LLC
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

#include <MltProducer.h>
#include <MltService.h>
#include <QString>
#include <QUndoCommand>
#include <QUuid>

class QmlMetadata;
class FilterController;

namespace Filter {

enum {
    UndoIdAdd = 300,
    UndoIdMove,
    UndoIdDisable,
    UndoIdChangeParameter,
    UndoIdChangeAddKeyframe,
    UndoIdChangeRemoveKeyframe,
    UndoIdChangeKeyframe,
};

class AddCommand : public QUndoCommand
{
public:
    typedef enum {
        AddSingle,
        AddSet,
        AddSetLast,
    } AddType;
    AddCommand(AttachedFiltersModel &model,
               const QString &name,
               Mlt::Service &service,
               int row,
               AddCommand::AddType type = AddCommand::AddSingle,
               QUndoCommand *parent = 0);
    void redo();
    void undo();

protected:
    int id() const { return UndoIdAdd; }
    bool mergeWith(const QUndoCommand *other);

private:
    AttachedFiltersModel &m_model;
    std::vector<int> m_rows;
    std::vector<Mlt::Service> m_services;
    Mlt::Producer m_producer;
    QUuid m_producerUuid;
    AddType m_type;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(AttachedFiltersModel &model,
                  const QString &name,
                  Mlt::Service &service,
                  int row,
                  QUndoCommand *parent = 0);
    void redo();
    void undo();

private:
    AttachedFiltersModel &m_model;
    int m_index;
    int m_row;
    Mlt::Producer m_producer;
    QUuid m_producerUuid;
    Mlt::Service m_service;
};

class MoveCommand : public QUndoCommand
{
public:
    MoveCommand(AttachedFiltersModel &model,
                const QString &name,
                int fromRow,
                int toRow,
                QUndoCommand *parent = 0);
    void redo();
    void undo();

protected:
    int id() const { return UndoIdMove; }

private:
    AttachedFiltersModel &m_model;
    int m_fromRow;
    int m_toRow;
    Mlt::Producer m_producer;
    QUuid m_producerUuid;
};

class DisableCommand : public QUndoCommand
{
public:
    DisableCommand(AttachedFiltersModel &model,
                   const QString &name,
                   int row,
                   bool disabled,
                   QUndoCommand *parent = 0);
    void redo();
    void undo();

protected:
    int id() const { return UndoIdDisable; }
    bool mergeWith(const QUndoCommand *other);

private:
    AttachedFiltersModel &m_model;
    int m_row;
    Mlt::Producer m_producer;
    QUuid m_producerUuid;
    bool m_disabled;
};

class UndoParameterCommand : public QUndoCommand
{
public:
    UndoParameterCommand(const QString &name,
                         FilterController *controller,
                         int row,
                         Mlt::Properties &before,
                         const QString &desc = QString(),
                         QUndoCommand *parent = 0);
    void update(const QString &propertyName);
    void redo();
    void undo();

protected:
    int id() const { return UndoIdChangeParameter; }
    bool mergeWith(const QUndoCommand *other);

private:
    int m_row;
    QUuid m_producerUuid;
    Mlt::Properties m_before;
    Mlt::Properties m_after;
    FilterController *m_filterController;
    bool m_firstRedo;
};

class UndoAddKeyframeCommand : public UndoParameterCommand
{
public:
    UndoAddKeyframeCommand(const QString &name,
                           FilterController *controller,
                           int row,
                           Mlt::Properties &before)
        : UndoParameterCommand(name, controller, row, before, QObject::tr("add keyframe"))
    {}

protected:
    int id() const { return UndoIdChangeAddKeyframe; }
    bool mergeWith(const QUndoCommand *other) { return false; }
};

class UndoRemoveKeyframeCommand : public UndoParameterCommand
{
public:
    UndoRemoveKeyframeCommand(const QString &name,
                              FilterController *controller,
                              int row,
                              Mlt::Properties &before)
        : UndoParameterCommand(name, controller, row, before, QObject::tr("remove keyframe"))
    {}

protected:
    int id() const { return UndoIdChangeRemoveKeyframe; }
    bool mergeWith(const QUndoCommand *other) { return false; }
};

class UndoModifyKeyframeCommand : public UndoParameterCommand
{
public:
    UndoModifyKeyframeCommand(const QString &name,
                              FilterController *controller,
                              int row,
                              Mlt::Properties &before,
                              int paramIndex,
                              int keyframeIndex)
        : UndoParameterCommand(name, controller, row, before, QObject::tr("modify keyframe"))
        , m_paramIndex(paramIndex)
        , m_keyframeIndex(keyframeIndex)
    {}

protected:
    int id() const { return UndoIdChangeRemoveKeyframe; }
    bool mergeWith(const QUndoCommand *other)
    {
        auto *that = dynamic_cast<const UndoModifyKeyframeCommand *>(other);
        if (!that || m_paramIndex != that->m_paramIndex || m_keyframeIndex != that->m_keyframeIndex)
            return false;
        else
            return UndoParameterCommand::mergeWith(other);
    }

private:
    int m_paramIndex;
    int m_keyframeIndex;
};

} // namespace Filter

#endif // FILTERCOMMANDS_H
