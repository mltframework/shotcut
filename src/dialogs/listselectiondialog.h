/*
 * Copyright (c) 2018-2022 Meltytech, LLC
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

// 头文件重复包含保护宏：未定义LISTSELECTIONDIALOG_H时，执行后续内容
#ifndef LISTSELECTIONDIALOG_H
#define LISTSELECTIONDIALOG_H

// 引入Qt标准对话框头文件（ListSelectionDialog继承自QDialog）
#include <QDialog>

// 声明UI命名空间（隔离Qt Designer生成的界面代码）
namespace Ui {
class ListSelectionDialog;  // 前向声明UI中的对话框类，避免提前包含头文件
}

// 前向声明：减少编译依赖
class QListWidgetItem;       // 列表项类（用于处理列表中的单个选项）
class QDialogButtonBox;      // 对话框按钮组类（整合确定、取消等按钮）

// 列表选择对话框类：用于展示可勾选的选项列表，支持通用选项和颜色选项两种模式
class ListSelectionDialog : public QDialog
{
    Q_OBJECT  // Qt元对象系统宏，支持信号槽等Qt核心特性

public:
    // 【构造函数】：初始化列表选择对话框（通用选项模式）
    // 参数说明：
    // - list：待展示的选项文本列表（如["选项1", "选项2"]）
    // - parent：父窗口指针（默认值0，用于Qt对象树管理）
    explicit ListSelectionDialog(const QStringList &list, QWidget *parent = 0);

    // 【析构函数】：释放UI资源
    ~ListSelectionDialog();

    // 【公共方法】：设置颜色选项列表（切换为颜色模式）
    // 参数：colors - 颜色字符串列表（如["#FF0000", "red"]）
    // 功能：列表项以颜色为背景，默认勾选，禁用交替行颜色和排序
    void setColors(const QStringList &colors);

    // 【公共方法】：设置初始选中的选项
    // 参数：selection - 需要初始勾选的选项文本列表
    // 功能：遍历列表项，将文本匹配的项设为已勾选状态
    void setSelection(const QStringList &selection);

    // 【公共方法】：获取用户选中的选项
    // 返回值：QStringList - 所有已勾选选项的文本列表
    QStringList selection() const;

    // 【公共方法】：获取对话框的按钮组
    // 返回值：QDialogButtonBox* - 对话框底部的按钮组对象（可用于自定义按钮逻辑）
    QDialogButtonBox *buttonBox() const;

private:
    // 【私有成员变量】：UI界面对象指针
    // 作用：访问对话框中的列表控件、按钮组等UI元素，由Qt Designer生成
    Ui::ListSelectionDialog *ui;

private slots:
    // 【私有槽函数】：处理列表项激活事件（如双击、回车）
    // 参数：item - 被激活的列表项
    // 功能：切换该列表项的勾选状态（已勾选→未勾选，未勾选→已勾选）
    void onItemActivated(QListWidgetItem *item);
};

// 结束头文件保护宏
#endif // LISTSELECTIONDIALOG_H
