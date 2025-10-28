/*
 * Copyright (c) 2014-2024 Meltytech, LLC
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

#ifndef QMLAPPLICATION_H
#define QMLAPPLICATION_H

#include <QColor>
#include <QDir>
#include <QObject>
#include <QPoint>
#include <QRect>

namespace Mlt {
class Producer;
}

// 【类说明】：QML应用核心工具类（单例模式）
// 【功能】：为QML界面提供系统级功能接口和工具方法
// 【特性】：跨平台支持、滤镜操作、文件管理、界面工具等
class QmlApplication : public QObject
{
    Q_OBJECT
    // 【QML属性】：对话框模态性设置（常量）
    Q_PROPERTY(Qt::WindowModality dialogModality READ dialogModality CONSTANT);
    // 【QML属性】：当前鼠标位置
    Q_PROPERTY(QPoint mousePos READ mousePos);
    // 【QML属性】：工具提示背景色（调色板改变时通知）
    Q_PROPERTY(QColor toolTipBaseColor READ toolTipBaseColor NOTIFY paletteChanged)
    // 【QML属性】：工具提示文字颜色（调色板改变时通知）
    Q_PROPERTY(QColor toolTipTextColor READ toolTipTextColor NOTIFY paletteChanged)
    // 【QML属性】：操作系统名称（常量）
    Q_PROPERTY(QString OS READ OS CONSTANT)
    // 【QML属性】：主窗口位置和尺寸
    Q_PROPERTY(QRect mainWinRect READ mainWinRect);
    // 【QML属性】：剪贴板中是否有滤镜数据（滤镜复制时通知）
    Q_PROPERTY(bool hasFiltersOnClipboard READ hasFiltersOnClipboard NOTIFY filtersCopied)
    // 【QML属性】：设备像素比（常量，支持高DPI）
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio CONSTANT)
    // 【QML属性】：最大纹理尺寸（常量，OpenGL相关）
    Q_PROPERTY(int maxTextureSize READ maxTextureSize CONSTANT)
    // 【QML属性】：可用转场特效列表（常量）
    Q_PROPERTY(QStringList wipes READ wipes CONSTANT)

public:
    // 【单例模式】：获取唯一实例
    static QmlApplication &singleton();
    
    // 系统级功能方法
    static Qt::WindowModality dialogModality();
    static QPoint mousePos();
    static QColor toolTipBaseColor();
    static QColor toolTipTextColor();
    static QString OS();
    static QRect mainWinRect();
    static bool hasFiltersOnClipboard();
    
    // 【可调用方法】：滤镜复制操作
    Q_INVOKABLE static void copyAllFilters();
    Q_INVOKABLE static void copyEnabledFilters();
    Q_INVOKABLE static void copyCurrentFilter();
    
    // 【可调用方法】：时间码转换
    Q_INVOKABLE static QString clockFromFrames(int frames);
    Q_INVOKABLE static QString timeFromFrames(int frames);
    
    // 【可调用方法】：音频相关
    Q_INVOKABLE static int audioChannels();
    
    // 【可调用方法】：文件管理
    Q_INVOKABLE static QString getNextProjectFile(const QString &filename);
    Q_INVOKABLE static bool isProjectFolder();
    
    // 显示和图形相关
    static qreal devicePixelRatio();
    Q_INVOKABLE void showStatusMessage(const QString &message, int timeoutSeconds = 15);
    static int maxTextureSize();
    
    // 【可调用方法】：用户确认和工具函数
    Q_INVOKABLE static bool confirmOutputFilter();
    static QDir dataDir();
    Q_INVOKABLE static QColor contrastingColor(QString color);
    static QStringList wipes();
    Q_INVOKABLE static bool addWipe(const QString &filePath);
    Q_INVOKABLE static bool intersects(const QRectF &a, const QRectF &b);

signals:
    // 【信号】：应用程序调色板改变时发射
    void paletteChanged();
    // 【信号】：滤镜复制完成时发射
    void filtersCopied();
    // 【信号】：滤镜粘贴完成时发射，携带生产者对象
    void filtersPasted(Mlt::Producer *);

private:
    // 【私有构造函数】：单例模式防止外部实例化
    explicit QmlApplication();
    // 【禁用拷贝构造】：单例模式要求
    QmlApplication(QmlApplication const &);
    // 【禁用赋值操作】：单例模式要求
    void operator=(QmlApplication const &);
};

#endif // QMLAPPLICATION_H
