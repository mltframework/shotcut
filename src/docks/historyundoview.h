/*
 * Copyright (c) 2026 Meltytech, LLC
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

#ifndef HISTORYUNDOVIEW_H
#define HISTORYUNDOVIEW_H

#include <QUndoView>

class MultitrackModel;
class QKeyEvent;
class QMouseEvent;
class QUndoStack;

/// Wraps the History dock's QUndoView so that a multi-step jump (clicking or
/// keying to a row several steps away from the current one) is bracketed by
/// MultitrackModel::beginBulkUpdate()/endBulkUpdate(). QUndoView triggers
/// QUndoStack::setIndex() synchronously from within these event handlers, so
/// bracketing the whole handler call is guaranteed to enclose any resulting
/// redo()/undo() replay, letting the model defer expensive per-step work
/// (e.g. adjustTrackFilters()) until the jump is complete. The view compares
/// the undo index before and after each event so no-op events do not flush
/// deferred work or refresh the consumer.
class HistoryUndoView : public QUndoView
{
    Q_OBJECT

public:
    HistoryUndoView(QUndoStack *stack, MultitrackModel *model, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    MultitrackModel *m_model;
};

#endif // HISTORYUNDOVIEW_H
