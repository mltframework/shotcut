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

// 头文件重复包含保护宏：未定义FILEDOWNLOADDIALOG_H时，执行后续内容
#ifndef FILEDOWNLOADDIALOG_H
#define FILEDOWNLOADDIALOG_H

// 引入Qt进度对话框头文件（FileDownloadDialog继承自QProgressDialog）
#include <QProgressDialog>
// 引入Qt SSL错误类头文件（用于处理下载中的SSL证书错误）
#include <QSslError>

// 前向声明：避免包含不必要的头文件，减少编译依赖
class QFile;                // 文件类（用于写入下载的文件数据）
class QNetworkReply;        // 网络响应类（接收下载进度、数据和错误信息）

// 文件下载对话框类：用于通过网络下载文件，展示下载进度并处理相关异常
class FileDownloadDialog : public QProgressDialog
{
    Q_OBJECT  // Qt元对象系统宏，支持信号槽等Qt核心特性

public:
    // 【构造函数】：初始化文件下载对话框
    // 参数说明：
    // - title：对话框标题（显示下载任务名称，如“Downloading File”）
    // - parent：父窗口指针（默认值nullptr，用于Qt对象树管理）
    explicit FileDownloadDialog(const QString &title, QWidget *parent = nullptr);

    // 【析构函数】：空实现（无额外资源需手动释放，依赖Qt对象树自动管理）
    ~FileDownloadDialog();

    // 【公共方法】：设置下载源地址
    // 参数：src - 下载文件的网络URL（如“https://example.com/file.zip”）
    void setSrc(const QString &src);

    // 【公共方法】：设置下载目标路径
    // 参数：dst - 下载文件保存到本地的路径（如“C:/files/file.zip”）
    void setDst(const QString &dst);

    // 【公共方法】：开始下载任务
    // 返回值：bool - 下载成功返回true，失败或取消返回false
    bool start();

private slots:
    // 【私有槽函数】：处理下载进度更新
    // 参数：bytesReceived - 已接收字节数；bytesTotal - 总字节数（-1表示未知）
    // 功能：根据已接收和总字节数，更新进度对话框的进度条
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    // 【私有槽函数】：处理网络响应数据
    // 功能：读取网络响应中的下载数据，写入本地临时文件
    void onReadyRead();

    // 【私有槽函数】：处理下载完成事件
    // 功能：下载完成后，关闭进度对话框并返回成功状态
    void onFinished();

    // 【私有槽函数】：处理SSL证书错误
    // 参数：errors - SSL错误列表（如证书无效、过期等）
    // 功能：提示用户是否忽略SSL错误，继续下载
    void sslErrors(const QList<QSslError> &errors);

private:
    // 【私有成员变量】：下载源地址（网络URL）
    // 作用：存储待下载文件的网络地址，供start()方法发起请求时使用
    QString m_src;

    // 【私有成员变量】：下载目标路径（本地文件路径）
    // 作用：存储下载文件的本地保存路径，下载成功后重命名临时文件到该路径
    QString m_dst;

    // 【私有成员变量】：本地文件对象指针
    // 作用：用于写入下载数据，先保存为临时文件，下载成功后再重命名为目标文件
    QFile *m_file;

    // 【私有成员变量】：网络响应对象指针
    // 作用：接收网络请求的响应数据、下载进度、错误信息等
    QNetworkReply *m_reply;

    // 【私有成员变量】：网络响应状态码（未在实现中使用，预留扩展）
    // 作用：可用于存储HTTP响应码（如200表示成功、404表示文件不存在等）
    int m_replyCode;
};

// 结束头文件保护宏
#endif // FILEDOWNLOADDIALOG_H
