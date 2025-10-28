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

// 【构造函数】
// 【功能】：初始化编辑菜单组件
// 【参数】：parent - 父对象指针，用于Qt对象树管理
QmlEditMenu::QmlEditMenu(QObject *parent)
    : QObject(parent)
    , m_showPastePlain(false)  // 默认不显示纯文本粘贴选项
    , m_readOnly(false)        // 默认可读写模式
{}

// 【核心功能】：弹出编辑上下文菜单
// 【说明】：根据当前状态动态构建编辑菜单，包含标准编辑操作
void QmlEditMenu::popup()
{
    QMenu menu;

    // 【撤销操作】
    QAction undoAction(tr("Undo"));
    undoAction.setShortcut(QKeySequence::Undo);  // 设置快捷键 Ctrl+Z
    connect(&undoAction, &QAction::triggered, this, &QmlEditMenu::undoTriggered);
    if (!m_readOnly)  // 只读模式下禁用撤销
        menu.addAction(&undoAction);

    // 【重做操作】
    QAction redoAction(tr("Redo"));
    redoAction.setShortcut(QKeySequence::Redo);  // 设置快捷键 Ctrl+Y
    connect(&redoAction, &QAction::triggered, this, &QmlEditMenu::redoTriggered);
    if (!m_readOnly)  // 只读模式下禁用重做
        menu.addAction(&redoAction);

    if (!m_readOnly)
        menu.addSeparator();  // 添加分隔线

    // 【剪切操作】
    QAction cutAction(tr("Cut"));
    cutAction.setShortcut(QKeySequence::Cut);    // 设置快捷键 Ctrl+X
    connect(&cutAction, &QAction::triggered, this, &QmlEditMenu::cutTriggered);
    if (!m_readOnly)  // 只读模式下禁用剪切
        menu.addAction(&cutAction);

    // 【复制操作】
    QAction copyAction(tr("Copy"));
    copyAction.setShortcut(QKeySequence::Copy);  // 设置快捷键 Ctrl+C
    connect(&copyAction, &QAction::triggered, this, &QmlEditMenu::copyTriggered);
    menu.addAction(&copyAction);  // 只读模式下仍允许复制

    // 【粘贴操作】
    QAction pasteAction(tr("Paste"));
    pasteAction.setShortcut(QKeySequence::Paste); // 设置快捷键 Ctrl+V
    connect(&pasteAction, &QAction::triggered, this, &QmlEditMenu::pasteTriggered);
    if (!m_readOnly)  // 只读模式下禁用粘贴
        menu.addAction(&pasteAction);

    // 【纯文本粘贴操作】
    QAction pastePlainAction(tr("Paste Text Only"));
    pastePlainAction.setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V)); // 快捷键 Ctrl+Shift+V
    connect(&pastePlainAction, &QAction::triggered, this, &QmlEditMenu::pastePlainTriggered);
    if (m_showPastePlain && !m_readOnly)  // 根据配置和模式决定是否显示
        menu.addAction(&pastePlainAction);

    // 【删除操作】
    QAction deleteAction(tr("Delete"));
    deleteAction.setShortcut(QKeySequence::Delete);  // 设置快捷键 Delete
    connect(&deleteAction, &QAction::triggered, this, &QmlEditMenu::deleteTriggered);
    if (!m_readOnly)  // 只读模式下禁用删除
        menu.addAction(&deleteAction);

    // 【清空操作】
    QAction clearAction(tr("Clear"));
    // 清空操作通常没有标准快捷键
    connect(&clearAction, &QAction::triggered, this, &QmlEditMenu::clearTriggered);
    if (!m_readOnly)  // 只读模式下禁用清空
        menu.addAction(&clearAction);

    if (!m_readOnly)
        menu.addSeparator();  // 添加分隔线

    // 【全选操作】
    QAction selectAllAction(tr("Select All"));
    selectAllAction.setShortcut(QKeySequence::SelectAll);  // 设置快捷键 Ctrl+A
    connect(&selectAllAction, &QAction::triggered, this, &QmlEditMenu::selectAllTriggered);
    menu.addAction(&selectAllAction);  // 只读模式下仍允许全选

    // 在鼠标当前位置显示菜单
    menu.exec(QCursor::pos());
}
