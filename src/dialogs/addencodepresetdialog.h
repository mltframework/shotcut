/*
 * Copyright (c) 2012 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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
#ifndef ADDENCODEPRESETDIALOG_H  // 如果未定义ADDENCODEPRESETDIALOG_H
#define ADDENCODEPRESETDIALOG_H  // 定义ADDENCODEPRESETDIALOG_H

// 包含Qt对话框基类头文件
#include <QDialog>

// 声明UI命名空间（用于隔离UI生成的代码）
namespace Ui {
class AddEncodePresetDialog;  // 前向声明UI中的对话框类
}

// 定义编码预设添加对话框类，继承自QDialog
class AddEncodePresetDialog : public QDialog
{
    Q_OBJECT  // 启用Qt元对象系统支持（信号槽、反射等）

public:
    // 构造函数，explicit防止隐式转换，parent为父窗口指针（默认nullptr）
    explicit AddEncodePresetDialog(QWidget *parent = 0);
    // 析构函数
    ~AddEncodePresetDialog();
    // 设置属性文本的方法，参数为属性字符串
    void setProperties(const QString &);
    // 获取预设名称的方法，返回值为名称字符串（const表示不修改成员变量）
    QString presetName() const;
    // 获取属性文本的方法，返回值为属性字符串（const表示不修改成员变量）
    QString properties() const;

private:
    // 指向UI界面对象的指针（由Qt Designer生成的界面元素）
    Ui::AddEncodePresetDialog *ui;
};

#endif // ADDENCODEPRESETDIALOG_H  // 结束头文件保护宏
