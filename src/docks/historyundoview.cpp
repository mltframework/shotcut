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

#include "historyundoview.h"

#include "models/multitrackmodel.h"

#include <QUndoStack>

HistoryUndoView::HistoryUndoView(QUndoStack *stack, MultitrackModel *model, QWidget *parent)
    : QUndoView(stack, parent)
    , m_model(model)
{}

void HistoryUndoView::mousePressEvent(QMouseEvent *event)
{
    const int index = stack()->index();
    m_model->beginBulkUpdate();
    QUndoView::mousePressEvent(event);
    m_model->endBulkUpdate(stack()->index() != index);
}

void HistoryUndoView::mouseDoubleClickEvent(QMouseEvent *event)
{
    const int index = stack()->index();
    m_model->beginBulkUpdate();
    QUndoView::mouseDoubleClickEvent(event);
    m_model->endBulkUpdate(stack()->index() != index);
}

void HistoryUndoView::keyPressEvent(QKeyEvent *event)
{
    const int index = stack()->index();
    m_model->beginBulkUpdate();
    QUndoView::keyPressEvent(event);
    m_model->endBulkUpdate(stack()->index() != index);
}
