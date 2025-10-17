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
#ifndef MESSAGEDIALOG_H
#define MESSAGEDIALOG_H

#include <QMessageBox>

// 【类说明】：消息对话框的QML包装类
// 【功能】：在QML中创建和管理系统消息对话框
// 【特性】：支持信息提示、询问确认、自定义按钮组合
class MessageDialog : public QObject
{
    Q_OBJECT
    // 【QML属性】：对话框标题文字，可读写
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    // 【QML属性】：消息正文内容，可读写
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    // 【QML属性】：按钮组合标志，可读写
    Q_PROPERTY(int buttons READ buttons WRITE setButtons NOTIFY buttonsChanged)

public:
    // 【枚举】：标准按钮类型定义
    // 【说明】：包装QMessageBox的标准按钮，便于QML使用
    enum StandardButtons {
        Ok = QMessageBox::Ok,         // 确定按钮
        Yes = QMessageBox::Yes,       // 是按钮
        No = QMessageBox::No,         // 否按钮
        Cancel = QMessageBox::Cancel  // 取消按钮
    };
    Q_ENUM(StandardButtons)  // 注册到QML系统
    
    // 【构造函数】
    explicit MessageDialog(QObject *parent = nullptr);

    // 【可调用方法】：打开消息对话框
    // 【说明】：Q_INVOKABLE宏使该方法可以在QML中直接调用
    Q_INVOKABLE void open();

signals:
    // 【信号】：对话框标题改变时发射
    void titleChanged(const QString &title);
    // 【信号】：消息文本改变时发射
    void textChanged(const QString &text);
    // 【信号】：按钮组合改变时发射
    void buttonsChanged(int buttons);
    // 【信号】：用户接受（确认）对话框时发射
    void accepted();
    // 【信号】：用户拒绝（取消）对话框时发射
    void rejected();

private:
    QString m_title;    // 存储对话框标题
    QString m_text;     // 存储消息正文内容
    int m_buttons;      // 存储按钮组合标志

    // 【私有方法】：获取对话框标题
    QString title() const { return m_title; }
    // 【私有方法】：设置对话框标题
    void setTitle(const QString &title);
    // 【私有方法】：获取消息正文
    QString text() const { return m_text; }
    // 【私有方法】：设置消息正文
    void setText(const QString &text);
    // 【私有方法】：获取按钮组合
    int buttons() const { return m_buttons; }
    // 【私有方法】：设置按钮组合
    void setButtons(int buttons);
};

#endif // MESSAGEDIALOG_H
