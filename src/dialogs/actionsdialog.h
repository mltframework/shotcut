/*
 * Copyright (c) 2022-2024 Meltytech, LLC
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

// 防止头文件重复包含的宏定义
#ifndef ACTIONSDIALOG_H  // 如果未定义ACTIONSIDALOG_H
#define ACTIONSDIALOG_H  // 定义ACTIONSIDALOG_H

// 包含必要的头文件
#include "models/actionsmodel.h"  // 包含动作模型类的头文件

#include <QDialog>  // 包含Qt对话框类的头文件

// 前向声明（避免不必要的头文件包含）
class PrivateTreeView;
class QLineEdit;
class QSortFilterProxyModel;
class StatusLabelWidget;
class QKeySequenceEdit;

// 定义ActionsDialog类，继承自QDialog
class ActionsDialog : public QDialog
{
    Q_OBJECT  // Qt元对象系统宏，用于支持信号和槽等特性
public:
    // 构造函数，explicit关键字防止隐式转换，parent为父窗口指针，默认值为0
    explicit ActionsDialog(QWidget *parent = 0);
    // 保存当前编辑器内容的函数声明
    void saveCurrentEditor();

public slots:  // 公共槽函数（可被信号触发）
    // 让搜索结果获得焦点的槽函数
    void focusSearchResults();

protected:  // 保护成员函数（子类可重写）
    // 重写隐藏事件处理函数
    void hideEvent(QHideEvent *event);
    // 重写显示事件处理函数
    void showEvent(QShowEvent *event);

private:  // 私有成员变量
    QLineEdit *m_searchField;  // 搜索输入框指针
    ActionsModel m_model;  // 动作数据模型对象
    PrivateTreeView *m_table;  // 自定义树视图指针（用于显示数据）
    QSortFilterProxyModel *m_proxyModel;  // 排序过滤代理模型指针（用于数据过滤和排序）
    StatusLabelWidget *m_status;  // 状态标签部件指针（用于显示状态信息）
};

#endif // ACTIONSDIALOG_  // 结束ACTIONSIDALOG_H的条件编译块
