/*
 * Copyright (c) 2025 Meltytech, LLC
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

#include "qmlextension.h"

#include "Logger.h"
#include "qmltypes/qmlutilities.h"
#include "settings.h"

#include <QDir>
#include <QQmlComponent>

// 【常量定义】：Whisper语音识别扩展的标识符
const QString QmlExtension::WHISPER_ID = QStringLiteral("whispermodel");

// 【QmlExtensionFile构造函数】
// 【功能】：初始化扩展文件对象
// 【参数】：parent - 父对象指针
QmlExtensionFile::QmlExtensionFile(QObject *parent)
    : QObject(parent)
    , m_standard(false)  // 默认非标准文件（用户自定义）
{}

// 【核心功能】：加载QML扩展
// 【参数】：id - 扩展的唯一标识符
// 【返回值】：成功加载的扩展对象指针，失败返回nullptr
// 【说明】：支持从应用数据目录和安装目录加载扩展
QmlExtension *QmlExtension::load(const QString &id)
{
    // 首先尝试从应用数据目录加载（用户自定义扩展）
    QString filePath = appDir(id).absoluteFilePath(extensionFileName(id));
    if (!QFile::exists(filePath)) {
        // 如果不存在，尝试从安装目录加载（系统预置扩展）
        filePath = installDir(id).absoluteFilePath(extensionFileName(id));
    }
    
    // 检查扩展文件是否存在
    if (!QFile::exists(filePath)) {
        LOG_ERROR() << filePath << "does not exist";
        return nullptr;
    }
    
    // 使用QML引擎加载扩展组件
    QQmlComponent component(QmlUtilities::sharedEngine(), filePath);
    QmlExtension *extension = qobject_cast<QmlExtension *>(component.create());
    
    if (!extension) {
        LOG_ERROR() << component.errorString();  // 输出加载错误信息
    }
    return extension;
}

// 【功能】：生成扩展文件名
// 【参数】：id - 扩展标识符
// 【返回值】：完整的扩展文件名（id.qml）
QString QmlExtension::extensionFileName(const QString &id)
{
    return id + ".qml";
}

// 【功能】：获取扩展安装目录
// 【参数】：id - 扩展标识符
// 【返回值】：安装目录的QDir对象
// 【说明】：位于QML目录下的extensions子目录
QDir QmlExtension::installDir(const QString &id)
{
    QDir dir = QmlUtilities::qmlDir();  // 获取QML根目录
    dir.mkdir("extensions");            // 创建extensions目录（如果不存在）
    dir.cd("extensions");               // 进入extensions目录
    return dir;
}

// 【功能】：获取应用数据目录中的扩展目录
// 【参数】：id - 扩展标识符
// 【返回值】：应用数据目录的QDir对象
// 【说明】：为每个扩展创建独立的子目录
QDir QmlExtension::appDir(const QString &id)
{
    QDir dir = Settings.appDataLocation();  // 获取应用数据目录
    dir.mkdir("extensions");                // 创建extensions目录
    dir.cd("extensions");                   // 进入extensions目录
    dir.mkdir(id);                          // 为特定扩展创建目录
    dir.cd(id);                             // 进入扩展目录
    return dir;
}

// 【QmlExtension构造函数】
QmlExtension::QmlExtension(QObject *parent)
    : QObject(parent)
{}

// 【功能】：设置扩展标识符
// 【参数】：id - 新的扩展标识符
// 【说明】：标识符改变时发射changed信号
void QmlExtension::setId(const QString &id)
{
    m_id = id;
    emit changed();  // 通知属性改变
}

// 【功能】：设置扩展名称
// 【参数】：name - 新的扩展名称
void QmlExtension::setName(const QString &name)
{
    m_name = name;
    emit changed();
}

// 【功能】：设置扩展版本
// 【参数】：version - 新的版本号
void QmlExtension::setVersion(const QString &version)
{
    m_version = version;
    emit changed();
}

// 【功能】：获取扩展文件的本地路径
// 【参数】：index - 文件索引
// 【返回值】：文件的完整本地路径
// 【说明】：用于访问扩展相关的资源文件
QString QmlExtension::localPath(int index)
{
    if (index < 0 || index >= fileCount()) {
        LOG_ERROR() << "Invalid Index" << index;  // 索引越界检查
        return QString();
    }
    QDir localPath = appDir(m_id);  // 获取扩展的应用数据目录
    return localPath.absoluteFilePath(m_files[index]->file());  // 返回文件完整路径
}

// 【功能】：检查扩展文件是否已下载
// 【参数】：index - 文件索引
// 【返回值】：文件是否存在本地
bool QmlExtension::downloaded(int index)
{
    return QFile(localPath(index)).exists();  // 检查文件是否存在
}
