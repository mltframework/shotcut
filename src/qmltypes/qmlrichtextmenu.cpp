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

#include "qmlrichtextmenu.h"

#include <QMenu>

QmlRichTextMenu::QmlRichTextMenu(QObject *parent)
    : QObject(parent)
{
}

void QmlRichTextMenu::popup()
{
    QMenu menu;

    QMenu *fileMenu = menu.addMenu(tr("File"));

    QAction openAction(tr("Open..."));
    connect(&openAction, &QAction::triggered, this, &QmlRichTextMenu::openTriggered);
    fileMenu->addAction(&openAction);

    QAction saveAsAction(tr("Save As..."));
    connect(&saveAsAction, &QAction::triggered, this, &QmlRichTextMenu::saveAsTriggered);
    fileMenu->addAction(&saveAsAction);

    QMenu *editMenu = menu.addMenu(tr("Edit"));

    QAction undoAction(tr("Undo"));
    undoAction.setShortcut(QKeySequence::Undo);
    connect(&undoAction, &QAction::triggered, this, &QmlRichTextMenu::undoTriggered);
    editMenu->addAction(&undoAction);

    QAction redoAction(tr("Redo"));
    redoAction.setShortcut(QKeySequence::Redo);
    connect(&redoAction, &QAction::triggered, this, &QmlRichTextMenu::redoTriggered);
    editMenu->addAction(&redoAction);

    editMenu->addSeparator();

    QAction cutAction(tr("Cut"));
    cutAction.setShortcut(QKeySequence::Cut);
    connect(&cutAction, &QAction::triggered, this, &QmlRichTextMenu::cutTriggered);
    editMenu->addAction(&cutAction);

    QAction copyAction(tr("Copy"));
    copyAction.setShortcut(QKeySequence::Copy);
    connect(&copyAction, &QAction::triggered, this, &QmlRichTextMenu::copyTriggered);
    editMenu->addAction(&copyAction);

    QAction pasteAction(tr("Paste"));
    pasteAction.setShortcut(QKeySequence::Paste);
    connect(&pasteAction, &QAction::triggered, this, &QmlRichTextMenu::pasteTriggered);
    editMenu->addAction(&pasteAction);

    QAction pastePlainAction(tr("Paste Text Only"));
    pastePlainAction.setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_V));
    connect(&pastePlainAction, &QAction::triggered, this, &QmlRichTextMenu::pastePlainTriggered);
    editMenu->addAction(&pastePlainAction);

    QAction selectAllAction(tr("Select All"));
    selectAllAction.setShortcut(QKeySequence::SelectAll);
    connect(&selectAllAction, &QAction::triggered, this, &QmlRichTextMenu::selectAllTriggered);
    menu.addAction(&selectAllAction);

    QAction tableAction(tr("Insert Table"));
    connect(&tableAction, &QAction::triggered, this, &QmlRichTextMenu::insertTableTriggered);
    menu.addAction(&tableAction);

    menu.exec(QCursor::pos());
}
