/*
 * Copyright (c) 2023 Meltytech, LLC
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

// 头文件重复包含保护宏：未定义BITRATEDIALOG_H时，才执行后续内容
#ifndef BITRATEDIALOG_H
#define BITRATEDIALOG_H

// 引入依赖的Qt头文件
#include <QDialog>       // Qt标准对话框类，BitrateDialog继承自此
#include <QJsonArray>    // Qt的JSON数组类，用于接收比特率数据
#include <QString>       // Qt字符串类，用于接收资源名称

// 比特率查看对话框类：用于展示音频/视频文件的比特率变化图表
class BitrateDialog : public QDialog
{
    Q_OBJECT  // Qt元对象系统宏，支持信号槽等Qt特性（此处虽未显式用信号槽，符合类设计规范）

public:
    // 构造函数：初始化比特率查看对话框
    // 参数说明：
    // - resource：待查看的资源名称（如音频/视频文件路径，用于图表标题）
    // - fps：帧率（>0表示视频，=0表示音频，用于区分图表数据类型）
    // - data：比特率原始数据（JSON数组格式，包含每帧/每段的时间、时长、大小等信息）
    // - parent：父窗口指针（默认nullptr，用于Qt对象树管理）
    explicit BitrateDialog(const QString &resource,
                           double fps,
                           const QJsonArray &data,
                           QWidget *parent = nullptr);
};

// 结束头文件保护宏
#endif // BITRATEDIALOG_H
