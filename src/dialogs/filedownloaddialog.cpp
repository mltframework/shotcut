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

// 包含当前类的头文件
#include "filedownloaddialog.h"

// 引入依赖的头文件（日志、主窗口、QML应用等）
#include "Logger.h"                  // 日志工具类，用于打印下载相关日志
#include "mainwindow.h"              // 主窗口类（获取父窗口指针）
#include "qmltypes/qmlapplication.h" // QML应用类（获取对话框模态属性）

// 引入Qt网络和界面相关头文件
#include <QMessageBox>               // 消息框（用于显示下载失败、SSL错误等提示）
#include <QNetworkAccessManager>     // 网络访问管理器（发起网络请求）
#include <QNetworkReply>             // 网络响应对象（接收下载数据、进度等）
#include <QNetworkRequest>           // 网络请求对象（配置下载请求参数）

// 静态常量：下载进度最大值（用于进度条显示，1000对应100%）
static const int PROGRESS_MAX = 1000;


// 【构造函数】：初始化文件下载对话框
// 参数说明：
// - title：对话框标题（显示下载任务名称）
// - parent：父窗口指针（默认用主窗口MAIN作为父窗口）
FileDownloadDialog::FileDownloadDialog(const QString &title, QWidget *parent)
    // 继承QProgressDialog，初始化进度条：标题、取消按钮文本、进度范围（0到PROGRESS_MAX）、父窗口
    : QProgressDialog(title, tr("Cancel"), 0, PROGRESS_MAX, parent ? parent : &MAIN)
{
    setWindowTitle(title);                  // 设置窗口标题
    setModal(true);                         // 模态对话框（阻塞操作）
    setWindowModality(Qt::ApplicationModal); // 应用级模态（阻塞整个应用，不仅父窗口）
    setMinimumDuration(0);                  // 立即显示进度条（无延迟）
}

// 【析构函数】：空实现（无额外资源需手动释放）
FileDownloadDialog::~FileDownloadDialog() {}


// 【公共方法】：设置下载源地址（URL）
// 参数：src - 下载文件的网络地址（如"https://xxx.com/file.zip"）
void FileDownloadDialog::setSrc(const QString &src)
{
    m_src = src; // 保存源地址到成员变量
}

// 【公共方法】：设置下载目标路径（本地文件路径）
// 参数：dst - 下载文件保存到本地的路径（如"C:/downloads/file.zip"）
void FileDownloadDialog::setDst(const QString &dst)
{
    m_dst = dst; // 保存目标路径到成员变量
}


// 【公共方法】：开始下载任务
// 返回值：bool - 下载成功返回true，失败/取消返回false
bool FileDownloadDialog::start()
{
    // 打印下载源和目标路径日志
    LOG_INFO() << "Download Source" << m_src;
    LOG_INFO() << "Download Destination" << m_dst;

    bool retVal = false;                  // 下载结果（默认失败）
    QString tmpPath = m_dst + ".tmp";     // 临时文件路径（避免下载中断导致目标文件损坏）
    m_file = new QFile(tmpPath, this);    // 创建临时文件对象

    // 1. 打开临时文件（只写模式），失败则返回
    if (!m_file || !m_file->open(QIODevice::WriteOnly)) {
        LOG_ERROR() << "Unable to open file to write"; // 打印打开失败日志
        delete m_file; // 释放文件对象
        return retVal; // 返回失败
    }

    // 2. 初始化网络请求
    QNetworkAccessManager manager(this);  // 网络访问管理器（发起请求）
    QUrl url = m_src;                     // 转换源地址为QUrl
    QNetworkRequest request(url);         // 创建网络请求
    request.setTransferTimeout(6000);     // 设置超时时间（6秒）
    m_reply = manager.get(request);       // 发起GET请求，获取网络响应对象

    // 3. 连接网络响应的信号与槽（处理下载进度、数据、完成、SSL错误）
    QObject::connect(m_reply, &QNetworkReply::downloadProgress, this, &FileDownloadDialog::onDownloadProgress);
    QObject::connect(m_reply, &QNetworkReply::readyRead, this, &FileDownloadDialog::onReadyRead);
    QObject::connect(m_reply, &QNetworkReply::finished, this, &FileDownloadDialog::onFinished);
    QObject::connect(m_reply, &QNetworkReply::sslErrors, this, &FileDownloadDialog::sslErrors);

    // 4. 显示进度对话框，等待下载完成（exec()阻塞直到对话框关闭）
    int result = exec();

    // 5. 处理下载结果
    if (result != QDialog::Accepted) {
        // 情况1：用户取消下载（进度框被取消）
        LOG_WARNING() << "Download canceled";
        m_file->remove(); // 删除临时文件
    } else if (m_reply->error() != QNetworkReply::NoError) {
        // 情况2：下载出错（网络错误等）
        LOG_ERROR() << m_reply->errorString(); // 打印错误信息

        // 特殊处理：HTTPS错误时尝试切换为HTTP重试
        if (m_reply->error() == QNetworkReply::UnknownNetworkError && m_src.startsWith("https:")) {
            m_src.replace("https://", "http://"); // 替换为HTTP地址
            return start(); // 重新调用start()重试
        }

        // 非重试错误：删除临时文件，显示失败提示
        if (m_reply->error() != QNetworkReply::NoError) {
            m_file->remove();
            QMessageBox::information(this, windowTitle(), tr("Download Failed"));
        }
    } else {
        // 情况3：下载成功
        m_file->rename(m_dst); // 临时文件重命名为目标路径（完成下载）
        retVal = true; // 标记下载成功
    }

    // 6. 释放资源
    delete m_reply;
    delete m_file;
    return retVal; // 返回下载结果
}


// 【私有槽函数】：处理下载进度更新
// 参数：bytesReceived - 已接收字节数；bytesTotal - 总字节数（-1表示未知）
void FileDownloadDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) { // 只有总字节数已知时，才更新进度条
        // 计算进度（已接收/总字节数 × 进度最大值）
        int progress = bytesReceived * PROGRESS_MAX / bytesTotal;
        LOG_INFO() << "Download Progress" << progress / 10; // 打印进度（转换为百分比）
        setValue(progress); // 更新进度条显示
    }
}

// 【私有槽函数】：处理网络响应数据（接收下载数据并写入文件）
void FileDownloadDialog::onReadyRead()
{
    // 读取所有可用数据，写入临时文件
    m_file->write(m_reply->readAll());
}

// 【私有槽函数】：处理下载完成事件
void FileDownloadDialog::onFinished()
{
    accept(); // 关闭进度对话框，返回Accepted状态
}

// 【私有槽函数】：处理SSL证书错误（如证书无效、过期等）
// 参数：errors - SSL错误列表
void FileDownloadDialog::sslErrors(const QList<QSslError> &errors)
{
    LOG_ERROR() << "SSL Errors" << errors; // 打印SSL错误日志

    // 拼接错误提示信息
    QString message = tr("The following SSL errors were encountered:");
    foreach (const QSslError &error, errors) {
        message += QStringLiteral("\n") + error.errorString(); // 追加每个错误的描述
    }
    message += tr("\nAttempt to ignore SSL errors?"); // 询问是否忽略错误

    // 显示SSL错误询问框（是/否选项）
    QMessageBox qDialog(QMessageBox::Question,
                        windowTitle(),
                        message,
                        QMessageBox::No | QMessageBox::Yes,
                        this);
    qDialog.setDefaultButton(QMessageBox::Yes); // 默认选择"是"
    qDialog.setEscapeButton(QMessageBox::No);   // 按ESC键选择"否"
    qDialog.setWindowModality(QmlApplication::dialogModality()); // 匹配应用模态属性
    int result = qDialog.exec(); // 显示对话框，等待用户选择

    if (result == QMessageBox::Yes) {
        m_reply->ignoreSslErrors(); // 用户选择忽略，继续下载
    }
}
