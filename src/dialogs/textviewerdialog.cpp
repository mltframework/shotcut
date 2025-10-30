/*
 * Copyright (c) 2012-2024 Meltytech, LLC
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

// 包含当前类的头文件
#include "textviewerdialog.h"
// 包含UI文件生成的头文件（用于访问界面控件）
#include "ui_textviewerdialog.h"

// 引入依赖的业务逻辑头文件
#include "settings.h"  // 配置类（读取默认保存目录）
#include "util.h"     // 工具类（获取文件对话框选项、检查文件可写性）

// 引入Qt相关头文件
#include <QClipboard>      // 剪贴板类（实现文本复制功能）
#include <QFileDialog>     // 文件对话框（实现文本保存功能）
#include <QPushButton>     // 按钮控件（添加“复制”按钮）
#include <QScrollBar>      // 滚动条类（控制文本区域滚动）


// 【构造函数】：初始化文本查看对话框
// 参数说明：
// - parent：父窗口指针
// - forMltXml：是否为MLT XML文件模式（true=保存为.mlt，false=保存为.txt）
TextViewerDialog::TextViewerDialog(QWidget *parent, bool forMltXml)
    : QDialog(parent)
    , ui(new Ui::TextViewerDialog)  // 初始化UI指针，创建界面对象
    , m_forMltXml(forMltXml)        // 标记是否为MLT XML模式（影响保存格式）
{
    ui->setupUi(this);  // 加载UI布局，初始化界面控件（文本显示区、按钮组等）

    // 1. 向按钮组添加“复制”按钮（自定义动作按钮，用于复制文本到剪贴板）
    auto button = ui->buttonBox->addButton(tr("Copy"), QDialogButtonBox::ActionRole);
    // 2. 连接“复制”按钮点击信号（点击后将文本区内容复制到系统剪贴板）
    connect(button, &QAbstractButton::clicked, this, [&]() {
        QGuiApplication::clipboard()->setText(ui->plainTextEdit->toPlainText());
    });
}

// 【析构函数】：释放UI资源
TextViewerDialog::~TextViewerDialog()
{
    delete ui;  // 释放UI对象占用的内存
}


// 【公共方法】：设置文本区内容并控制滚动
// 参数说明：
// - s：待显示的文本内容
// - scroll：是否自动滚动到文本底部（true=滚动，false=不滚动）
void TextViewerDialog::setText(const QString &s, bool scroll)
{
    // 仅当新文本与当前文本不同时，才更新文本区（避免重复刷新）
    if (s != ui->plainTextEdit->toPlainText()) {
        ui->plainTextEdit->setPlainText(s);  // 设置文本区内容
        
        // 若需要自动滚动，将垂直滚动条定位到最大值（即文本底部）
        if (scroll)
            ui->plainTextEdit->verticalScrollBar()->setValue(
                ui->plainTextEdit->verticalScrollBar()->maximum());
    }
}


// 【公共方法】：获取对话框的按钮组
// 返回值：QDialogButtonBox* - 对话框底部的按钮组对象（可用于自定义按钮逻辑）
QDialogButtonBox *TextViewerDialog::buttonBox() const
{
    return ui->buttonBox;
}


// 【槽函数】：处理“确定”按钮点击事件（保存文本到文件）
void TextViewerDialog::on_buttonBox_accepted()
{
    // 1. 初始化保存参数（从配置读取默认保存目录，设置对话框标题和文件过滤器）
    QString path = Settings.savePath();  // 默认保存目录
    QString caption = tr("Save Text");   // 对话框标题
    // 文件过滤器：根据模式选择（MLT XML模式→.mlt，普通模式→.txt）
    QString nameFilter = tr("Text Documents (*.txt);;All Files (*)");
    if (m_forMltXml) {
        nameFilter = tr("MLT XML (*.mlt);;All Files (*)");
        // MLT模式下，对话框标题改为“保存MLT XML”
    }

    // 2. 打开文件保存对话框（获取用户选择的保存路径）
    QString filename = QFileDialog::getSaveFileName(this,
                                                    caption,
                                                    path,
                                                    nameFilter,
                                                    nullptr,
                                                    Util::getFileDialogOptions());
    if (filename.isEmpty()) {
        return;  // 用户取消选择，终止保存操作
    }

    // 3. 处理文件名后缀（确保符合当前模式的格式）
    QFileInfo fi(filename);
    if (fi.suffix().isEmpty()) {  // 若文件名无后缀
        if (m_forMltXml)
            filename += ".mlt";    // MLT模式添加.mlt后缀
        else
            filename += ".txt";    // 普通模式添加.txt后缀
    }

    // 4. 检查文件是否可写（若不可写，弹出警告并返回）
    if (Util::warnIfNotWritable(filename, this, caption))
        return;

    // 5. 写入文件（将文本区内容以UTF-8编码写入目标文件）
    QFile f(filename);
    // 以“只写+文本模式”打开文件（文本模式会自动处理换行符）
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(ui->plainTextEdit->toPlainText().toUtf8());  // 写入文本
        f.close();  // 关闭文件
    }
}
