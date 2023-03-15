/*
 * Copyright (c) 2022 Meltytech, LLC
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

#include "qmleditmenu.h"

#include <QMenu>

QmlEditMenu::QmlEditMenu(QObject *parent)
    : QObject(parent)
    , m_showPastePlain(false)
    , m_readOnly(false)
{
}

void QmlEditMenu::popup()
{
    QMenu menu;

    QAction undoAction(tr("Undo"));
    undoAction.setShortcut(QKeySequence::Undo);
    connect(&undoAction, &QAction::triggered, this, &QmlEditMenu::undoTriggered);
    if (!m_readOnly)
        menu.addAction(&undoAction);

    QAction redoAction(tr("Redo"));
    redoAction.setShortcut(QKeySequence::Redo);
    connect(&redoAction, &QAction::triggered, this, &QmlEditMenu::redoTriggered);
    if (!m_readOnly)
        menu.addAction(&redoAction);

    if (!m_readOnly)
        menu.addSeparator();

    QAction cutAction(tr("Cut"));
    cutAction.setShortcut(QKeySequence::Cut);
    connect(&cutAction, &QAction::triggered, this, &QmlEditMenu::cutTriggered);
    if (!m_readOnly)
        menu.addAction(&cutAction);

    QAction copyAction(tr("Copy"));
    copyAction.setShortcut(QKeySequence::Copy);
    connect(&copyAction, &QAction::triggered, this, &QmlEditMenu::copyTriggered);
    menu.addAction(&copyAction);

    QAction pasteAction(tr("Paste"));
    pasteAction.setShortcut(QKeySequence::Paste);
    connect(&pasteAction, &QAction::triggered, this, &QmlEditMenu::pasteTriggered);
    if (!m_readOnly)
        menu.addAction(&pasteAction);

    QAction pastePlainAction(tr("Paste Text Only"));
    pastePlainAction.setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    connect(&pastePlainAction, &QAction::triggered, this, &QmlEditMenu::pastePlainTriggered);
    if (m_showPastePlain && !m_readOnly)
        menu.addAction(&pastePlainAction);

    QAction deleteAction(tr("Delete"));
    deleteAction.setShortcut(QKeySequence::Delete);
    connect(&deleteAction, &QAction::triggered, this, &QmlEditMenu::deleteTriggered);
    if (!m_readOnly)
        menu.addAction(&deleteAction);

    QAction clearAction(tr("Clear"));
    connect(&clearAction, &QAction::triggered, this, &QmlEditMenu::clearTriggered);
    if (!m_readOnly)
        menu.addAction(&clearAction);

    if (!m_readOnly)
        menu.addSeparator();

    QAction selectAllAction(tr("Select All"));
    selectAllAction.setShortcut(QKeySequence::SelectAll);
    connect(&selectAllAction, &QAction::triggered, this, &QmlEditMenu::selectAllTriggered);
    menu.addAction(&selectAllAction);

    menu.exec(QCursor::pos());
}
