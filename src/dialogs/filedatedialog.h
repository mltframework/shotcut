/*
 * Copyright (c) 2019 Meltytech, LLC
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

// 头文件重复包含保护宏：未定义FILEDATEDIALOG_H时，执行后续内容
#ifndef FILEDATEDIALOG_H
#define FILEDATEDIALOG_H

// 引入Qt标准对话框头文件（FileDateDialog继承自QDialog）
#include <QDialog>

// 前向声明：避免包含不必要的头文件，减少编译依赖
class QComboBox;                // 下拉选择框（用于展示可选日期列表）
class QDateTimeEdit;            // 日期时间编辑框（用于手动修改日期时间）
namespace Mlt {
class Producer;                 // MLT生产者类（关联待设置日期的音视频文件）
}

// 文件日期设置对话框类：用于设置或修改音视频文件的创建日期时间
class FileDateDialog : public QDialog
{
    Q_OBJECT  // Qt元对象系统宏，支持信号槽等Qt核心特性

public:
    // 【构造函数】：初始化文件日期设置对话框
    // 参数说明：
    // - title：对话框标题前缀（如“视频”“音频”，最终标题为“XXX File Date”）
    // - producer：MLT生产者对象（关联待设置日期的音视频文件，用于读取/写入日期）
    // - parent：父窗口指针（默认值0，用于Qt对象树管理）
    explicit FileDateDialog(QString title, Mlt::Producer *producer, QWidget *parent = 0);

private slots:
    // 【私有槽函数】：处理“确定”按钮点击事件
    // 功能：将编辑框中的日期时间保存到MLT生产者对象
    void accept();

    // 【私有槽函数】：处理下拉框选择变化事件
    // 参数：下拉框选中项的索引（-1表示未选中）
    // 功能：根据选中的索引，更新日期时间编辑框的内容
    void dateSelected(int index);

private:
    // 【私有方法】：填充日期选项到下拉框
    // 参数：MLT生产者对象（用于从多渠道获取可选日期）
    // 功能：从生产者当前日期、系统时间、文件系统、元数据中获取日期，添加到下拉框
    void populateDateOptions(Mlt::Producer *producer);

    // 【私有成员变量】：MLT生产者对象指针
    // 作用：关联待设置日期的音视频文件，用于读取现有日期和写入新日期
    Mlt::Producer *m_producer;

    // 【私有成员变量】：日期选择下拉框指针
    // 作用：展示多种可选日期（如当前值、系统时间、文件元数据日期）
    QComboBox *m_dtCombo;

    // 【私有成员变量】：日期时间编辑框指针
    // 作用：用于手动编辑日期时间，或显示下拉框选中的日期
    QDateTimeEdit *m_dtEdit;
};

// 结束头文件保护宏
#endif // FILEDATEDIALOG_H
