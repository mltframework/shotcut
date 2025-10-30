/*
 * Copyright (c) 2020 Meltytech, LLC
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
#include "systemsyncdialog.h"
// 包含UI文件生成的头文件（用于访问界面控件）
#include "ui_systemsyncdialog.h"

// 引入依赖的业务逻辑头文件
#include "mltcontroller.h"  // MLT控制器（更新消费者配置，使延迟生效）
#include "settings.h"       // 配置类（读取/保存视频延迟参数）


// 【构造函数】：初始化系统同步（视频延迟调节）对话框
// 参数：parent - 父窗口指针
SystemSyncDialog::SystemSyncDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SystemSyncDialog)  // 初始化UI指针，创建界面对象
    , m_oldValue(Settings.playerVideoDelayMs())  // 保存初始延迟值（用于取消操作时恢复）
{
    ui->setupUi(this);  // 加载UI布局，初始化界面控件

    // 1. 同步滑块和数值输入框的初始值（从配置中读取已保存的视频延迟）
    ui->syncSlider->setValue(Settings.playerVideoDelayMs());
    // 2. 初始隐藏“应用”按钮（未修改时无需显示）
    ui->applyButton->hide();
}

// 【析构函数】：释放UI资源
SystemSyncDialog::~SystemSyncDialog()
{
    delete ui;  // 释放UI对象占用的内存
}


// 【槽函数】：处理同步滑块释放事件（滑块拖动结束后应用延迟）
void SystemSyncDialog::on_syncSlider_sliderReleased()
{
    // 以滑块当前值作为新延迟，调用setDelay应用并保存
    setDelay(ui->syncSlider->value());
}


// 【槽函数】：处理延迟数值输入框编辑完成事件（输入框确认后同步滑块并应用延迟）
void SystemSyncDialog::on_syncSpinBox_editingFinished()
{
    // 1. 同步滑块值与输入框值（确保两者显示一致）
    ui->syncSlider->setValue(ui->syncSpinBox->value());
    // 2. 以输入框当前值作为新延迟，调用setDelay应用并保存
    setDelay(ui->syncSpinBox->value());
}


// 【槽函数】：处理对话框“取消”按钮点击事件（恢复初始延迟值）
void SystemSyncDialog::on_buttonBox_rejected()
{
    // 取消时，将延迟恢复为对话框打开前的初始值（m_oldValue）
    setDelay(m_oldValue);
}


// 【槽函数】：处理“撤销”按钮点击事件（重置延迟为0）
void SystemSyncDialog::on_undoButton_clicked()
{
    // 1. 同步滑块和输入框值为0
    ui->syncSlider->setValue(0);
    // 2. 应用延迟0并保存
    setDelay(0);
}


// 【槽函数】：处理延迟数值输入框值变化事件（显示“应用”按钮）
void SystemSyncDialog::on_syncSpinBox_valueChanged(int arg1)
{
    Q_UNUSED(arg1)  // 未使用参数（仅监听值变化事件）
    // 输入框值修改后，显示“应用”按钮（提示用户手动确认）
    ui->applyButton->show();
}


// 【槽函数】：处理“应用”按钮点击事件（手动应用输入框中的延迟值）
void SystemSyncDialog::on_applyButton_clicked()
{
    // 以输入框当前值作为新延迟，调用setDelay应用并保存
    setDelay(ui->syncSpinBox->value());
}


// 【私有方法】：设置视频延迟并更新配置与MLT
// 参数：delay - 新的视频延迟值（单位：毫秒）
void SystemSyncDialog::setDelay(int delay)
{
    // 仅当新延迟与当前配置不同时，才执行更新（避免重复操作）
    if (delay != Settings.playerVideoDelayMs()) {
        Settings.setPlayerVideoDelayMs(delay);  // 保存新延迟到配置
        MLT.consumerChanged();                  // 通知MLT更新消费者（使延迟生效）
    }
    // 应用延迟后，隐藏“应用”按钮（表示已生效）
    ui->applyButton->hide();
}
