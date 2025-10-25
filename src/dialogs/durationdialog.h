/*
 * Copyright (c) 2012-2022 Meltytech, LLC
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

// 头文件重复包含保护宏：未定义DURATIONDIALOG_H时，执行后续内容
#ifndef DURATIONDIALOG_H
#define DURATIONDIALOG_H

// 引入Qt标准对话框头文件（DurationDialog继承自QDialog）
#include <QDialog>

// 声明UI命名空间（隔离Qt Designer生成的界面代码）
namespace Ui {
class DurationDialog;  // 前向声明UI中的对话框类，避免提前包含头文件
}

// 时长设置对话框类：用于设置或修改视频/音频的时长（以帧为单位）
class DurationDialog : public QDialog
{
    Q_OBJECT  // Qt元对象系统宏，支持信号槽等Qt核心特性

public:
    // 构造函数：初始化时长对话框
    // 参数parent：父窗口指针（默认值0，用于Qt对象树管理）
    explicit DurationDialog(QWidget *parent = 0);
    // 析构函数：释放UI对象资源
    ~DurationDialog();

    // 公共方法：设置对话框的初始时长（帧单位）
    // 参数为待设置的时长（帧数值），用于在打开对话框时显示默认值
    void setDuration(int);
    // 公共方法：获取用户设置的时长（帧单位）
    // 返回值为用户在对话框中输入的最终时长，供外部调用者使用
    int duration() const;

private:
    // 私有成员变量：指向UI界面对象的指针
    // 由Qt Designer生成，用于访问对话框中的控件（如时间输入框）
    Ui::DurationDialog *ui;
};

// 结束头文件保护宏
#endif // DURATIONDIALOG_H
