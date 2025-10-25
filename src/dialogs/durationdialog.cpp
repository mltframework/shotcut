/*
 * Copyright (c) 2012-2025 Meltytech, LLC
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
#include "durationdialog.h"
// 包含MLT控制器头文件（获取最大帧计数等视频相关配置）
#include "mltcontroller.h"
// 包含UI文件生成的头文件（用于访问界面控件）
#include "ui_durationdialog.h"


//【 构造函数 】：初始化时长设置对话框 
DurationDialog::DurationDialog(QWidget *parent)
    : QDialog(parent)  // 初始化父类QDialog，指定父窗口
    , ui(new Ui::DurationDialog)  // 初始化UI指针，创建界面对象
{
    ui->setupUi(this);  // 加载UI布局，初始化界面控件（如时间输入框）
    
    // 设置时间输入框（spinBox）的最大值为MLT支持的最大帧计数
    // 确保输入的时长不超过视频的最大可能帧数
    ui->spinBox->setMaximum(MLT.maxFrameCount());
    
    // 连接时间输入框的"确认"信号到对话框的"接受"槽函数
    // 当用户在输入框中确认输入（如按回车）时，直接关闭对话框并返回确认状态
    connect(ui->spinBox, &TimeSpinBox::accepted, this, &QDialog::accept);
}

//【 析构函数 】：释放资源 
DurationDialog::~DurationDialog()
{
    delete ui;  // 释放UI对象占用的内存，避免内存泄漏
}

// 【 公共方法】：设置对话框的初始时长（帧单位）
void DurationDialog::setDuration(int frames)
{
    // 将传入的帧数值设置为时间输入框的初始值
    ui->spinBox->setValue(frames);
}

// 【公共方法】：获取用户设置的时长（帧单位）
int DurationDialog::duration() const
{
    // 返回时间输入框中用户输入的帧数值
    return ui->spinBox->value();
}
