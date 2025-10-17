/*
 * Copyright (c) 2013-2018 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 * Author: Brian Matherly <code@brianmatherly.com>
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

#ifndef COLORWHEELITEM_H
#define COLORWHEELITEM_H

#include <QImage>
#include <QQuickPaintedItem>

// 【类说明】：颜色轮自定义QML组件
// 【功能】：提供交互式颜色选择界面，包含色相环和亮度滑动条
// 【特性】：支持鼠标拖拽、滚轮微调、HSV颜色空间转换
// 【继承】：QQuickPaintedItem - 支持自定义绘制的QML项目
class ColorWheelItem : public QQuickPaintedItem
{
    Q_OBJECT
    // 【QML属性】：当前选择的颜色，可读写，变化时发射信号
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    // 【QML属性】：红色分量（整数0-255）
    Q_PROPERTY(int red READ red WRITE setRed)
    // 【QML属性】：绿色分量（整数0-255）
    Q_PROPERTY(int green READ green WRITE setGreen)
    // 【QML属性】：蓝色分量（整数0-255）
    Q_PROPERTY(int blue READ blue WRITE setBlue)
    // 【QML属性】：红色分量（浮点数0.0-1.0）
    Q_PROPERTY(qreal redF READ redF WRITE setRedF)
    // 【QML属性】：绿色分量（浮点数0.0-1.0）
    Q_PROPERTY(qreal greenF READ greenF WRITE setGreenF)
    // 【QML属性】：蓝色分量（浮点数0.0-1.0）
    Q_PROPERTY(qreal blueF READ blueF WRITE setBlueF)
    // 【QML属性】：颜色变化步长，用于滚轮微调
    Q_PROPERTY(qreal step READ step WRITE setStep)
    
public:
    // 【构造函数】
    explicit ColorWheelItem(QQuickItem *parent = 0);
    
    // 颜色属性访问方法
    QColor color();
    void setColor(const QColor &color);
    
    // RGB分量访问方法（整数格式）
    int red();
    void setRed(int red);
    int green();
    void setGreen(int green);
    int blue();
    void setBlue(int blue);
    
    // RGB分量访问方法（浮点格式）
    qreal redF();
    void setRedF(qreal red);
    qreal greenF();
    void setGreenF(qreal green);
    qreal blueF();
    void setBlueF(qreal blue);
    
    // 步长属性访问方法
    qreal step();
    void setStep(qreal blue);

signals:
    // 【信号】：颜色改变时发射，携带新的颜色值
    void colorChanged(const QColor &color);

protected:
    // 【事件处理】：鼠标按下事件（开始颜色选择）
    void mousePressEvent(QMouseEvent *event);
    // 【事件处理】：鼠标移动事件（拖拽选择颜色）
    void mouseMoveEvent(QMouseEvent *event);
    // 【事件处理】：鼠标释放事件（结束颜色选择）
    void mouseReleaseEvent(QMouseEvent *event);
    // 【事件处理】：悬停移动事件（更新光标形状）
    void hoverMoveEvent(QHoverEvent *event);
    // 【事件处理】：鼠标滚轮事件（微调颜色分量）
    void wheelEvent(QWheelEvent *event);
    // 【渲染方法】：执行自定义绘制
    void paint(QPainter *painter);

private:
    QImage m_image;             // 颜色轮缓存图像，提高渲染性能
    bool m_isMouseDown;         // 鼠标按下状态标志
    QPoint m_lastPoint;         // 最后鼠标位置坐标
    QSize m_size;               // 组件当前尺寸
    int m_margin;               // 边距大小
    QRegion m_wheelRegion;      // 颜色轮可点击区域
    QRegion m_sliderRegion;     // 滑动条可点击区域
    QColor m_color;             // 当前选择的颜色
    bool m_isInWheel;           // 鼠标是否在颜色轮内
    bool m_isInSquare;          // 鼠标是否在滑动条内
    qreal m_step;               // 颜色变化步长

    // 【私有方法】：计算颜色轮尺寸
    int wheelSize() const;
    // 【核心算法】：将屏幕坐标转换为颜色值
    QColor colorForPoint(const QPoint &point);
    // 【渲染方法】：绘制颜色轮（色相环）
    void drawWheel();
    // 【渲染方法】：在颜色轮上绘制当前颜色指示点
    void drawWheelDot(QPainter &painter);
    // 【渲染方法】：在滑动条上绘制当前亮度指示条
    void drawSliderBar(QPainter &painter);
    // 【渲染方法】：绘制亮度滑动条
    void drawSlider();
    // 【辅助方法】：更新鼠标光标形状
    void updateCursor(const QPoint &pos);
};

#endif // COLORWHEELITEM_H
