/*
 * Copyright (c) 2017-2024 Meltytech, LLC
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
#include "transcodedialog.h"
// 包含UI文件生成的头文件（用于访问界面控件）
#include "ui_transcodedialog.h"

// 引入依赖的业务逻辑头文件
#include "mltcontroller.h"  // MLT控制器（获取当前视频配置，如帧率）
#include "settings.h"       // 配置类（读取/保存转码高级模式等偏好）

// 引入Qt相关头文件
#include <QPushButton>  // 按钮控件（添加“高级”按钮）


// 【构造函数】：初始化转码配置对话框
// 参数说明：
// - message：转码提示文本（如格式选择说明）
// - isProgressive：当前视频是否为逐行扫描（影响中间格式选择）
// - parent：父窗口指针
TranscodeDialog::TranscodeDialog(const QString &message, bool isProgressive, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TranscodeDialog)  // 初始化UI指针，创建界面对象
    , m_format(0)                  // 初始转码格式索引（0=有损，1=中间，2=无损）
    , m_isChecked(false)           // 初始勾选状态（用于自定义复选框）
    , m_isProgressive(isProgressive)  // 标记视频是否逐行扫描
{
    ui->setupUi(this);  // 加载UI布局，初始化界面控件

    // 1. 基础配置：窗口标题、提示文本、隐藏默认复选框
    setWindowTitle(tr("Convert to Edit-friendly..."));  // 窗口标题（“转换为易编辑格式...”）
    ui->messageLabel->setText(message);                  // 设置转码提示文本
    ui->checkBox->hide();                                // 隐藏默认复选框（按需通过showCheckBox()显示）
    ui->subclipCheckBox->hide();                         // 隐藏子片段复选框（按需通过showSubClipCheckBox()显示）
    ui->deinterlaceCheckBox->setChecked(false);          // 去隔行复选框默认未勾选

    // 2. 帧率配置区：绑定“启用帧率覆盖”复选框与相关控件的启用状态
    connect(ui->fpsCheckBox, SIGNAL(toggled(bool)), ui->fpsWidget, SLOT(setEnabled(bool)));
    connect(ui->fpsCheckBox, SIGNAL(toggled(bool)), ui->fpsLabel, SLOT(setEnabled(bool)));
    connect(ui->fpsCheckBox, SIGNAL(toggled(bool)), ui->frcComboBox, SLOT(setEnabled(bool)));
    connect(ui->fpsCheckBox, SIGNAL(toggled(bool)), ui->frcLabel, SLOT(setEnabled(bool)));
    ui->fpsCheckBox->setChecked(false);  // 默认不启用帧率覆盖
    ui->fpsWidget->setEnabled(false);    // 帧率输入框默认禁用
    ui->fpsLabel->setEnabled(false);     // 帧率标签默认禁用
    ui->frcComboBox->setEnabled(false);  // 帧率转换模式下拉框默认禁用
    ui->frcLabel->setEnabled(false);     // 帧率转换模式标签默认禁用

    // 3. 初始化帧率输入框（默认值为当前MLT配置的帧率）
    ui->fpsWidget->setFps(MLT.profile().fps());

    // 4. 填充帧率转换模式下拉框（文本显示用户可见名称，数据存储模式标识）
    ui->frcComboBox->addItem(tr("Duplicate (fast)"), QVariant("dup"));          // 复制帧（快速）
    ui->frcComboBox->addItem(tr("Blend"), QVariant("blend"));                    // 混合帧
    ui->frcComboBox->addItem(tr("Motion Compensation (slow)"), QVariant("mci")); // 运动补偿（慢速）
    ui->frcComboBox->setCurrentIndex(0);  // 默认选中“复制帧”

    // 5. 添加“高级”按钮（控制高级配置区的显示/隐藏）
    QPushButton *advancedButton = new QPushButton(tr("Advanced"));
    advancedButton->setCheckable(true);  // 按钮可勾选（切换高级模式）
    // 绑定按钮勾选状态与高级配置区的可见性
    connect(advancedButton, SIGNAL(toggled(bool)), ui->advancedWidget, SLOT(setVisible(bool)));
    // 根据配置决定高级区初始状态（未启用则隐藏）
    if (!Settings.convertAdvanced()) {
        ui->advancedWidget->hide();
    }
    advancedButton->setChecked(Settings.convertAdvanced());  // 按钮初始勾选状态与配置一致
    ui->advancedCheckBox->setChecked(Settings.convertAdvanced());  // 高级模式复选框与配置一致
    // 将“高级”按钮添加到按钮组（动作角色）
    ui->buttonBox->addButton(advancedButton, QDialogButtonBox::ActionRole);

    // 6. 初始化转码格式标签（根据初始格式索引0更新文本）
    on_horizontalSlider_valueChanged(m_format);
}

// 【析构函数】：释放UI资源
TranscodeDialog::~TranscodeDialog()
{
    delete ui;  // 释放UI对象占用的内存
}


// 【公共方法】：显示默认复选框（用于自定义勾选逻辑，如“应用到所有”）
void TranscodeDialog::showCheckBox()
{
    ui->checkBox->show();
}


// 【公共方法】：获取“去隔行”选项的勾选状态
// 返回值：bool - true=启用去隔行，false=禁用
bool TranscodeDialog::deinterlace() const
{
    return ui->deinterlaceCheckBox->isChecked();
}


// 【公共方法】：获取“启用帧率覆盖”选项的勾选状态
// 返回值：bool - true=启用帧率覆盖，false=禁用
bool TranscodeDialog::fpsOverride() const
{
    return ui->fpsCheckBox->isChecked();
}


// 【公共方法】：获取用户设置的目标帧率
// 返回值：double - 目标帧率（仅当fpsOverride()为true时有效）
double TranscodeDialog::fps() const
{
    return ui->fpsWidget->fps();
}


// 【公共方法】：获取选中的帧率转换模式
// 返回值：QString - 模式标识（如“dup”“blend”“mci”）
QString TranscodeDialog::frc() const
{
    // Frame Rate Conversion Mode（帧率转换模式）
    return ui->frcComboBox->currentData().toString();
}


// 【公共方法】：获取“转换为709色彩空间”选项的勾选状态
// 返回值：bool - true=启用转换，false=禁用
bool TranscodeDialog::get709Convert()
{
    return ui->convert709CheckBox->isChecked();
}


// 【公共方法】：设置“转换为709色彩空间”选项的勾选状态
// 参数：enable - true=勾选，false=取消勾选
void TranscodeDialog::set709Convert(bool enable)
{
    ui->convert709CheckBox->setChecked(enable);
}


// 【公共方法】：获取选中的音频采样率
// 返回值：QString - 采样率字符串（如“44100”“48000”，未选中则为空）
QString TranscodeDialog::sampleRate() const
{
    QString sampleRate;
    if (ui->sampleRateComboBox->currentIndex() == 1) {
        sampleRate = "44100";  // 第1项（索引1）为44100Hz
    } else if (ui->sampleRateComboBox->currentIndex() == 2) {
        sampleRate = "48000";  // 第2项（索引2）为48000Hz
    }
    return sampleRate;
}


// 【公共方法】：显示“子片段”复选框（用于转码时仅处理子片段）
void TranscodeDialog::showSubClipCheckBox()
{
    ui->subclipCheckBox->show();
}


// 【公共方法】：获取“子片段”选项的勾选状态
// 返回值：bool - true=仅转码子片段，false=转码完整片段
bool TranscodeDialog::isSubClip() const
{
    return ui->subclipCheckBox->isChecked();
}


// 【公共方法】：设置“子片段”选项的勾选状态
// 参数：checked - true=勾选，false=取消勾选
void TranscodeDialog::setSubClipChecked(bool checked)
{
    ui->subclipCheckBox->setChecked(checked);
}


// 【公共方法】：设置目标帧率并启用帧率覆盖
// 参数：fps - 目标帧率
void TranscodeDialog::setFrameRate(double fps)
{
    ui->fpsCheckBox->setChecked(true);  // 启用帧率覆盖
    ui->fpsWidget->setFps(fps);         // 设置目标帧率
}


// 【槽函数】：处理转码格式滑块值变化事件（更新格式标签文本）
// 参数：position - 滑块位置（0=有损，1=中间，2=无损）
void TranscodeDialog::on_horizontalSlider_valueChanged(int position)
{
    switch (position) {
    case 0:
        // 有损格式：仅I帧的H.264/AC-3 MP4
        ui->formatLabel->setText(tr("Lossy: I-frame–only %1").arg("H.264/AC-3 MP4"));
        break;
    case 1:
        // 中间格式：逐行→DNxHR/PCM MOV，隔行→ProRes/PCM MOV
        ui->formatLabel->setText(
            tr("Intermediate: %1").arg(m_isProgressive ? "DNxHR/PCM MOV" : "ProRes/PCM MOV"));
        break;
    case 2:
        // 无损格式：Ut Video/PCM MKV
        ui->formatLabel->setText(tr("Lossless: %1").arg("Ut Video/PCM MKV"));
        break;
    }
    m_format = position;  // 更新当前格式索引
}


// 【槽函数】：处理默认复选框点击事件（更新勾选状态）
// 参数：checked - 复选框勾选状态
void TranscodeDialog::on_checkBox_clicked(bool checked)
{
    m_isChecked = checked;
}


// 【槽函数】：处理“高级模式”复选框点击事件（保存配置）
// 参数：checked - 复选框勾选状态
void TranscodeDialog::on_advancedCheckBox_clicked(bool checked)
{
    Settings.setConvertAdvanced(checked);  // 将高级模式状态保存到配置
}
