/*
 * Copyright (c) 2019 Meltytech, LLC
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

#include "playlistlistview.h"
#include <QDropEvent>

PlaylistListView::PlaylistListView(QWidget *parent)
    : QListView(parent)
{
}

void PlaylistListView::dropEvent(QDropEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (event->dropAction() == Qt::MoveAction && index.row() == -1) {
        event->acceptProposedAction();
        emit movedToEnd();
    } else {
        QListView::dropEvent(event);
    }
}

void PlaylistListView::keyPressEvent(QKeyEvent *event)
{
    // Ignore select all
    if (event->key() == Qt::Key_A && event->modifiers() == Qt::ControlModifier) {
        event->ignore();
        return;
    }
    QListView::keyPressEvent(event);
    event->ignore();
}
