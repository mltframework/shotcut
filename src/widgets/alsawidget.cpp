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

#include "alsawidget.h"
#include "ui_alsawidget.h"

#include "mltcontroller.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

/**
 * @class AlsaWidget
 * @brief ALSA音频输入设备配置控件
 * 
 * 该控件提供ALSA（Advanced Linux Sound Architecture）音频输入设备的
 * 配置界面，允许用户选择音频设备、设置通道数，并管理配置预设。
 * 主要用于Linux系统下的音频采集功能。
 */

/**
 * @brief 构造函数
 * @param parent 父控件指针
 * 
 * 初始化ALSA配置界面，设置默认值并加载预设配置。
 * 界面包含设备名称输入框、通道数设置和预设管理功能。
 */
AlsaWidget::AlsaWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AlsaWidget)
{
    // 初始化UI组件
    ui->setupUi(this);
    
    // 设置标签高亮颜色
    Util::setColorsToHighlight(ui->label_2);
    
    // 初始隐藏应用按钮
    ui->applyButton->hide();
    
    // 保存当前配置为默认预设
    ui->preset->saveDefaultPreset(getPreset());
    
    // 加载所有可用预设
    ui->preset->loadPresets();
    
    // 从设置中加载音频输入设备名称
    ui->lineEdit->setText(Settings.audioInput());
}

/**
 * @brief 析构函数
 * 
 * 清理UI资源，确保内存正确释放。
 */
AlsaWidget::~AlsaWidget()
{
    delete ui;
}

/**
 * @brief 创建新的ALSA生产者对象
 * @param profile MLT配置信息
 * @return 新创建的ALSA生产者指针
 * 
 * 根据当前界面配置创建ALSA音频输入生产者：
 * - 使用指定的音频设备名称（默认为"default"）
 * - 设置音频通道数（如果大于0）
 * - 标记为后台采集属性
 * - 更新全局音频输入设置
 * 
 * @note 调用者负责管理返回的生产者对象内存
 */
Mlt::Producer *AlsaWidget::newProducer(Mlt::Profile &profile)
{
    // 构建ALSA资源字符串
    QString s("alsa:%1");
    if (ui->lineEdit->text().isEmpty())
        s = s.arg("default");  // 使用默认设备
    else
        s = s.arg(ui->lineEdit->text());  // 使用用户指定设备
    
    // 如果设置了通道数，添加到资源字符串
    if (ui->alsaChannelsSpinBox->value() > 0)
        s += QStringLiteral("?channels=%1").arg(ui->alsaChannelsSpinBox->value());
    
    // 创建MLT生产者对象
    Mlt::Producer *p = new Mlt::Producer(profile, s.toUtf8().constData());
    
    // 设置生产者属性
    p->set(kBackgroundCaptureProperty, 1);      // 标记为后台采集
    p->set(kShotcutCaptionProperty, "ALSA");    // 设置显示名称
    
    // 保存音频输入设置到全局配置
    Settings.setAudioInput(ui->lineEdit->text());
    
    return p;
}

/**
 * @brief 获取当前配置的预设属性
 * @return 包含当前配置的MLT属性对象
 * 
 * 将当前界面配置封装为MLT属性对象，用于预设保存和加载：
 * - 资源字符串格式："alsa:设备名称"
 * - 音频通道数
 */
Mlt::Properties AlsaWidget::getPreset() const
{
    Mlt::Properties p;
    
    // 构建资源字符串
    QString s("alsa:%1");
    if (ui->lineEdit->text().isEmpty())
        s = s.arg("default");  // 默认设备
    else
        s = s.arg(ui->lineEdit->text());  // 用户指定设备
    
    // 设置属性值
    p.set("resource", s.toUtf8().constData());  // 资源路径
    p.set("channels", ui->alsaChannelsSpinBox->value());  // 通道数
    
    return p;
}

/**
 * @brief 加载预设配置到界面
 * @param p 包含预设配置的MLT属性对象
 * 
 * 从MLT属性对象解析并设置界面控件：
 * - 从资源字符串提取设备名称
 * - 设置音频通道数
 */
void AlsaWidget::loadPreset(Mlt::Properties &p)
{
    // 获取资源字符串
    QString s(p.get("resource"));
    
    // 解析设备名称（alsa:device_name格式）
    int i = s.indexOf(':');
    if (i > -1)
        ui->lineEdit->setText(s.mid(i + 1));  // 提取冒号后的设备名称
    
    // 设置通道数（如果预设中包含该属性）
    if (p.get("channels"))
        ui->alsaChannelsSpinBox->setValue(p.get_int("channels"));
}

/**
 * @brief 预设选择槽函数
 * @param p 预设属性的void指针
 * 
 * 当用户从预设列表中选择一个预设时调用，
 * 将void指针转换为MLT属性对象并加载到界面。
 */
void AlsaWidget::on_preset_selected(void *p)
{
    // 转换指针类型
    Mlt::Properties *properties = (Mlt::Properties *) p;
    
    // 加载预设配置
    loadPreset(*properties);
    
    // 清理内存
    delete properties;
}

/**
 * @brief 保存预设槽函数
 * 
 * 当用户点击保存预设按钮时调用，
 * 将当前配置保存为新的预设。
 */
void AlsaWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

/**
 * @brief 设置生产者并更新界面
 * @param producer MLT生产者对象
 * 
 * 当关联的生产者发生变化时调用，
 * 显示应用按钮并加载生产者的配置到界面。
 */
void AlsaWidget::setProducer(Mlt::Producer *producer)
{
    // 显示应用按钮
    ui->applyButton->show();
    
    // 如果生产者有效，加载其配置
    if (producer)
        loadPreset(*producer);
}

/**
 * @brief 应用按钮点击槽函数
 * 
 * 当用户点击应用按钮时调用，
 * 创建新的生产者并设置到MLT控制器，开始播放。
 */
void AlsaWidget::on_applyButton_clicked()
{
    // 创建新的生产者并设置到MLT
    MLT.setProducer(newProducer(MLT.profile()));
    
    // 开始播放
    MLT.play();
}
