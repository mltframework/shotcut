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

#ifndef FONTDIALOG_H
#define FONTDIALOG_H

#include <QFont>
#include <QObject>

// 【类说明】：字体对话框的QML包装类
// 【功能】：在QML中创建和管理系统字体选择对话框
// 【特性】：支持字体家族、样式、大小选择，跨平台一致体验
class FontDialog : public QObject
{
    Q_OBJECT
    // 【QML属性】：当前选中的字体，可读写
    // 【说明】：包含字体家族、大小、样式、粗细等完整字体属性
    Q_PROPERTY(QFont selectedFont READ selectedFont WRITE setSelectedFont NOTIFY selectedFontChanged)

public:
    // 【构造函数】
    // 【参数】：parent - 父对象指针，用于Qt对象树管理
    FontDialog(QObject *parent = nullptr);

    // 【可调用方法】：打开字体选择对话框
    // 【说明】：Q_INVOKABLE宏使该方法可以在QML中直接调用
    // 【平台差异】：在Unix系统（非macOS）上使用Qt标准对话框而非原生对话框
    Q_INVOKABLE void open();

signals:
    // 【信号】：用户接受（确认）字体选择时发射
    void accepted();
    
    // 【信号】：用户拒绝（取消）字体选择时发射
    void rejected();
    
    // 【信号】：选中字体改变时发射，携带新的字体对象
    // 【参数】：font - 用户选择的新字体，包含所有字体属性
    void selectedFontChanged(const QFont &font);

private:
    QFont m_font; // 存储当前选中的字体对象

    // 【私有方法】：获取当前选中的字体
    // 【返回值】：当前字体对象，包含完整的字体属性信息
    QFont selectedFont() const { return m_font; }
    
    // 【私有方法】：设置选中字体，会检查字体是否实际改变
    // 【参数】：font - 新的字体对象
    // 【说明】：只有字体真正改变时才更新内部状态并发射信号
    void setSelectedFont(const QFont &font);
};

#endif // FONTDIALOG_H
