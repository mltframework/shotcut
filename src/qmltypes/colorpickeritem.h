/*
 * Copyright (c) 2014-2025 Meltytech, LLC
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

#ifndef COLORPICKERITEM_H
#define COLORPICKERITEM_H

#include "widgets/screenselector.h"

#include <QColor>
#include <QObject>

// 【类说明】：屏幕颜色拾取器组件
// 【功能】：提供从屏幕任意位置拾取颜色的功能，支持跨平台实现
// 【特性】：Linux平台使用DBus系统服务，其他平台使用传统截屏方式
// 【算法】：通过屏幕截图和区域平均颜色计算实现精准颜色识别
class ColorPickerItem : public QObject
{
    Q_OBJECT
public:
    // 【构造函数】
    // 【参数】：parent - 父对象指针，用于Qt对象树管理
    explicit ColorPickerItem(QObject *parent = 0);

signals:
    // 【信号】：请求开始颜色拾取过程
    // 【参数】：initialPos - 初始鼠标位置坐标，默认值(-1,-1)表示由用户自由选择
    void pickColor(QPoint initialPos = QPoint(-1, -1));
    
    // 【信号】：颜色拾取完成，返回用户选择的颜色
    // 【参数】：color - 拾取到的颜色值，包含RGB分量
    void colorPicked(const QColor &color);
    
    // 【信号】：用户取消颜色拾取操作
    // 【触发时机】：用户在颜色选择过程中按ESC键或点击取消
    void cancelled();

private slots:
    // 【槽函数】：处理屏幕区域选择完成事件
    // 【参数】：rect - 用户通过鼠标拖拽选择的屏幕区域矩形
    // 【说明】：根据平台配置选择不同的颜色抓取实现方式
    void screenSelected(const QRect &rect);
    
    // 【槽函数】：传统屏幕颜色抓取方法（跨平台通用）
    // 【说明】：通过截取屏幕指定区域并计算像素平均颜色实现
    void grabColor();
    
#ifdef Q_OS_LINUX
    // 【Linux专用槽函数】：通过DBus调用系统颜色拾取服务
    // 【说明】：使用freedesktop门户服务，提供更好的桌面集成体验
    void grabColorDBus();
    
    // 【Linux专用槽函数】：处理DBus颜色拾取服务的异步响应
    // 【参数】：response - 响应状态码（0表示成功，非0表示错误）
    // 【参数】：results - 包含颜色数据的键值对结果
    void gotColorResponse(uint response, const QVariantMap &results);
#endif

private:
    ScreenSelector m_selector;  // 屏幕区域选择器实例，负责用户交互界面
    QRect m_selectedRect;       // 存储用户选择的屏幕区域坐标和尺寸
};

#endif // COLORPICKERITEM_H
