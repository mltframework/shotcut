/*
 * Copyright (c) 2012-2022 Meltytech, LLC
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

// 包含对应的头文件，声明了AddEncodePresetDialog类
#include "addencodepresetdialog.h"
// 包含UI文件生成的头文件，用于访问界面控件
#include "ui_addencodepresetdialog.h"

// AddEncodePresetDialog类的构造函数
// 参数parent为父窗口指针，用于Qt的对象树管理
AddEncodePresetDialog::AddEncodePresetDialog(QWidget *parent)
    : QDialog(parent)  // 初始化父类QDialog，指定父窗口
    , ui(new Ui::AddEncodePresetDialog)  // 初始化UI指针，创建UI对象
{
    // 调用UI的setupUi方法，初始化对话框界面（加载UI布局、创建控件等）
    ui->setupUi(this);
}

// AddEncodePresetDialog类的析构函数
AddEncodePresetDialog::~AddEncodePresetDialog()
{
    // 释放UI对象占用的内存
    delete ui;
}

// 用于设置编码预设的属性文本
// 参数properties为要显示的属性字符串
void AddEncodePresetDialog::setProperties(const QString &properties)
{
    // 将属性文本设置到UI中的propertiesEdit控件（多行文本编辑框）
    ui->propertiesEdit->setPlainText(properties);
}

// 获取用户输入的预设名称
// 返回值为用户在名称输入框中输入的文本
QString AddEncodePresetDialog::presetName() const
{
    return ui->nameEdit->text();  // 从nameEdit控件（单行文本框）获取文本
}

// 获取编码预设的完整属性（包含扩展信息）
// 返回值为整合后的属性字符串
QString AddEncodePresetDialog::properties() const
{
    // 获取用户输入的文件扩展名（从extensionEdit控件）
    const auto &extension = ui->extensionEdit->text();
    // 如果扩展名不为空，则在原有属性后追加扩展名信息
    if (!extension.isEmpty()) {
        return ui->propertiesEdit->toPlainText() + "\nmeta.preset.extension=" + extension;
    }
    // 如果扩展名为空，直接返回原有属性文本
    return ui->propertiesEdit->toPlainText();
}
