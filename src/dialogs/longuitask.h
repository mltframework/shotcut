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

// 头文件重复包含保护宏：未定义LONGUITASK_H时，执行后续内容
#ifndef LONGUITASK_H
#define LONGUITASK_H

// 引入Qt相关头文件
#include <QFuture>                  // Qt异步任务结果类（接收并发任务的返回值）
#include <QProgressDialog>          // Qt进度对话框类（LongUiTask继承自此，用于展示任务进度）
#include <QtConcurrent/QtConcurrent> // Qt并发框架（用于执行异步任务）

// 长时间UI任务类：封装异步任务执行与进度展示，避免UI卡死
class LongUiTask : public QProgressDialog
{
public:
    // 【构造函数】：初始化长时间UI任务进度对话框
    // 参数：title - 对话框标题（显示任务名称，如“Exporting Video”）
    explicit LongUiTask(QString title);

    // 【析构函数】：空实现（无额外资源需手动释放）
    ~LongUiTask();

    // 【模板公共方法】：等待异步任务完成并展示进度
    // 模板参数：Ret - 异步任务的返回值类型
    // 参数说明：
    // - text：进度提示文本（如“Processing...”）
    // - future：异步任务的未来对象（QFuture，用于监测任务状态）
    // 功能：循环等待任务完成，期间保持UI响应，任务结束后返回结果
    template<class Ret>
    Ret wait(QString text, const QFuture<Ret> &future)
    {
        setLabelText(text);             // 设置进度提示文本
        setRange(0, 0);                 // 进度范围设为0-0（表示“忙”状态，进度条滚动）
        while (!future.isFinished()) {  // 循环直到任务完成
            setValue(0);                // 更新进度条（维持滚动状态）
            QCoreApplication::processEvents(); // 处理UI事件，避免界面卡死
            QThread::msleep(100);       // 休眠100毫秒，降低CPU占用
        }
        return future.result();         // 返回异步任务的执行结果
    }

    // 【模板公共方法】：异步执行任务并等待完成
    // 模板参数：
    // - Ret - 任务函数的返回值类型
    // - Func - 任务函数类型
    // - Args - 任务函数的参数类型（可变参数）
    // 参数说明：
    // - text：进度提示文本
    // - f：待执行的任务函数（需在后台线程运行，避免阻塞UI）
    // - args：任务函数的参数（完美转发，保持参数类型）
    // 功能：通过QtConcurrent启动异步任务，调用wait()等待完成并返回结果
    template<class Ret, class Func, class... Args>
    Ret runAsync(QString text, Func &&f, Args &&...args)
    {
        // 启动异步任务：将函数f和参数args转发到后台线程执行
        QFuture<Ret> future = QtConcurrent::run(f, std::forward<Args>(args)...);
        return wait<Ret>(text, future); // 等待任务完成并返回结果
    }

    // 【公共方法】：更新任务进度（显示确定进度）
    // 参数说明：
    // - text：当前进度提示文本（如“Processing 5/10 files”）
    // - value：当前进度值（如5）
    // - max：进度最大值（如10）
    // 功能：设置进度范围和当前值，更新提示文本，保持UI响应
    void reportProgress(QString text, int value, int max);

    // 【静态公共方法】：取消当前长时间UI任务
    // 功能：触发进度对话框的取消操作，终止正在执行的任务
    static void cancel();
};

// 结束头文件保护宏
#endif // LONGUITASK_H
