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

/ 包含当前类的头文件
#include "listselectiondialog.h"
// 包含UI文件生成的头文件（用于访问界面控件）
#include "ui_listselectiondialog.h"

// 引入Qt列表控件头文件
#include <QListWidget>


// 【构造函数】：初始化列表选择对话框（通用列表模式）
// 参数说明：
// - list：待显示的选项文本列表（如["选项1", "选项2"]）
// - parent：父窗口指针
ListSelectionDialog::ListSelectionDialog(const QStringList &list, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ListSelectionDialog)  // 初始化UI指针，创建界面对象
{
    ui->setupUi(this);  // 加载UI布局，初始化界面控件（列表、按钮组等）

    // 遍历选项列表，为每个文本创建列表项并添加到列表控件
    for (auto &text : list) {
        // 创建列表项，文本为当前选项，父控件为UI中的列表控件
        QListWidgetItem *item = new QListWidgetItem(text, ui->listWidget);
        // 设置列表项属性：可勾选、启用、可选择
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        // 初始勾选状态：未勾选
        item->setCheckState(Qt::Unchecked);
        // 连接列表项激活信号（如双击、回车）到槽函数（切换勾选状态）
        connect(ui->listWidget,
                SIGNAL(itemActivated(QListWidgetItem *)),
                SLOT(onItemActivated(QListWidgetItem *)));
    }
}

// 【析构函数】：释放UI资源
ListSelectionDialog::~ListSelectionDialog()
{
    delete ui;  // 释放UI对象占用的内存
}


// 【公共方法】：设置颜色列表（特殊模式：列表项以颜色为背景，默认勾选）
// 参数：list - 颜色字符串列表（如["#FF0000", "red", "255,0,0"]）
void ListSelectionDialog::setColors(const QStringList &list)
{
    // 配置列表控件：禁用交替行颜色、禁用排序
    ui->listWidget->setAlternatingRowColors(false);
    ui->listWidget->setSortingEnabled(false);

    // 遍历颜色列表，创建带颜色背景的列表项
    for (auto &text : list) {
        // 创建列表项，文本为颜色字符串，父控件为列表控件
        QListWidgetItem *item = new QListWidgetItem(text, ui->listWidget);
        // 设置列表项属性：可勾选、启用、可选择
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        // 连接列表项激活信号到槽函数（切换勾选状态）
        connect(ui->listWidget,
                SIGNAL(itemActivated(QListWidgetItem *)),
                SLOT(onItemActivated(QListWidgetItem *)));
        // 将文本解析为颜色对象
        QColor color(text);
        // 初始勾选状态：已勾选
        item->setCheckState(Qt::Checked);
        // 若颜色有效，设置列表项背景为该颜色
        if (color.isValid()) {
            item->setBackground(color);
        }
    }
}

// 【公共方法】：设置初始选中的选项
// 参数：selection - 初始需要勾选的选项文本列表
void ListSelectionDialog::setSelection(const QStringList &selection)
{
    int n = ui->listWidget->count();  // 获取列表项总数
    // 遍历所有列表项
    for (int i = 0; i < n; ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);  // 获取当前列表项
        // 若当前项文本在选中列表中，设置为已勾选
        if (selection.indexOf(item->text()) > -1)
            item->setCheckState(Qt::Checked);
    }
}

// 【公共方法】：获取用户选中的选项
// 返回值：QStringList - 所有已勾选选项的文本列表
QStringList ListSelectionDialog::selection() const
{
    QStringList result;  // 存储选中结果的列表
    int n = ui->listWidget->count();  // 获取列表项总数
    // 遍历所有列表项
    for (int i = 0; i < n; ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);  // 获取当前列表项
        // 若当前项为已勾选状态，将其文本添加到结果列表
        if (item->checkState() == Qt::Checked)
            result << item->text();
    }
    return result;
}

// 【公共方法】：获取对话框的按钮组
// 返回值：QDialogButtonBox* - 对话框底部的按钮组（如确定、取消按钮）
QDialogButtonBox *ListSelectionDialog::buttonBox() const
{
    return ui->buttonBox;  // 返回UI中的按钮组对象
}


// 【私有槽函数】：处理列表项激活事件（切换勾选状态）
// 参数：item - 被激活的列表项
void ListSelectionDialog::onItemActivated(QListWidgetItem *item)
{
    // 切换列表项的勾选状态：已勾选→未勾选，未勾选→已勾选
    item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
}
