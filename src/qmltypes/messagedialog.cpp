/*
 * Copyright (c) 2023 Meltytech, LLC
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

#include "messagedialog.h"

#include "Logger.h"
#include "qmlapplication.h"

#include <QApplication>

// 【构造函数】
// 【功能】：初始化消息对话框对象
// 【参数】：parent - 父对象指针，用于Qt对象树管理
MessageDialog::MessageDialog(QObject *parent)
    : QObject{parent}
    , m_buttons{0}  // 默认无按钮（信息提示框）
{}

// 【核心功能】：打开消息对话框
// 【说明】：根据配置显示不同类型的消息框（信息、询问等）
// 【流程】：配置对话框 → 设置模态 → 显示并等待用户响应
void MessageDialog::open()
{
    QMessageBox dialog;
    
    // 【按钮配置逻辑】：根据按钮类型设置不同的对话框行为
    if (m_buttons & QMessageBox::No) {
        // 【询问对话框】：包含"是/否"选择
        dialog.setIcon(QMessageBox::Question);      // 设置问号图标
        dialog.setStandardButtons(QMessageBox::StandardButtons(m_buttons)); // 设置标准按钮
        dialog.setDefaultButton(QMessageBox::Yes);  // 默认选中"是"
        dialog.setEscapeButton(QMessageBox::No);    // ESC键对应"否"
    } else if (!m_buttons) {
        // 【信息提示框】：只有确定按钮
        dialog.setIcon(QMessageBox::Information);   // 设置信息图标
        dialog.setDefaultButton(QMessageBox::Ok);   // 默认确定按钮
    } else {
        // 【自定义按钮】：使用指定的按钮组合
        dialog.setStandardButtons(QMessageBox::StandardButtons(m_buttons));
    }
    
    // 【标题设置】：使用自定义标题或应用程序名称
    if (!m_title.isEmpty()) {
        dialog.setWindowTitle(m_title);  // 使用自定义标题
    } else {
        dialog.setWindowTitle(QApplication::applicationName()); // 使用应用名称作为默认标题
    }
    
    // 设置消息文本和模态
    dialog.setText(m_text);  // 设置主要消息内容
    dialog.setWindowModality(QmlApplication::dialogModality()); // 设置对话框模态性
    
    // 显示对话框并等待用户响应
    auto button = QMessageBox::StandardButton(dialog.exec());
    
    // 处理用户响应
    if (QMessageBox::Ok == button || QMessageBox::Yes == button) {
        emit accepted();  // 用户点击确定/是，发射接受信号
    } else {
        emit rejected();  // 用户点击取消/否，发射拒绝信号
    }
}

// 【功能】：设置对话框标题
// 【参数】：title - 新的标题文字
// 【说明】：标题改变时会发射titleChanged信号通知QML更新
void MessageDialog::setTitle(const QString &title)
{
    if (title != m_title) {
        m_title = title;
        emit titleChanged(title);  // 通知QML界面标题已更新
    }
}

// 【功能】：设置消息文本内容
// 【参数】：text - 新的消息文本
// 【说明】：文本改变时会发射textChanged信号通知QML更新
void MessageDialog::setText(const QString &text)
{
    if (text != m_text) {
        m_text = text;
        emit textChanged(text);  // 通知QML界面文本已更新
    }
}

// 【功能】：设置对话框按钮组合
// 【参数】：buttons - 按钮标志位组合（QMessageBox::StandardButton枚举）
// 【说明】：支持多种标准按钮组合，如Ok、Yes|No、Ok|Cancel等
void MessageDialog::setButtons(int buttons)
{
    if (buttons != m_buttons) {
        m_buttons = buttons;
        emit buttonsChanged(buttons);  // 通知QML界面按钮设置已更新
    }
}
