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

#ifndef COLORDIALOG_H
#define COLORDIALOG_H

#include <QColor>
#include <QObject>

// 【类说明】：颜色对话框的QML包装类
// 【功能】：在QML中创建和管理系统颜色选择对话框
// 【特性】：支持颜色选择、透明度设置、自定义标题
class ColorDialog : public QObject
{
    Q_OBJECT
// 【QML属性】：当前选中的颜色，可读写
    Q_PROPERTY(
        QColor selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged)

// 【QML属性】：对话框标题文字，可读写
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

public:
 // 【构造函数】
    // 【参数】：parent - 父对象指针，用于Qt对象树管理
    explicit ColorDialog(QObject *parent = nullptr);

// 【可调用方法】：打开颜色选择对话框
    // 【说明】：Q_INVOKABLE宏使该方法可以在QML中直接调用
    // 【流程】：弹出系统颜色选择器，处理用户选择结果
    Q_INVOKABLE void open();

signals:
// 【信号】：选中颜色改变时发射，携带新的颜色值
    void selectedColorChanged(const QColor &color);

 // 【信号】：用户接受（确认）颜色选择时发射
    void accepted();

 // 【信号】：对话框标题改变时发射
    void titleChanged();

private:
    QColor m_color;// 存储当前选中的颜色值
    QString m_title;// 存储对话框标题文字

// 【私有方法】：获取当前选中的颜色
    QColor selectedColor() const { return m_color; }

// 【私有方法】：设置选中颜色，会检查颜色是否实际改变
    void setSelectedColor(const QColor &color);

// 【私有方法】：获取对话框标题
    QString title() const { return m_title; }

// 【私有方法】：设置对话框标题，会检查标题是否实际改变
    void setTitle(const QString &title);
};

#endif // COLORDIALOG_H

