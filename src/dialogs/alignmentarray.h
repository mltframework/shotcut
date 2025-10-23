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

// 防止头文件重复包含的保护宏：未定义ALIGNMENTARRAY_H时才执行后续内容
#ifndef ALIGNMENTARRAY_H
#define ALIGNMENTARRAY_H

// 引入依赖头文件
#include <QMutex>                // Qt互斥锁，用于多线程安全
#include <complex>               // C++标准复数库，处理FFT复数运算
#include <fftw3.h>               // FFTW库头文件，提供快速傅里叶变换功能
#include <vector>                // C++动态数组，存储音频特征数据

// 音频对齐数据数组类：用于存储音频特征，通过FFT计算对齐参数（偏移量、速度）
class AlignmentArray
{
public:
    // 构造函数与析构函数
    AlignmentArray();                          // 默认构造函数：初始化成员变量
    AlignmentArray(size_t minimum_size);       // 带参构造函数：指定最小数组大小并初始化
    virtual ~AlignmentArray();                 // 虚析构函数：释放FFT相关资源（避免内存泄漏）

    // 公共成员函数（外部调用接口）
    void init(size_t minimum_size);            // 初始化：设置数组大小，释放旧资源
    void setValues(const std::vector<double> &values);  // 设置音频特征数据，标记未变换状态
    // 计算仅时间偏移量（不考虑速度变化），返回对齐质量，offset为输出参数
    double calculateOffset(AlignmentArray &from, int *offset);
    // 计算时间偏移量+速度补偿（考虑速度变化），返回对齐质量，speed/offset为输出参数
    double calculateOffsetAndSpeed(AlignmentArray &from,
                                   double *speed,
                                   int *offset,
                                   double speedRange);

private:
    // 私有成员函数（内部逻辑实现）
    void transform();                          // 执行FFT变换：时域→频域，计算自相关最大值

    // 私有成员变量（数据存储与状态管理）
    std::vector<double> m_values;              // 原始音频特征数据（如每帧音量平均值）
    fftw_plan m_forwardPlan;                   // FFT正向计划（时域→频域）
    std::complex<double> *m_forwardBuf;        // 正向变换缓冲区（存储频域数据）
    fftw_plan m_backwardPlan;                  // FFT反向计划（频域→时域）
    std::complex<double> *m_backwardBuf;       // 反向变换缓冲区（存储自相关结果）
    double m_autocorrelationMax;               // 自相关最大值（用于对齐质量归一化）
    size_t m_minimumSize;                      // 初始化时指定的最小数组大小
    size_t m_actualComplexSize;                // 实际FFT复数数组大小（由minimumSize计算）
    bool m_isTransformed;                      // 变换状态标记（true=已完成FFT，无需重复执行）
    QMutex m_transformMutex;                   // 变换互斥锁（保护多线程下的变换操作）
};

// 结束头文件保护宏
#endif
