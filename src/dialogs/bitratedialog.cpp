/*
 * Copyright (c) 2023-2024 Meltytech, LLC
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
#include "bitratedialog.h"

// 引入依赖头文件（保存图片对话框、配置类等）
#include "dialogs/saveimagedialog.h"  // 保存图片的对话框类
#include "settings.h"                 // 配置类（读取主题等设置）

// 引入Qt基础组件和图表相关头文件
#include <QDialogButtonBox>
#include <QJsonObject>
#include <QPushButton>
#include <QQueue>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSet>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QValueAxis>

// 【静态常量】滑动窗口大小（用于计算平滑平均值，窗口内包含30个数据点）
static const auto kSlidingWindowSize = 30;

//【构造函数：初始化比特率查看对话框】
// 参数说明：
// - resource：资源名称（如视频/音频文件路径，用于显示在标题）
// - fps：帧率（>0表示视频，=0表示音频）
// - data：比特率数据数组（JSON格式，包含每帧/每段的时间、时长、大小等信息）
// - parent：父窗口指针
BitrateDialog::BitrateDialog(const QString &resource,
                             double fps,
                             const QJsonArray &data,
                             QWidget *parent)
    : QDialog(parent)
{
    // 1. 对话框基础设置
    setMinimumSize(400, 200);       // 设置最小尺寸
    setModal(true);                 // 模态对话框（阻塞父窗口操作）
    setWindowTitle(tr("Bitrate Viewer"));  // 设置窗口标题（比特率查看器）
    setSizeGripEnabled(true);       // 启用右下角大小调整手柄

    // 2. 初始化统计变量（用于计算比特率相关指标）
    double time = 0.0;              // 当前累计时间（秒）
    double maxSize = 0.0;           // 最大数据大小（未实际使用）
    double firstTime = 0.0;         // 第一个数据点的时间（用于时间偏移校准）
    double keySubtotal = 0.0;       // 关键帧（I帧）累计比特率（Kb）
    double interSubtotal = 0.0;     // 非关键帧（P/B帧）累计比特率（Kb）
    double totalKbps = 0.0;         // 总比特率（Kb）
    double minKbps = std::numeric_limits<double>().max();  // 最小每秒比特率（初始为最大值）
    double maxKbps = 0.0;           // 最大每秒比特率（初始为0）
    int periodCount = 0;            // 周期计数（每个周期为1秒，用于图表X轴）
    double previousSecond = 0.0;    // 上一个周期的起始秒数（用于划分1秒周期）

    // 3. 初始化图表组件
    QQueue<double> window;                  // 滑动窗口队列（存储最近30个周期的比特率）
    auto barSeries = new QStackedBarSeries; // 堆叠柱状图系列（显示I帧和P/B帧比特率）
    auto averageLine = new QSplineSeries;   // 平滑曲线系列（显示滑动平均比特率）
    QBarSet *interSet = nullptr;            // 非关键帧（P/B帧）数据组（视频专用）
    // 关键帧（I帧）数据组：视频显示"I"，音频显示"Audio"
    auto keySet = new QBarSet(fps > 0.0 ? "I" : tr("Audio"));

    // 4. 配置柱状图属性
    barSeries->setBarWidth(1.0);    // 设置柱宽为1.0（占满整个周期）
    if (fps > 0.0) {                // 如果是视频（有帧率），添加P/B帧数据组
        interSet = new QBarSet("P/B");
        barSeries->append(interSet);
    }
    barSeries->append(keySet);      // 添加I帧/音频数据组到柱状图
    averageLine->setName(tr("Average"));  // 设置平均曲线名称

    // 5. 解析比特率数据（遍历JSON数组，计算每个周期的比特率）
    for (int i = 0; i < data.size(); ++i) {
        auto o = data[i].toObject();       // 取当前数据点（JSON对象）
        auto pts = o["pts_time"].toString().toDouble();  // 当前数据点的时间戳（秒）
        auto duration = o["duration_time"].toString().toDouble();  // 数据点时长（秒）
        // 计算当前数据点的比特率（size为字节，转换为Kb：字节×8/1000）
        auto size = o["size"].toString().toDouble() * 8.0 / 1000.0;

        // 5.1 校准时间（以第一个数据点为起点，计算相对时间）
        if (pts > 0.0) {
            time = pts + qMax(0.0, duration);  // 累计时间 = 时间戳 + 时长（确保非负）
        }
        if (i == 0) {
            firstTime = time;  // 记录第一个数据点的时间（作为基准）
        }
        time -= firstTime;     // 转换为相对时间（从0开始）

        // 5.2 累加关键帧/非关键帧的比特率
        if (o["flags"].toString()[0] == 'K') {  // 标记为"K"表示关键帧（I帧）
            keySubtotal += size;
        } else {                                // 非关键帧（P/B帧）
            interSubtotal += size;
        }
        totalKbps += size;  // 累加总比特率

        // 5.3 按1秒为周期，更新图表数据（每满1秒或到最后一个数据点时执行）
        if (time >= (previousSecond + 1.0) || (i + 1) == data.size()) {
            // 计算当前周期的总比特率（关键帧+非关键帧）
            auto kbps = interSubtotal + keySubtotal;
            // 更新最小/最大比特率
            if (kbps < minKbps) minKbps = kbps;
            if (kbps > maxKbps) maxKbps = kbps;

            // 为每个周期添加柱状图数据（处理跨多秒的情况，如一个数据点占2秒则添加2个柱）
            int n = qMax(1, int(time - previousSecond));  // 周期数（至少1个）
            for (int j = 0; j < n; ++j) {
                if (interSet) interSet->append(interSubtotal);  // 添加P/B帧数据（视频）
                keySet->append(keySubtotal);                    // 添加I帧/音频数据
                ++periodCount;  // 递增周期计数
            }

            // 5.4 计算滑动平均比特率（用最近30个周期的数据）
            while (window.size() >= kSlidingWindowSize) {
                window.dequeue();  // 窗口满时，移除最早的数据点
            }
            window.enqueue(kbps);  // 添加当前周期的比特率到窗口
            double sum = 0.0;
            for (auto &v : window) sum += v;  // 计算窗口内数据总和
            // X轴坐标偏移0.5：使曲线与柱状图中心对齐（柱状图中心在整数秒位置）
            averageLine->append(time - 0.5, sum / window.size());

            // 5.5 重置周期计数器，更新上一个周期的起始秒数
            interSubtotal = 0.0;
            keySubtotal = 0.0;
            previousSecond = std::floor(time);  // 取当前时间的整数部分（如2.3→2）
        }
    }

    // 6. 配置图表（QChart）
    auto chart = new QChart();
    chart->addSeries(barSeries);    // 添加柱状图系列
    chart->addSeries(averageLine);  // 添加平均曲线系列
    // 根据配置的主题设置图表主题（深色/浅色）
    chart->setTheme(Settings.theme() == "dark" ? QChart::ChartThemeDark : QChart::ChartThemeLight);
    averageLine->setColor(Qt::yellow);  // 设置平均曲线颜色为黄色
    // 设置图表标题：显示资源名称、平均/最小/最大比特率（四舍五入为整数）
    chart->setTitle(tr("Bitrates for %1 ~~ Avg. %2 Min. %3 Max. %4 Kb/s")
                        .arg(resource)
                        .arg(qRound(totalKbps / time))
                        .arg(qRound(minKbps))
                        .arg(qRound(maxKbps)));

    // 7. 配置X轴（时间轴，单位：秒）
    auto axisX = new QValueAxis();
    chart->addAxis(axisX, Qt::AlignBottom);  // X轴在底部
    barSeries->attachAxis(axisX);            // 柱状图关联X轴
    averageLine->attachAxis(axisX);          // 平均曲线关联X轴
    axisX->setRange(0.0, time);              // X轴范围：0到总相对时间
    axisX->setLabelFormat("%.0f s");         // 标签格式：整数秒（如"5 s"）
    axisX->setTickType(QValueAxis::TicksDynamic);  // 动态生成刻度
    // 周期数>100时，刻度间隔为10秒；否则为5秒（避免刻度过于密集）
    axisX->setTickInterval(periodCount > 100 ? 10.0 : 5.0);

    // 8. 配置Y轴（比特率轴，单位：Kb/s）
    QValueAxis *axisY = new QValueAxis();
    chart->addAxis(axisY, Qt::AlignLeft);    // Y轴在左侧
    barSeries->attachAxis(axisY);            // 柱状图关联Y轴
    averageLine->attachAxis(axisY);          // 平均曲线关联Y轴
    axisY->setRange(0.0, maxKbps);           // Y轴范围：0到最大比特率
    axisY->setLabelFormat("%.0f Kb/s");      // 标签格式：整数Kb/s（如"1000 Kb/s"）

    // 9. 配置图表图例
    chart->legend()->setVisible(true);       // 显示图例
    chart->legend()->setAlignment(Qt::AlignBottom);  // 图例在底部

    // 10. 配置图表视图（QChartView）
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);  // 启用抗锯齿（使图表更清晰）

    // 11. 配置对话框布局
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);  // 水平可扩展，垂直固定
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 8, 8);  // 布局边距（上0，右8，下8，左0）
    layout->setSpacing(8);                   // 控件间距8像素
    // 添加滚动区域（当图表过宽时可横向滚动）
    auto scrollArea = new QScrollArea(this);
    scrollArea->setWidget(chartView);        // 滚动区域内容为图表视图
    layout->addWidget(scrollArea);
    // 添加按钮组（保存、关闭）
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)->setDefault(true);  // "关闭"按钮设为默认
    layout->addWidget(buttons);

    // 12. 连接按钮信号与槽函数
    // "保存"按钮：将图表保存为图片（通过SaveImageDialog）
    connect(buttons, &QDialogButtonBox::accepted, this, [=] {
        // 创建与图表视图大小相同的图片（RGB32格式）
        QImage image(chartView->size(), QImage::Format_RGB32);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);  // 抗锯齿绘制
        chartView->render(&painter);  // 将图表视图内容绘制到图片
        painter.end();
        // 打开保存图片对话框，执行保存
        SaveImageDialog(this, tr("Save Bitrate Graph"), image).exec();
    });
    // "关闭"按钮：关闭对话框
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 13. 配置图表视图大小
    chartView->setMinimumWidth(qMax(1010, periodCount * 5));  // 最小宽度：取1010或周期数×5的较大值
    chartView->setMinimumHeight(520);  // 最小高度520像素
    resize(1024, 576);  // 对话框初始大小（1024×576像素）
    show();  // 显示对话框
}
