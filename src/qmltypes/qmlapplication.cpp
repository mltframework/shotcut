/*
 * Copyright (c) 2013-2024 Meltytech, LLC
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

#include "qmlapplication.h"

#include "controllers/filtercontroller.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/attachedfiltersmodel.h"
#include "settings.h"
#include "util.h"
#include "videowidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCursor>
#include <QFileInfo>
#include <QMessageBox>
#include <QPalette>
#include <QStyle>
#include <QSysInfo>
#ifdef Q_OS_WIN
#include <QLocale>
#else
#include <clocale>
#endif
#include <limits>

// 【单例模式】：获取QmlApplication的唯一实例
// 【说明】：使用Meyer's单例实现，保证线程安全
QmlApplication &QmlApplication::singleton()
{
    static QmlApplication instance;
    return instance;
}

// 【构造函数】
QmlApplication::QmlApplication()
    : QObject()
{}

// 【功能】：获取对话框模态性设置
// 【说明】：不同平台使用不同的模态类型以提供最佳用户体验
Qt::WindowModality QmlApplication::dialogModality()
{
#ifdef Q_OS_MAC
    return Qt::WindowModal;      // macOS：窗口模态
#else
    return Qt::ApplicationModal; // 其他系统：应用模态
#endif
}

// 【功能】：获取当前鼠标位置
QPoint QmlApplication::mousePos()
{
    return QCursor::pos();
}

// 【功能】：获取工具提示背景色
// 【说明】：考虑GTK+主题的特殊处理
QColor QmlApplication::toolTipBaseColor()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if ("gtk+" == QApplication::style()->objectName())
        return QApplication::palette().highlight().color(); // GTK+主题使用高亮色
#endif
    return QApplication::palette().toolTipBase().color();   // 默认工具提示背景色
}

// 【功能】：获取工具提示文字颜色
QColor QmlApplication::toolTipTextColor()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if ("gtk+" == QApplication::style()->objectName())
        return QApplication::palette().highlightedText().color(); // GTK+主题使用高亮文字色
#endif
    return QApplication::palette().toolTipText().color();        // 默认工具提示文字色
}

// 【功能】：获取当前操作系统名称
QString QmlApplication::OS()
{
#if defined(Q_OS_MAC)
    return "macOS";
#elif defined(Q_OS_LINUX)
    return "Linux";
#elif defined(Q_OS_UNIX)
    return "UNIX";
#elif defined(Q_OS_WIN)
    return "Windows";
#else
    return "";
#endif
}

// 【功能】：获取主窗口的几何位置和尺寸
QRect QmlApplication::mainWinRect()
{
    return MAIN.geometry();
}

// 【功能】：检查剪贴板中是否有滤镜数据
bool QmlApplication::hasFiltersOnClipboard()
{
    return MLT.hasFiltersOnClipboard();
}

// 【功能】：复制已启用的滤镜到剪贴板
// 【说明】：只复制当前选中生产者上启用的滤镜
void QmlApplication::copyEnabledFilters()
{
    QScopedPointer<Mlt::Producer> producer(
        new Mlt::Producer(MAIN.filterController()->attachedModel()->producer()));
    MLT.copyFilters(producer.data(), MLT.FILTER_INDEX_ENABLED); // 复制启用状态的滤镜
    QGuiApplication::clipboard()->setText(MLT.filtersClipboardXML()); // 设置XML到剪贴板
    emit QmlApplication::singleton().filtersCopied(); // 发射复制完成信号
}

// 【功能】：复制所有滤镜到剪贴板（包括禁用状态）
void QmlApplication::copyAllFilters()
{
    QScopedPointer<Mlt::Producer> producer(
        new Mlt::Producer(MAIN.filterController()->attachedModel()->producer()));
    MLT.copyFilters(producer.data(), MLT.FILTER_INDEX_ALL); // 复制所有滤镜
    QGuiApplication::clipboard()->setText(MLT.filtersClipboardXML());
    emit QmlApplication::singleton().filtersCopied();
}

// 【功能】：复制当前选中的单个滤镜到剪贴板
void QmlApplication::copyCurrentFilter()
{
    int currentIndex = MAIN.filterController()->currentIndex();
    if (currentIndex < 0) {
        MAIN.showStatusMessage(tr("Select a filter to copy")); // 提示用户先选择滤镜
        return;
    }
    QScopedPointer<Mlt::Producer> producer(
        new Mlt::Producer(MAIN.filterController()->attachedModel()->producer()));
    MLT.copyFilters(producer.data(), currentIndex); // 复制指定索引的滤镜
    QGuiApplication::clipboard()->setText(MLT.filtersClipboardXML());
    emit QmlApplication::singleton().filtersCopied();
}

// 【功能】：将帧数转换为时间码显示（用于时钟显示）
QString QmlApplication::clockFromFrames(int frames)
{
    if (MLT.producer()) {
        return MLT.producer()->frames_to_time(frames, Settings.timeFormat());
    }
    return QString();
}

// 【功能】：将帧数转换为时间码显示（通用时间显示）
QString QmlApplication::timeFromFrames(int frames)
{
    if (MLT.producer()) {
        return MLT.producer()->frames_to_time(frames, Settings.timeFormat());
    }
    return QString();
}

// 【功能】：获取音频通道数
int QmlApplication::audioChannels()
{
    return MLT.audioChannels();
}

// 【功能】：获取下一个可用的项目文件名（避免重复）
// 【说明】：在项目文件夹中查找不存在的文件名，用于自动保存等场景
QString QmlApplication::getNextProjectFile(const QString &filename)
{
    QDir dir(MLT.projectFolder());
    if (!MLT.projectFolder().isEmpty() && dir.exists()) {
        QFileInfo info(filename);
        QString basename = info.completeBaseName(); // 文件名（不含扩展名）
        QString extension = info.suffix();          // 文件扩展名
        
        if (extension.isEmpty()) {
            extension = basename;
            basename = QString();
        }
        
        // 从1开始递增查找可用的文件名
        for (unsigned i = 1; i < std::numeric_limits<unsigned>::max(); i++) {
            QString filename = QString::fromLatin1("%1%2.%3").arg(basename).arg(i).arg(extension);
            if (!dir.exists(filename)) // 找到不存在的文件名
                return dir.filePath(filename);
        }
    }
    return QString();
}

// 【功能】：检查是否存在有效的项目文件夹
bool QmlApplication::isProjectFolder()
{
    QDir dir(MLT.projectFolder());
    return (!MLT.projectFolder().isEmpty() && dir.exists());
}

// 【功能】：获取设备像素比（支持高DPI显示）
qreal QmlApplication::devicePixelRatio()
{
    return MAIN.devicePixelRatioF();
}

// 【功能】：在状态栏显示消息
// 【参数】：message - 要显示的消息，timeoutSeconds - 显示时长（秒）
void QmlApplication::showStatusMessage(const QString &message, int timeoutSeconds)
{
    MAIN.showStatusMessage(message, timeoutSeconds);
}

// 【功能】：获取最大纹理尺寸（OpenGL相关）
int QmlApplication::maxTextureSize()
{
    auto *videoWidget = qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget());
    return videoWidget ? videoWidget->maxTextureSize() : 0;
}

// 【功能】：确认是否要在输出轨道上添加滤镜
// 【说明】：输出轨道上的滤镜会影响所有剪辑，需要用户确认
bool QmlApplication::confirmOutputFilter()
{
    bool result = true;
    if (MAIN.filterController()->isOutputTrackSelected() && Settings.askOutputFilter()) {
        QMessageBox dialog(QMessageBox::Warning,
                           qApp->applicationName(),
                           tr("<p>Do you really want to add filters to <b>Output</b>?</p>"
                              "<p><b>Timeline > Output</b> is currently selected. "
                              "Adding filters to <b>Output</b> affects ALL clips in the "
                              "timeline including new ones that will be added.</p>"),
                           QMessageBox::No | QMessageBox::Yes,
                           &MAIN);
        dialog.setWindowModality(dialogModality());
        dialog.setDefaultButton(QMessageBox::No);  // 默认选择"否"（安全选项）
        dialog.setEscapeButton(QMessageBox::Yes);  // ESC键对应"是"
        dialog.setCheckBox(
            new QCheckBox(tr("Do not show this anymore.", "confirm output filters dialog")));
        result = dialog.exec() == QMessageBox::Yes;
        if (dialog.checkBox()->isChecked()) {
            Settings.setAskOutputFilter(false); // 用户选择不再显示此提示
        }
    }
    return result;
}

// 【功能】：获取应用程序数据目录路径
// 【说明】：不同平台的数据目录结构不同
QDir QmlApplication::dataDir()
{
    QDir dir(qApp->applicationDirPath());
#if defined(Q_OS_MAC)
    dir.cdUp();
    dir.cd("Resources");        // macOS：Resources目录
#else
#if defined(Q_OS_UNIX) || (defined(Q_OS_WIN) && defined(NODEPLOY))
    dir.cdUp();
#endif
    dir.cd("share");            // Unix/Windows：share目录
#endif
    return dir;
}

// 【功能】：获取与指定颜色对比度较高的文字颜色
// 【说明】：用于确保文字在不同背景色下可读
QColor QmlApplication::contrastingColor(QString color)
{
    return Util::textColor(color);
}

// 【功能】：获取可用的转场特效文件列表
QStringList QmlApplication::wipes()
{
    QStringList result;
    const auto transitions = QString::fromLatin1("transitions");
    QDir dir(Settings.appDataLocation());
    if (!dir.exists(transitions)) {
        dir.mkdir(transitions); // 创建转场目录（如果不存在）
    }
    if (dir.cd(transitions)) {
        for (auto &s : dir.entryList(QDir::Files | QDir::Readable)) {
            result << dir.filePath(s); // 添加所有可读的转场文件
        }
    }
    return result;
}

// 【功能】：添加新的转场特效文件
bool QmlApplication::addWipe(const QString &filePath)
{
    const auto transitions = QString::fromLatin1("transitions");
    QDir dir(Settings.appDataLocation());
    if (!dir.exists(transitions)) {
        dir.mkdir(transitions);
    }
    if (dir.cd(transitions)) {
        return QFile::copy(filePath, dir.filePath(QFileInfo(filePath).fileName()));
    }
    return false;
}

// 【功能】：检查两个矩形是否相交
// 【说明】：QML辅助函数，用于界面布局计算
bool QmlApplication::intersects(const QRectF &a, const QRectF &b)
{
    return a.intersects(b);
}
