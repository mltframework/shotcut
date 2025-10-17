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

#ifndef QMLEDITMENU_H
#define QMLEDITMENU_H

#include <QObject>

// 【类说明】：QML编辑菜单组件
// 【功能】：提供标准编辑操作的上下文菜单
// 【特性】：支持动态菜单项显示、快捷键配置、只读模式
class QmlEditMenu : public QObject
{
    Q_OBJECT
    // 【QML属性】：是否显示纯文本粘贴选项
    Q_PROPERTY(bool showPastePlain MEMBER m_showPastePlain NOTIFY showPastePlainChanged)
    // 【QML属性】：是否为只读模式（禁用编辑操作）
    Q_PROPERTY(bool readOnly MEMBER m_readOnly NOTIFY readOnlyChanged)

public:
    // 【构造函数】
    explicit QmlEditMenu(QObject *parent = 0);

signals:
    // 【信号】：纯文本粘贴选项显示状态改变
    void showPastePlainChanged();
    // 【信号】：只读模式状态改变
    void readOnlyChanged();
    
    // 编辑操作触发信号
    void undoTriggered();        // 撤销操作触发
    void redoTriggered();        // 重做操作触发
    void cutTriggered();         // 剪切操作触发
    void copyTriggered();        // 复制操作触发
    void pasteTriggered();       // 粘贴操作触发
    void pastePlainTriggered();  // 纯文本粘贴触发
    void deleteTriggered();      // 删除操作触发
    void clearTriggered();       // 清空操作触发
    void selectAllTriggered();   // 全选操作触发

public slots:
    // 【槽函数】：弹出编辑菜单
    // 【说明】：在鼠标当前位置显示上下文编辑菜单
    void popup();

private:
    bool m_showPastePlain;  // 控制是否显示纯文本粘贴选项
    bool m_readOnly;        // 控制是否为只读模式（禁用编辑操作）
};

#endif // QMLEDITMENU_H
