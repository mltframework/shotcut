/*
 * Copyright (c) 2013-2022 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 * Author: Brian Matherly <code@brianmatherly.com>
 * Some ideas came from Qt-Plus: https://github.com/liuyanghejerry/Qt-Plus
 * and Steinar Gunderson's Movit demo app.
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

#include "colorwheelitem.h"
#include "mainwindow.h"

#include <QCursor>
#include <QPainter>
#include <qmath.h>

#include <cstdio>

static const qreal WHEEL_SLIDER_RATIO = 10.0; // 颜色轮与滑动条宽度比例

// 【构造函数】：初始化颜色轮组件
// 【功能】：创建颜色轮界面元素，设置事件处理
// 【参数】：parent - 父QQuickItem对象
ColorWheelItem::ColorWheelItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_image()
    , m_isMouseDown(false)
    , m_lastPoint(0, 0)
    , m_size(0, 0)
    , m_margin(5)           // 边距设置
    , m_color(0, 0, 0, 0)   // 初始颜色（黑色透明）
    , m_isInWheel(false)
    , m_isInSquare(false)
    , m_step(1 / 256)       // 颜色变化步长
{
    setAcceptedMouseButtons(Qt::LeftButton); // 只接受左键点击
    setAcceptHoverEvents(true);              // 启用悬停事件
}

// 【功能】：获取当前颜色
QColor ColorWheelItem::color()
{
    return m_color;
}

// 【功能】：设置颜色并触发界面更新
// 【说明】：颜色改变时会发射colorChanged信号通知QML
void ColorWheelItem::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        update(); // 请求重绘
        emit colorChanged(m_color);
    }
}

// 以下是一组RGB分量的getter/setter方法
// 每个分量改变都会更新颜色并触发重绘

int ColorWheelItem::red() { return m_color.red(); }
void ColorWheelItem::setRed(int red)
{
    if (m_color.red() != red) {
        m_color.setRed(red);
        update();
        emit colorChanged(m_color);
    }
}

int ColorWheelItem::green() { return m_color.green(); }
void ColorWheelItem::setGreen(int green)
{
    if (m_color.green() != green) {
        m_color.setGreen(green);
        update();
        emit colorChanged(m_color);
    }
}

int ColorWheelItem::blue() { return m_color.blue(); }
void ColorWheelItem::setBlue(int blue)
{
    if (m_color.blue() != blue) {
        m_color.setBlue(blue);
        update();
        emit colorChanged(m_color);
    }
}

// 以下是一组浮点数格式的RGB分量getter/setter
// 使用0.0-1.0的范围，适合颜色计算

qreal ColorWheelItem::redF() { return m_color.redF(); }
void ColorWheelItem::setRedF(qreal red)
{
    if (m_color.redF() != red) {
        m_color.setRedF(red);
        update();
        emit colorChanged(m_color);
    }
}

qreal ColorWheelItem::greenF() { return m_color.greenF(); }
void ColorWheelItem::setGreenF(qreal green)
{
    if (m_color.greenF() != green) {
        m_color.setGreenF(green);
        update();
        emit colorChanged(m_color);
    }
}

qreal ColorWheelItem::blueF() { return m_color.blueF(); }
void ColorWheelItem::setBlueF(qreal blue)
{
    if (m_color.blueF() != blue) {
        m_color.setBlueF(blue);
        update();
        emit colorChanged(m_color);
    }
}

qreal ColorWheelItem::step() { return m_step; }
void ColorWheelItem::setStep(qreal step) { m_step = step; }

// 【功能】：计算颜色轮的直径尺寸
// 【算法】：根据组件宽高和比例约束计算最大可用尺寸
int ColorWheelItem::wheelSize() const
{
    qreal ws = (qreal) width() / (1.0 + 1.0 / WHEEL_SLIDER_RATIO);
    return qMin(ws, height());
}

// 【核心算法】：将屏幕坐标转换为颜色值
// 【参数】：point - 鼠标位置坐标
// 【返回值】：对应坐标处的颜色值
QColor ColorWheelItem::colorForPoint(const QPoint &point)
{
    if (!m_image.valid(point))
        return QColor();
        
    if (m_isInWheel) {
        // 【颜色轮区域】：计算色相和饱和度
        qreal w = wheelSize() - m_margin * 2;
        qreal xf = qreal(point.x() - m_margin) / w;  // 归一化X坐标
        qreal yf = 1.0 - qreal(point.y() - m_margin) / w; // 归一化Y坐标
        qreal xp = 2.0 * xf - 1.0;  // 转换为[-1,1]范围
        qreal yp = 2.0 * yf - 1.0;
        qreal rad = qMin(hypot(xp, yp), 1.0); // 计算极坐标半径
        qreal theta = qAtan2(yp, xp);         // 计算极坐标角度
        
        theta -= 105.0 / 360.0 * 2.0 * M_PI;  // 调整角度偏移（视觉优化）
        if (theta < 0.0)
            theta += 2.0 * M_PI;              // 规范化角度到[0,2π]
            
        qreal hue = (theta * 180.0 / M_PI) / 360.0; // 转换为HSV色相
        return QColor::fromHsvF(hue, rad, m_color.valueF());
    }
    
    if (m_isInSquare) {
        // 【滑动条区域】：计算亮度值
        qreal value = 1.0 - qreal(point.y() - m_margin) / (wheelSize() - m_margin * 2);
        return QColor::fromHsvF(m_color.hueF(), m_color.saturationF(), value);
    }
    
    return QColor();
}

// 【事件处理】：鼠标按下事件
void ColorWheelItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_lastPoint = event->pos();
        if (m_wheelRegion.contains(m_lastPoint)) {
            // 在颜色轮内点击：选择色相和饱和度
            m_isInWheel = true;
            m_isInSquare = false;
            QColor color = colorForPoint(m_lastPoint);
            setColor(color);
        } else if (m_sliderRegion.contains(m_lastPoint)) {
            // 在滑动条内点击：选择亮度
            m_isInWheel = false;
            m_isInSquare = true;
            QColor color = colorForPoint(m_lastPoint);
            setColor(color);
        }
        m_isMouseDown = true;
    }
}

// 【事件处理】：鼠标移动事件（拖拽选择颜色）
void ColorWheelItem::mouseMoveEvent(QMouseEvent *event)
{
    updateCursor(event->pos()); // 更新鼠标光标

    if (!m_isMouseDown) return;
    
    m_lastPoint = event->pos();
    if (m_wheelRegion.contains(m_lastPoint) && m_isInWheel) {
        QColor color = colorForPoint(m_lastPoint);
        setColor(color);
    } else if (m_sliderRegion.contains(m_lastPoint) && m_isInSquare) {
        QColor color = colorForPoint(m_lastPoint);
        setColor(color);
    }
}

// 【事件处理】：鼠标释放事件
void ColorWheelItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isMouseDown = false;
        m_isInWheel = false;
        m_isInSquare = false;
    }
}

// 【事件处理】：悬停移动事件（更新光标形状）
void ColorWheelItem::hoverMoveEvent(QHoverEvent *event)
{
    updateCursor(event->position().toPoint());
}

// 【事件处理】：鼠标滚轮事件（微调颜色）
void ColorWheelItem::wheelEvent(QWheelEvent *event)
{
    QPoint steps = event->angleDelta() / 8 / 15; // 转换为步数
    qreal delta = (qreal) steps.y() * m_step;    // 计算颜色变化量
    QColor currentColor = color();
    qreal c;

    // 分别调整RGB三个分量
    c = currentColor.redF();
    c += delta;
    if (c < 0) c = 0;     // 限制最小值
    if (c > 1) c = 1;     // 限制最大值
    currentColor.setRedF(c);

    c = currentColor.greenF();
    c += delta;
    if (c < 0) c = 0;
    if (c > 1) c = 1;
    currentColor.setGreenF(c);

    c = currentColor.blueF();
    c += delta;
    if (c < 0) c = 0;
    if (c > 1) c = 1;
    currentColor.setBlueF(c);

    setColor(currentColor);
    event->accept();
}

// 【核心渲染】：绘制整个颜色轮组件
void ColorWheelItem::paint(QPainter *painter)
{
    QSize size(width(), height());

    // 尺寸改变时重新生成缓存图像
    if (m_size != size) {
        m_image = QImage(QSize(width(), height()), QImage::Format_ARGB32_Premultiplied);
        m_image.fill(qRgba(0, 0, 0, 0)); // 透明背景
        drawWheel();    // 绘制颜色轮
        drawSlider();   // 绘制亮度滑动条
        m_size = size;
    }

    painter->setRenderHint(QPainter::Antialiasing); // 抗锯齿
    painter->drawImage(0, 0, m_image);              // 绘制缓存图像
    drawWheelDot(*painter);  // 绘制当前颜色指示点
    drawSliderBar(*painter); // 绘制亮度指示条
}

// 【功能】：绘制颜色轮（色相环）
void ColorWheelItem::drawWheel()
{
    int r = wheelSize();
    QPainter painter(&m_image);
    painter.setRenderHint(QPainter::Antialiasing);
    m_image.fill(0); // 透明背景

    // 锥形渐变：实现色相环（红→黄→绿→青→蓝→紫→红）
    QConicalGradient conicalGradient;
    conicalGradient.setColorAt(0.0, Qt::red);
    conicalGradient.setColorAt(60.0 / 360.0, Qt::yellow);
    conicalGradient.setColorAt(135.0 / 360.0, Qt::green);
    conicalGradient.setColorAt(180.0 / 360.0, Qt::cyan);
    conicalGradient.setColorAt(240.0 / 360.0, Qt::blue);
    conicalGradient.setColorAt(315.0 / 360.0, Qt::magenta);
    conicalGradient.setColorAt(1.0, Qt::red);

    // 径向渐变：实现饱和度变化（中心饱和→边缘不饱和）
    QRadialGradient radialGradient(0.0, 0.0, r / 2);
    radialGradient.setColorAt(0.0, Qt::white);
    radialGradient.setColorAt(1.0, Qt::transparent);

    painter.translate(r / 2, r / 2); // 移动到中心
    painter.rotate(-105);            // 旋转起始角度

    // 绘制色相环
    QBrush hueBrush(conicalGradient);
    painter.setPen(Qt::NoPen);
    painter.setBrush(hueBrush);
    painter.drawEllipse(QPoint(0, 0), r / 2 - m_margin, r / 2 - m_margin);

    // 绘制饱和度渐变叠加
    QBrush saturationBrush(radialGradient);
    painter.setBrush(saturationBrush);
    painter.drawEllipse(QPoint(0, 0), r / 2 - m_margin, r / 2 - m_margin);

    // 定义颜色轮的可点击区域
    m_wheelRegion = QRegion(r / 2, r / 2, r - 2 * m_margin, r - 2 * m_margin, QRegion::Ellipse);
    m_wheelRegion.translate(-(r - 2 * m_margin) / 2, -(r - 2 * m_margin) / 2);
}

// 【功能】：在颜色轮上绘制当前颜色指示点
void ColorWheelItem::drawWheelDot(QPainter &painter)
{
    int r = wheelSize() / 2;
    QPen pen(Qt::white);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.setBrush(Qt::black);
    painter.translate(r, r);
    painter.rotate(360.0 - m_color.hue()); // 旋转到当前色相位置
    painter.rotate(-105);                  // 补偿初始旋转
    // 根据饱和度计算指示点位置
    painter.drawEllipse(QPointF(m_color.saturationF() * r - m_margin, 0.0), 4, 4);
    painter.resetTransform();
}

// 【功能】：在滑动条上绘制当前亮度指示条
void ColorWheelItem::drawSliderBar(QPainter &painter)
{
    qreal value = 1.0 - m_color.valueF(); // 亮度值反转（从上到下：亮→暗）
    int ws = wheelSize() * MAIN.devicePixelRatioF(); // 考虑高DPI缩放
    int w = (qreal) ws / WHEEL_SLIDER_RATIO;
    int h = ws - m_margin * 2;
    QPen pen(Qt::white);
    pen.setWidth(qRound(2 * MAIN.devicePixelRatioF()));
    painter.setPen(pen);
    painter.setBrush(Qt::black);
    painter.translate(ws, m_margin + value * h); // 定位到当前亮度位置
    painter.drawRect(0, 0, w, 4); // 绘制指示条
    painter.resetTransform();
}

// 【功能】：绘制亮度滑动条
void ColorWheelItem::drawSlider()
{
    QPainter painter(&m_image);
    painter.setRenderHint(QPainter::Antialiasing);
    int ws = wheelSize();
    int w = (qreal) ws / WHEEL_SLIDER_RATIO;
    int h = ws - m_margin * 2;
    // 线性渐变：实现亮度变化（上白→下黑）
    QLinearGradient gradient(0, 0, w, h);
    gradient.setColorAt(0.0, Qt::white);
    gradient.setColorAt(1.0, Qt::black);
    QBrush brush(gradient);
    painter.setPen(Qt::NoPen);
    painter.setBrush(brush);
    painter.translate(ws, m_margin);
    painter.drawRect(0, 0, w, h);
    // 定义滑动条的可点击区域
    m_sliderRegion = QRegion(ws, m_margin, w, h);
}

// 【功能】：更新鼠标光标形状
// 【说明】：在可交互区域显示十字光标，其他区域恢复默认
void ColorWheelItem::updateCursor(const QPoint &pos)
{
    if (m_wheelRegion.contains(pos) || m_sliderRegion.contains(pos)) {
        setCursor(QCursor(Qt::CrossCursor)); // 十字光标
    } else {
        unsetCursor(); // 默认光标
    }
}
