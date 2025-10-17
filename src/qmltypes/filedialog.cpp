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

#include "filedialog.h"

#include "mainwindow.h"
#include "settings.h"
#include "util.h"

// 【构造函数】
// 【功能】：初始化文件对话框，创建QFileDialog实例并连接信号
// 【参数】：parent - 父对象指针，用于Qt对象树管理
FileDialog::FileDialog(QObject *parent)
    : QObject{parent}
{
    // 创建文件对话框实例，设置主窗口为父对象确保正确的窗口层级
    m_fileDialog.reset(new QFileDialog(&MAIN));
    
    // 【信号连接】：将底层对话框信号转发到QML可用的信号
    connect(m_fileDialog.get(), &QDialog::accepted, this, &FileDialog::accepted);
    connect(m_fileDialog.get(), &QDialog::rejected, this, &FileDialog::rejected);
    connect(m_fileDialog.get(), &QFileDialog::fileSelected, this, &FileDialog::fileSelected);
    connect(m_fileDialog.get(), &QFileDialog::filterSelected, this, &FileDialog::filterSelected);
}

// 【功能】：设置文件对话框模式
// 【参数】：mode - 文件模式（打开文件、保存文件等）
// 【说明】：此设置会影响对话框的行为和外观
void FileDialog::setFileMode(FileMode mode)
{
    m_fileMode = mode;
}

// 【功能】：获取对话框标题
// 【返回值】：当前对话框的标题文字
QString FileDialog::title() const
{
    return m_fileDialog->windowTitle();
}

// 【功能】：设置对话框标题
// 【参数】：title - 新的标题文字
// 【说明】：标题改变时会发射titleChanged信号通知QML更新
void FileDialog::setTitle(const QString &title)
{
    if (title != m_fileDialog->windowTitle()) {
        m_fileDialog->setWindowTitle(title);
        emit titleChanged(); // 通知QML界面标题已更新
    }
}

// 【功能】：获取文件名过滤器列表
// 【返回值】：当前设置的文件类型过滤器（如："Images (*.png *.jpg)"）
QStringList FileDialog::nameFilters() const
{
    return m_fileDialog->nameFilters();
}

// 【功能】：设置文件名过滤器
// 【参数】：filters - 文件类型过滤器列表
// 【说明】：用于限制用户在对话框中可以看到的文件类型
void FileDialog::setNameFilters(const QStringList &filters)
{
    if (filters != m_fileDialog->nameFilters()) {
        m_fileDialog->setNameFilters(filters);
        emit nameFiltersChanged(); // 通知QML过滤器已更新
    }
}

// 【功能】：获取用户选择的文件路径
// 【返回值】：第一个选中文件的完整路径
// 【注意】：假设至少有一个文件被选中，调用前应检查selectedFiles()是否为空
QString FileDialog::selectedFile()
{
    return m_fileDialog->selectedFiles().first();
}

// 【核心功能】：打开文件对话框
// 【说明】：根据文件模式配置对话框参数并显示给用户
void FileDialog::open()
{
    // 根据文件模式设置对话框行为
    if (m_fileMode == FileDialog::OpenFile) {
        m_fileDialog->setAcceptMode(QFileDialog::AcceptOpen); // 打开模式
        m_fileDialog->setDirectory(Settings.openPath());      // 设置默认打开目录
    } else {
        m_fileDialog->setAcceptMode(QFileDialog::AcceptSave); // 保存模式
        m_fileDialog->setDirectory(Settings.savePath());      // 设置默认保存目录
    }
    
    // 【平台差异处理】：不同操作系统的模态设置
    // macOS使用非模态对话框，其他系统使用应用模态对话框
#ifdef Q_OS_MAC
    m_fileDialog->setWindowModality(Qt::NonModal);    // macOS：非模态
#else
    m_fileDialog->setWindowModality(Qt::ApplicationModal); // 其他系统：应用模态
#endif
    
    // 应用工具类中的文件对话框选项配置
    m_fileDialog->setOptions(Util::getFileDialogOptions());
    
    // 显示对话框给用户
    m_fileDialog->open();
}
