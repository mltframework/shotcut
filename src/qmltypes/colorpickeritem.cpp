/*
 * Copyright (c) 2014-2025 Meltytech, LLC
 * Inspiration: KDENLIVE colorpickerwidget.cpp by Till Theato (root@ttill.de)
 * Inspiration: QColorDialog.cpp
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

#include "colorpickeritem.h"
#include "Logger.h"

#include <QApplication>
#include <QGuiApplication>
#include <QImage>
#include <QScreen>
#include <QTimer>

#ifdef Q_OS_LINUX
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#endif

// 【构造函数】
// 【功能】：初始化颜色拾取器，设置信号连接和平台特定配置
// 【参数】：parent - 父对象指针，用于Qt对象树管理
ColorPickerItem::ColorPickerItem(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_LINUX
    // 【Linux平台】：注册QColor类型到DBus系统，用于进程间通信
    qDBusRegisterMetaType<QColor>();
#endif

    // 【信号连接】：将颜色拾取信号转发到屏幕选择器
    connect(this, &ColorPickerItem::pickColor, &m_selector, &ScreenSelector::startSelection);
    // 【信号连接】：屏幕区域选择完成后的处理
    connect(&m_selector, &ScreenSelector::screenSelected, this, &ColorPickerItem::screenSelected);
    // 【信号连接】：用户取消选择的通知
    connect(&m_selector, &ScreenSelector::cancelled, this, &ColorPickerItem::cancelled);
}

// 【功能】：处理屏幕区域选择完成事件
// 【参数】：rect - 用户选择的屏幕区域矩形
// 【说明】：根据平台和配置选择不同的颜色抓取方式
void ColorPickerItem::screenSelected(const QRect &rect)
{
    m_selectedRect = rect; // 保存选择的区域
#ifdef Q_OS_LINUX
    // 【Linux平台】：根据配置选择DBus方式或传统方式
    if (m_selector.useDBus())
        QTimer::singleShot(0, this, &ColorPickerItem::grabColorDBus);
    else
#endif
        // 【传统方式】：延迟200毫秒，给屏幕选择器窗口消失的时间
        // 确保抓取的是干净的屏幕内容，不包含选择器界面
        QTimer::singleShot(200, this, &ColorPickerItem::grabColor);
}

// 【功能】：传统屏幕颜色抓取方法
// 【说明】：通过截屏并计算区域平均颜色来实现颜色拾取
void ColorPickerItem::grabColor()
{
    // 获取选择区域所在的屏幕
    QScreen *screen = QGuiApplication::screenAt(m_selectedRect.topLeft());
    // 抓取屏幕指定区域的像素
    QPixmap screenGrab = screen->grabWindow(0,
                                            m_selectedRect.x() - screen->geometry().x(),
                                            m_selectedRect.y() - screen->geometry().y(),
                                            m_selectedRect.width(),
                                            m_selectedRect.height());
    QImage image = screenGrab.toImage(); // 转换为QImage进行像素级操作
    
    // 计算区域内的像素总数
    int numPixel = qMax(image.width() * image.height(), 1);
    int sumR = 0; // 红色分量总和
    int sumG = 0; // 绿色分量总和
    int sumB = 0; // 蓝色分量总和

    // 【核心算法】：遍历所有像素，累加RGB分量
    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            QColor color = image.pixel(x, y);
            sumR += color.red();
            sumG += color.green();
            sumB += color.blue();
        }
    }

    // 计算平均颜色并发射信号
    QColor avgColor(sumR / numPixel, sumG / numPixel, sumB / numPixel);
    emit colorPicked(avgColor);
}

#ifdef Q_OS_LINUX

// 【DBus支持】：QColor类型的DBus序列化操作符
// 【功能】：将QColor对象序列化为DBus可传输的格式
QDBusArgument &operator<<(QDBusArgument &arg, const QColor &color)
{
    arg.beginStructure();
    arg << color.redF() << color.greenF() << color.blueF(); // 使用浮点数格式
    arg.endStructure();
    return arg;
}

// 【DBus支持】：QColor类型的DBus反序列化操作符
// 【功能】：从DBus参数中还原QColor对象
const QDBusArgument &operator>>(const QDBusArgument &arg, QColor &color)
{
    double red, green, blue;
    arg.beginStructure();
    arg >> red >> green >> blue;
    color.setRedF(red);    // 设置红色分量
    color.setGreenF(green); // 设置绿色分量
    color.setBlueF(blue);   // 设置蓝色分量
    arg.endStructure();

    return arg;
}

// 【功能】：通过DBus调用系统颜色拾取服务
// 【说明】：使用Linux桌面环境的门户服务进行颜色拾取
void ColorPickerItem::grabColorDBus()
{
    // 创建DBus方法调用消息
    QDBusMessage message
        = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.portal.Desktop"),
                                         QLatin1String("/org/freedesktop/portal/desktop"),
                                         QLatin1String("org.freedesktop.portal.Screenshot"),
                                         QLatin1String("PickColor"));
    message << QLatin1String("x11:") << QVariantMap{}; // 添加调用参数
    
    // 异步调用DBus方法
    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    
    // 设置异步调用完成后的回调处理
    connect(watcher, &QDBusPendingCallWatcher::finished, [=](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            LOG_WARNING() << "Unable to get DBus reply: " << reply.error().message();
        } else {
            // 连接响应信号，等待颜色选择结果
            QDBusConnection::sessionBus().connect(QString(),
                                                  reply.value().path(),
                                                  QLatin1String("org.freedesktop.portal.Request"),
                                                  QLatin1String("Response"),
                                                  this,
                                                  SLOT(gotColorResponse(uint, QVariantMap)));
        }
    });
}

// 【功能】：处理DBus颜色拾取服务的响应
// 【参数】：response - 响应状态码（0表示成功）
// 【参数】：results - 包含颜色数据的返回结果
void ColorPickerItem::gotColorResponse(uint response, const QVariantMap &results)
{
    if (!response) { // 响应成功
        if (results.contains(QLatin1String("color"))) {
            // 从结果中提取颜色并发射信号
            const QColor color = qdbus_cast<QColor>(results.value(QLatin1String("color")));
            LOG_DEBUG() << "picked" << color;
            emit colorPicked(color);
        }
    } else { // 响应失败
        LOG_WARNING() << "Failed to grab screen" << response << results;
    }
}

#endif
