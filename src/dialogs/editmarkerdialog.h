/*
 * Copyright (c) 2021 Meltytech, LLC
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

// 头文件重复包含保护宏：未定义EDITMARKERDIALOG_H时，执行后续内容
#ifndef EDITMARKERDIALOG_H
#define EDITMARKERDIALOG_H

// 引入Qt标准对话框头文件（EditMarkerDialog继承自QDialog）
#include <QDialog>

// 前向声明：避免包含不必要的头文件，减少编译依赖
class EditMarkerWidget;       // 自定义标记编辑控件（封装标记编辑的核心UI和逻辑）
class QAbstractButton;        // Qt抽象按钮类（用于处理按钮点击事件）
class QDialogButtonBox;       // 对话框按钮组类（整合确定、取消等按钮）

// 标记编辑对话框类：用于编辑时间线上标记的文本、颜色、起始/结束帧位置
class EditMarkerDialog : public QDialog
{
    Q_OBJECT  // Qt元对象系统宏，支持信号槽等Qt核心特性

public:
    // 【构造函数】：初始化标记编辑对话框
    // 参数说明：
    // - parent：父窗口指针（用于Qt对象树管理）
    // - text：标记的初始文本（编辑前的内容）
    // - color：标记的初始颜色（编辑前的颜色）
    // - start：标记的初始起始帧位置
    // - end：标记的初始结束帧位置
    // - maxEnd：标记结束帧的最大值（限制结束帧不能超过此值，避免超出视频范围）
    explicit EditMarkerDialog(
        QWidget *parent, const QString &text, const QColor &color, int start, int end, int maxEnd);

    // 【公共方法】：获取用户编辑后的标记文本
    // 返回值：编辑完成后的标记文本
    QString getText();

    // 【公共方法】：获取用户选择的标记颜色
    // 返回值：编辑完成后的标记颜色（QColor类型）
    QColor getColor();

    // 【公共方法】：获取用户设置的标记起始帧位置
    // 返回值：编辑完成后的标记起始帧（整数）
    int getStart();

    // 【公共方法】：获取用户设置的标记结束帧位置
    // 返回值：编辑完成后的标记结束帧（整数）
    int getEnd();

private slots:
    // 【私有槽函数】：处理按钮点击事件（响应确定、取消等按钮操作）
    // 参数：被点击的按钮对象（QAbstractButton类型）
    void clicked(QAbstractButton *button);

private:
    // 【私有成员变量】：自定义标记编辑控件指针
    // 负责展示和编辑标记的文本、颜色、时间范围等核心内容
    EditMarkerWidget *m_sWidget;

    // 【私有成员变量】：对话框按钮组指针
    // 包含确定、取消等按钮，用于触发对话框的确认/取消逻辑
    QDialogButtonBox *m_buttonBox;
};

// 结束头文件保护宏
#endif // EDITMARKERDIALOG_H
