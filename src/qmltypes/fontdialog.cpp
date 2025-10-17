/*
 * Copyright (c) 2023-2024 Meltytech, LLC
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

#include "fontdialog.h"

#include <QFontDialog>

// 【构造函数】
// 【功能】：初始化字体对话框对象
// 【参数】：parent - 父对象指针，用于Qt对象树管理
FontDialog::FontDialog(QObject *parent)
    : QObject{parent}
{}

// 【核心功能】：打开字体选择对话框
// 【说明】：弹出系统字体选择器，处理用户字体选择结果
// 【流程】：创建对话框 → 配置选项 → 等待用户选择 → 处理结果
void FontDialog::open()
{
    // 创建字体对话框，使用当前字体作为初始值
    QFontDialog dialog(m_font);
    dialog.setModal(true); // 设置为模态对话框，阻塞用户与其他窗口交互
    
    // 【平台特定配置】：在Unix系统（非macOS）上不使用原生对话框
    // 【原因】：确保跨平台一致的界面体验，避免不同桌面环境的差异
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    dialog.setOption(QFontDialog::DontUseNativeDialog);
#endif

    // 显示对话框并等待用户操作
    if (dialog.exec() == QDialog::Accepted) {
        // 用户点击"确定"：获取选择的字体并更新状态
        setSelectedFont(dialog.currentFont());
        emit accepted(); // 发射接受信号，通知QML用户确认选择
    } else {
        // 用户点击"取消"或关闭对话框
        emit rejected(); // 发射拒绝信号，通知QML用户取消操作
    }
}

// 【功能】：设置选中的字体
// 【参数】：font - 新的字体对象
// 【说明】：会检查字体是否实际改变，避免不必要的信号发射
void FontDialog::setSelectedFont(const QFont &font)
{
    // 只有字体真正改变时才更新并发射信号
    if (font != m_font) {
        m_font = font; // 更新内部字体状态
        emit selectedFontChanged(font); // 通知绑定属性更新
    }
}
