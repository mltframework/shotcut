/*
 * Copyright (c) 2022 Meltytech, LLC
 *
 * Author: André Caldas de Souza <andrecaldas@unb.br>
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

#include "alignmentarray.h"  // 包含当前类的头文件

// 引入必要的头文件：Qt调试、线程同步、标准算法、数学函数等
#include <QDebug>
#include <QMutexLocker>       // 用于自动加锁/解锁互斥锁，简化线程同步

#include <algorithm>          // 标准算法库（如for_each、fill等）
#include <cmath>              // 数学函数库（如sqrt、abs等）
#include <cstring>            // C字符串处理（此处未直接使用，可能为兼容）
#include <iostream>           // 标准输入输出（调试用）
#include <numeric>            // 数值算法（如累加等）

// FFTW库的计划（plan）函数不是线程安全的，因此用全局互斥锁保护
static QMutex s_fftwPlanningMutex;

// ------------------------------ 构造函数与析构函数 ------------------------------
// 默认构造函数：初始化成员变量
AlignmentArray::AlignmentArray()
    : m_forwardBuf(nullptr)          // FFT正向变换缓冲区（复数数组）
    , m_backwardBuf(nullptr)         // FFT反向变换缓冲区（复数数组）
    , m_autocorrelationMax(std::numeric_limits<double>::min())  // 自相关最大值（初始化为最小值）
    , m_isTransformed(false)         // 标记是否已完成FFT变换（初始未变换）
{}

// 带参构造函数：调用默认构造函数后，初始化数组大小
AlignmentArray::AlignmentArray(size_t minimum_size)
    : AlignmentArray()  // 委托构造：先调用默认构造函数
{
    init(minimum_size);  // 初始化数组大小
}

// 析构函数：释放FFT相关资源（缓冲区和计划）
AlignmentArray::~AlignmentArray()
{
    QMutexLocker locker(&s_fftwPlanningMutex);  // 加锁保护FFT资源释放（线程安全）
    if (m_forwardBuf) {  // 如果正向缓冲区存在
        // 释放正向变换缓冲区，销毁正向计划
        fftw_free(reinterpret_cast<fftw_complex *>(m_forwardBuf));
        fftw_destroy_plan(m_forwardPlan);
        // 释放反向变换缓冲区，销毁反向计划
        fftw_free(reinterpret_cast<fftw_complex *>(m_backwardBuf));
        fftw_destroy_plan(m_backwardPlan);
    }
}

// ------------------------------ 核心初始化与数据设置 ------------------------------
// 初始化函数：设置数组最小大小，分配FFT所需资源
void AlignmentArray::init(size_t minimumSize)
{
    QMutexLocker locker(&m_transformMutex);  // 加锁保护变换相关资源（线程安全）
    m_minimumSize = minimumSize;  // 保存最小大小
    // 计算实际复数数组大小：用于FFT的数组长度（满足线性卷积的最小长度）
    m_actualComplexSize = (minimumSize * 2) - 1;

    // 如果已有缓冲区，先释放旧资源
    if (m_forwardBuf) {
        QMutexLocker locker(&s_fftwPlanningMutex);  // 加锁保护FFT计划操作
        fftw_free(reinterpret_cast<fftw_complex *>(m_forwardBuf));
        m_forwardBuf = nullptr;  // 置空指针，避免野指针
        fftw_destroy_plan(m_forwardPlan);  // 销毁正向计划
        // 释放反向缓冲区和计划
        fftw_free(reinterpret_cast<fftw_complex *>(m_backwardBuf));
        m_backwardBuf = nullptr;
        fftw_destroy_plan(m_backwardPlan);
    }
}

// 设置音频特征数据：保存原始数据并标记未变换状态
void AlignmentArray::setValues(const std::vector<double> &values)
{
    QMutexLocker locker(&m_transformMutex);  // 加锁保护数据修改
    m_values = values;  // 保存原始音频特征值（如每帧音量平均值）
    m_isTransformed = false;  // 数据更新后，之前的FFT变换失效，需重新计算
}

// ------------------------------ 音频对齐核心算法 ------------------------------
// 计算两个音频序列的时间偏移量（不考虑速度变化）
// 参数：
// - from：待对齐的音频序列
// - offset：输出参数，存储计算出的偏移量（帧单位）
// 返回值：对齐质量（0-1，值越高对齐越准确）
double AlignmentArray::calculateOffset(AlignmentArray &from, int *offset)
{
    // 1. 分配FFT相关资源（用于计算互相关）
    s_fftwPlanningMutex.lock();  // 加锁保护FFT计划创建
    // 分配复数缓冲区（用于存储互相关结果）
    fftw_complex *buf = fftw_alloc_complex(m_actualComplexSize);
    std::complex<double> *correlationBuf = reinterpret_cast<std::complex<double> *>(buf);
    // 创建反向FFT计划（用于将频域互相关结果转换回时域）
    fftw_plan correlationPlan
        = fftw_plan_dft_1d(m_actualComplexSize, buf, buf, FFTW_BACKWARD, FFTW_ESTIMATE);
    // 初始化缓冲区（填充0）
    std::fill(correlationBuf, correlationBuf + m_actualComplexSize, std::complex<double>(0));
    s_fftwPlanningMutex.unlock();  // 解锁

    // 2. 确保当前序列和待对齐序列都已完成FFT变换（转换到频域）
    transform();       // 当前序列（参考轨道）执行FFT
    from.transform();  // 待对齐序列执行FFT

    // 3. 在频域计算互相关：当前序列的频域数据 × 待对齐序列的频域共轭
    for (size_t i = 0; i < m_actualComplexSize; ++i) {
        correlationBuf[i] = m_forwardBuf[i] * std::conj(from.m_forwardBuf[i]);
    }

    // 4. 执行反向FFT，将频域互相关结果转换回时域（得到互相关序列）
    fftw_execute(correlationPlan);

    // 5. 寻找互相关最大值对应的偏移量（即最佳对齐位置）
    double max = 0;  // 存储最大互相关值
    for (size_t i = 0; i < m_actualComplexSize; ++i) {
        double norm = std::norm(correlationBuf[i]);  // 计算复数的模（互相关强度）
        if (max < norm) {  // 找到更大值时更新
            *offset = i;   // 记录当前索引（临时偏移量）
            max = norm;    // 更新最大值
        }
    }

    // 6. 调整偏移量符号（将索引转换为实际时间偏移，支持正负方向）
    if (2 * *offset > (int) m_actualComplexSize) {
        *offset -= ((int) m_actualComplexSize);  // 当索引超过一半长度时，转换为负偏移
    }

    // 7. 释放临时资源
    s_fftwPlanningMutex.lock();
    fftw_free(correlationBuf);       // 释放互相关缓冲区
    fftw_destroy_plan(correlationPlan);  // 销毁互相关计划
    s_fftwPlanningMutex.unlock();

    // 8. 归一化对齐质量（皮尔逊相关系数）
    // 公式：max(互相关) / (sqrt(参考序列自相关最大值) × sqrt(待对齐序列自相关最大值))
    double correlationCoefficient = sqrt(m_autocorrelationMax) * sqrt(from.m_autocorrelationMax);
    return max / correlationCoefficient;  // 返回归一化后的对齐质量（0-1）
}

// 计算两个音频序列的时间偏移量和速度补偿（考虑速度变化）
// 参数：
// - from：待对齐的音频序列
// - speed：输出参数，存储计算出的速度补偿值
// - offset：输出参数，存储计算出的偏移量（帧单位）
// - speedRange：允许的速度调整范围（如0.01表示±1%）
// 返回值：对齐质量（0-1，值越高对齐越准确）
double AlignmentArray::calculateOffsetAndSpeed(AlignmentArray &from,
                                               double *speed,
                                               int *offset,
                                               double speedRange)
{
    // 1. 初始化速度搜索参数
    // 最小速度步长：基于待对齐序列长度，确保至少能区分1帧的差异
    double minimumSpeedStep = 1.0 / (double) from.m_values.size();
    // 初始搜索步长（从较粗的步长开始，逐步精细化）
    double speedStep = 0.0005;
    // 最佳速度初始值（默认无补偿）
    double bestSpeed = 1.0;
    // 最佳偏移量和对齐质量（先通过无速度补偿计算初始值）
    int bestOffset = 0;
    double bestScore = calculateOffset(from, &bestOffset);

    // 2. 创建用于拉伸音频的临时数组
    AlignmentArray stretched(m_minimumSize);
    // 速度搜索范围（基于初始值±speedRange）
    double speedMin = bestSpeed - speedRange;
    double speedMax = bestSpeed + speedRange;

    // 3. 多轮搜索：逐步减小步长，精细化速度补偿值
    while (speedStep > (minimumSpeedStep / 10)) {  // 步长足够小时停止（比最小步长小10倍）
        // 在当前速度范围内搜索（以当前步长遍历）
        for (double s = speedMin; s <= speedMax; s += speedStep) {
            if (s == bestSpeed) {
                continue;  // 跳过已计算过的最佳速度
            }

            // 4. 模拟速度变化：拉伸/压缩待对齐序列的音频特征
            double factor = 1.0 / s;  // 拉伸因子（速度越小，拉伸越长）
            // 计算拉伸后的序列长度
            size_t stretchedSize = std::floor((double) from.m_values.size() * factor);
            std::vector<double> strechedValues(stretchedSize);  // 存储拉伸后的特征值

            // 5. 最近邻插值：快速实现序列拉伸（简单但高效）
            for (size_t i = 0; i < stretchedSize; i++) {
                // 根据速度计算原序列中的对应索引（四舍五入取最近点）
                size_t srcIndex = std::round(s * i);
                strechedValues[i] = from.m_values[srcIndex];  // 复制特征值
            }

            // 6. 计算拉伸后序列与参考序列的偏移量和对齐质量
            stretched.setValues(strechedValues);  // 设置拉伸后的特征值
            double score = calculateOffset(stretched, offset);  // 计算对齐质量

            // 7. 更新最佳参数（如果当前质量更高）
            if (score > bestScore) {
                bestScore = score;    // 更新最佳质量
                bestSpeed = s;        // 更新最佳速度
                bestOffset = *offset; // 更新最佳偏移量
            }
        }

        // 8. 缩小搜索步长（精细化搜索），并缩小搜索范围（聚焦最佳速度附近）
        speedStep /= 10;
        speedMin = bestSpeed - (speedStep * 5);  // 新范围：最佳速度±5个步长
        speedMax = bestSpeed + (speedStep * 5);
    }

    // 9. 输出最佳参数并返回对齐质量
    *speed = bestSpeed;    // 速度补偿值
    *offset = bestOffset;  // 时间偏移量
    return bestScore;      // 对齐质量
}

// 执行FFT变换：将时域音频特征转换为频域，并计算自相关最大值
void AlignmentArray::transform()
{
    QMutexLocker locker(&m_transformMutex);  // 加锁保护变换过程（线程安全）
    if (!m_isTransformed) {  // 如果尚未变换或数据已更新，执行变换
        // 1. 初始化FFT资源（缓冲区和计划）
        if (!m_forwardBuf) {
            s_fftwPlanningMutex.lock();  // 加锁保护FFT计划创建（线程安全）
            fftw_complex *buf = nullptr;

            // 分配正向变换缓冲区并创建计划（时域→频域）
            buf = fftw_alloc_complex(m_actualComplexSize);
            m_forwardBuf = reinterpret_cast<std::complex<double> *>(buf);  // 转换为C++复数指针
            m_forwardPlan = fftw_plan_dft_1d(
                m_actualComplexSize, buf, buf, FFTW_FORWARD, FFTW_ESTIMATE);  // 正向计划

            // 分配反向变换缓冲区并创建计划（频域→时域）
            buf = fftw_alloc_complex(m_actualComplexSize);
            m_backwardBuf = reinterpret_cast<std::complex<double> *>(buf);
            m_backwardPlan = fftw_plan_dft_1d(
                m_actualComplexSize, buf, buf, FFTW_BACKWARD, FFTW_ESTIMATE);  // 反向计划

            s_fftwPlanningMutex.unlock();  // 解锁
        }

        // 2. 初始化缓冲区（填充0）
        std::fill(m_forwardBuf, m_forwardBuf + m_actualComplexSize, std::complex<double>(0));
        std::fill(m_backwardBuf, m_backwardBuf + m_actualComplexSize, std::complex<double>(0));

        // 3. 归一化原始音频特征值（减去均值，除以标准差，消除音量差异影响）
        // 3.1 计算均值
        double accum = 0.0;
        std::for_each(m_values.begin(), m_values.end(), [&](const double d) { accum += d; });
        double mean = accum / m_values.size();

        // 3.2 计算标准差
        accum = 0;
        std::for_each(m_values.begin(), m_values.end(), [&](const double d) {
            accum += (d - mean) * (d - mean);  // 累加平方差
        });
        double stddev = sqrt(accum / (m_values.size() - 1));  // 标准差（样本标准差）

        // 3.3 填充归一化后的数据到正向缓冲区（仅填充原始数据长度，剩余部分保持0）
        for (size_t i = 0; i < m_values.size(); i++) {
            m_forwardBuf[i] = (m_values[i] - mean) / stddev;  // 归一化公式
        }

        // 4. 执行正向FFT：将时域数据转换为频域
        fftw_execute(m_forwardPlan);

        // 5. 计算自相关（频域中为功率谱：频域数据×自身共轭）
        for (size_t i = 0; i < m_actualComplexSize; i++) {
            m_backwardBuf[i] = m_forwardBuf[i] * std::conj(m_forwardBuf[i]);
        }

        // 6. 执行反向FFT：将频域自相关结果转换回时域
        fftw_execute(m_backwardPlan);

        // 7. 寻找自相关最大值（用于后续对齐质量归一化）
        for (size_t i = 0; i < m_actualComplexSize; i++) {
            double norm = std::norm(m_backwardBuf[i]);  // 计算复数模（自相关强度）
            if (norm > m_autocorrelationMax) {
                m_autocorrelationMax = norm;  // 更新最大值
            }
        }

        // 8. 标记变换完成
        m_isTransformed = true;
    }
}
