/*
 * Copyright (c) 2012-2018 Meltytech, LLC
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

#ifndef ALSAWIDGET_H
#define ALSAWIDGET_H

#include "abstractproducerwidget.h"

#include <QWidget>

namespace Ui {
class AlsaWidget;
}

/**
 * @class AlsaWidget
 * @brief ALSA音频输入设备配置界面控件
 * 
 * 该类继承自QWidget和AbstractProducerWidget，提供ALSA音频输入设备的
 * 图形化配置界面。用户可以通过该控件选择音频设备、配置通道数，并管理
 * 设备配置预设。
 * 
 * 主要功能包括：
 * - ALSA设备选择和参数配置
 * - 配置预设的保存和加载
 * - 与MLT框架的生产者对象交互
 * 
 * @see AbstractProducerWidget
 */
class AlsaWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件指针，默认为nullptr
     * 
     * 初始化ALSA配置界面，设置默认参数并加载可用预设。
     */
    explicit AlsaWidget(QWidget *parent = 0);
    
    /**
     * @brief 析构函数
     * 
     * 清理界面资源，确保内存正确释放。
     */
    ~AlsaWidget();

    // AbstractProducerWidget 接口实现

    /**
     * @brief 创建新的ALSA音频生产者
     * @param profile MLT配置信息，包含音频视频参数
     * @return 新创建的ALSA生产者对象指针
     * 
     * 根据当前界面配置创建ALSA音频输入生产者对象。
     * 生产者使用格式："alsa:设备名称?channels=通道数"
     * 
     * @note 调用者负责管理返回的生产者对象内存
     */
    Mlt::Producer *newProducer(Mlt::Profile &profile);
    
    /**
     * @brief 获取当前配置的预设属性
     * @return 包含当前配置的MLT属性对象
     * 
     * 将界面当前的设备名称和通道数设置封装为MLT属性对象，
     * 用于预设的保存和后续加载。
     */
    Mlt::Properties getPreset() const;
    
    /**
     * @brief 加载预设配置到界面
     * @param preset 包含预设配置的MLT属性对象
     * 
     * 从MLT属性对象中解析设备名称和通道数设置，
     * 并更新对应的界面控件。
     */
    void loadPreset(Mlt::Properties &preset);
    
    /**
     * @brief 设置生产者并更新界面状态
     * @param producer MLT生产者对象指针
     * 
     * 当关联的生产者对象发生变化时调用，更新界面显示状态
     * 并加载生产者的配置参数。
     */
    void setProducer(Mlt::Producer *producer);

private slots:
    /**
     * @brief 预设选择槽函数
     * @param p 预设属性的void指针，需要转换为Mlt::Properties*
     * 
     * 当用户从预设列表中选择预设时触发，加载选中的预设配置。
     */
    void on_preset_selected(void *p);
    
    /**
     * @brief 保存预设槽函数
     * 
     * 当用户点击保存预设按钮时触发，将当前配置保存为新预设。
     */
    void on_preset_saveClicked();
    
    /**
     * @brief 应用配置槽函数
     * 
     * 当用户点击应用按钮时触发，创建新的生产者并应用到MLT控制器。
     */
    void on_applyButton_clicked();

private:
    /// @brief 指向UI界面对象的指针，管理所有界面控件
    Ui::AlsaWidget *ui;
};

#endif // ALSAWIDGET_H
