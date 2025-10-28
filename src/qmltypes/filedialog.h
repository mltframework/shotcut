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

#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <QFileDialog>

// 【类说明】：文件对话框的QML包装类
// 【功能】：将Qt的文件对话框功能暴露给QML使用，提供文件选择功能
// 【特性】：支持打开/保存模式、文件类型过滤、自定义标题等
class FileDialog : public QObject
{
    Q_OBJECT
    // 【QML属性】：文件对话框模式（打开文件/保存文件）
    Q_PROPERTY(FileDialog::FileMode fileMode READ fileMode WRITE setFileMode NOTIFY fileModeChanged)
    
    // 【QML属性】：对话框标题文字
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    
    // 【QML属性】：文件名过滤器列表（如："Images (*.png *.jpg)"）
    Q_PROPERTY(
        QStringList nameFilters READ nameFilters WRITE setNameFilters NOTIFY nameFiltersChanged)
    
    // 【QML属性】：用户选择的文件路径（只读属性）
    Q_PROPERTY(QString selectedFile READ selectedFile NOTIFY fileSelected)

public:
    // 【枚举】：文件对话框的工作模式
    enum FileMode { 
        OpenFile,  // 打开文件模式 - 用于选择现有文件
        SaveFile   // 保存文件模式 - 用于指定新文件保存路径
    };
    Q_ENUM(FileMode) // 将枚举注册到QML系统，使其在QML中可用

    // 【构造函数】
    // 【参数】：parent - 父对象指针，用于Qt对象树管理
    explicit FileDialog(QObject *parent = nullptr);
    
    // 【功能】：获取当前文件模式
    FileDialog::FileMode fileMode() const { return m_fileMode; }
    
    // 【功能】：设置文件模式
    void setFileMode(FileDialog::FileMode mode);
    
    // 【功能】：获取对话框标题
    QString title() const;
    
    // 【功能】：设置对话框标题
    void setTitle(const QString &title);
    
    // 【功能】：获取文件名过滤器列表
    QStringList nameFilters() const;
    
    // 【功能】：设置文件名过滤器
    void setNameFilters(const QStringList &filters);
    
    // 【功能】：获取用户选择的文件路径
    QString selectedFile();
    
    // 【可调用方法】：打开文件对话框
    // 【说明】：Q_INVOKABLE宏使该方法可以在QML中直接调用
    Q_INVOKABLE void open();

signals:
    // 【信号】：文件模式改变时发射
    void fileModeChanged();
    
    // 【信号】：对话框标题改变时发射
    void titleChanged();
    
    // 【信号】：文件名过滤器改变时发射
    void nameFiltersChanged();
    
    // 【信号】：用户选择文件时发射，携带文件路径
    void fileSelected(const QString &file);
    
    // 【信号】：用户选择过滤器时发射，携带过滤器文本
    void filterSelected(const QString &filter);
    
    // 【信号】：用户接受（确认）对话框时发射
    void accepted();
    
    // 【信号】：用户拒绝（取消）对话框时发射
    void rejected();

private:
    FileDialog::FileMode m_fileMode{FileDialog::OpenFile}; // 默认模式为打开文件
    std::unique_ptr<QFileDialog> m_fileDialog; // 使用智能指针管理Qt文件对话框实例
};

#endif // FILEDIALOG_H
