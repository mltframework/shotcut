/*
 * Copyright (c) 2020 Meltytech, LLC
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
#include "longuitask.h"

// 引入主窗口头文件（用于设置进度对话框的父窗口）
#include "mainwindow.h"

// 静态全局变量：互斥锁（保护全局实例的线程安全）
static QMutex g_mutex;
// 静态全局变量：LongUiTask全局实例指针（用于外部调用取消操作）
static LongUiTask *g_instance = nullptr;


// 【构造函数】：初始化长时间UI任务进度对话框
// 参数：title - 对话框标题（显示任务名称，如“Processing...”）
LongUiTask::LongUiTask(QString title)
    // 继承QProgressDialog，初始化：标题、无取消按钮文本、进度范围（0-0表示不确定进度）、父窗口为主窗口MAIN
    : QProgressDialog(title, QString(), 0, 0, &MAIN)
{
    setWindowTitle(title);                  // 设置窗口标题
    setModal(true);                         // 模态对话框（阻塞操作）
    setWindowModality(Qt::ApplicationModal); // 应用级模态（阻塞整个应用）
    setMinimumDuration(2000);               // 延迟2秒显示（避免短任务频繁弹窗）
    setRange(0, 0);                         // 初始进度范围（0-0表示“忙”状态，进度条滚动）
    g_instance = this;                      // 赋值全局实例指针，供外部调用
}

// 【析构函数】：重置全局实例指针
LongUiTask::~LongUiTask()
{
    g_instance = nullptr; // 任务结束后，清空全局实例指针
}


// 【公共方法】：报告任务进度（更新进度条和文本）
// 参数说明：
// - text：当前进度文本（如“Processing file 5/10”）
// - value：当前进度值（如5）
// - max：进度最大值（如10）
void LongUiTask::reportProgress(QString text, int value, int max)
{
    setLabelText(text);             // 更新进度文本
    setRange(0, max - 1);           // 设置进度范围（最大值减1，使value能达到“完成”状态）
    setValue(value);                // 更新当前进度值
    QCoreApplication::processEvents(); // 处理未完成的事件（避免界面卡死）
}

// 【静态公共方法】：取消当前长时间UI任务
void LongUiTask::cancel()
{
    // 若全局实例存在，调用父类QProgressDialog的cancel()方法取消任务
    if (g_instance) {
        g_instance->QProgressDialog::cancel();
    }
