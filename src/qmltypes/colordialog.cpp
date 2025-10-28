/*
 * Copyright (c) 2023-2025 Meltytech, LLC
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

#include "colordialog.h"
#include "util.h"

#include <QColorDialog>

// 【构造函数】
// 【功能】：初始化颜色对话框对象
// 【参数】：parent - 父对象指针，用于Qt对象树管理
ColorDialog::ColorDialog(QObject *parent)
    : QObject{parent}
{}

// 【功能】：打开颜色选择对话框并处理用户选择
// 【说明】：这是核心交互方法，弹出系统颜色选择器，获取用户选择的颜色
void ColorDialog::open()
{
     // 保存当前颜色值作为初始值
    auto color = m_color;
    
    // 配置对话框选项：显示Alpha通道（透明度）
    QColorDialog::ColorDialogOptions flags = QColorDialog::ShowAlphaChannel;
    // 合并工具类中的其他对话框选项
    flags |= Util::getColorDialogOptions();

    // 弹出系统颜色选择对话框
    // 【参数说明】：
    // color - 初始颜色
    // nullptr - 父窗口（无）
    // m_title - 对话框标题
    // flags - 对话框选项标志
    auto newColor = QColorDialog::getColor(color, nullptr, m_title, flags);

    // 检查用户是否选择了有效颜色（点击了OK按钮）
    if (newColor.isValid()) {
        // 【透明度处理逻辑】：
        // 当用户选择完全透明颜色时，需要特殊处理以避免显示问题
        auto rgb = newColor;
        auto transparent = QColor(0, 0, 0, 0);// 完全透明的黑色

        // 比较RGB分量（忽略Alpha），判断颜色是否实质改变
        rgb.setAlpha(color.alpha());

        // 条件：用户选择了完全透明，且颜色值有实质变化
        if (newColor.alpha() == 0
            && (rgb != color || (newColor == transparent && color == transparent))) {
            // 将透明度恢复为不透明，避免显示问题
            newColor.setAlpha(255);
        }

        // 更新选择的颜色并发出接受信号
        setSelectedColor(newColor);
        emit accepted();// 通知QML界面用户已确认选择
    }
     // 注意：如果用户点击Cancel，这里不会执行任何操作
}

// 【功能】：设置选中的颜色值
// 【参数】：color - 新的颜色值
// 【说明】：会检查颜色是否实际改变，避免不必要的信号发射
void ColorDialog::setSelectedColor(const QColor &color)
{
    // 只有颜色真正改变时才更新并发射信号
    if (color != m_color) {
        m_color = color;
        emit selectedColorChanged(color);// 通知绑定属性更新
    }
}

// 【功能】：设置颜色对话框的标题
// 【参数】：title - 对话框标题文字
void ColorDialog::setTitle(const QString &title)
{
    if (title != m_title) {
        m_title = title;
        emit titleChanged();// 通知QML界面标题已更新
    }
}

