/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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

#include "playlisttable.h"
#include <Logger.h>
#include <QKeyEvent>

PlaylistTable::PlaylistTable(QWidget *parent)
    : QTableView(parent)
{

}

void PlaylistTable::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        /* make sure we ignore left/right keypresses here so it can bubble its way up to the top
         * level where it's handled as a global keyboard shortcut. There's seemingly no way to keep
         * QTableView from using left/right for moving between cells, so this is a slight hack to
         * prevent that behavior. */
        event->ignore();
        return;
    }
    // Ignore select all
    if (event->key() == Qt::Key_A && event->modifiers() == Qt::ControlModifier) {
        event->ignore();
        return;
    }
    QTableView::keyPressEvent(event);
    event->ignore();
}

void PlaylistTable::dropEvent(QDropEvent *event)
{
    QModelIndex index = indexAt(event->position().toPoint());
    if (event->dropAction() == Qt::MoveAction && index.row() == -1) {
        event->acceptProposedAction();
        emit movedToEnd();
    } else {
        QTableView::dropEvent(event);
    }
}
