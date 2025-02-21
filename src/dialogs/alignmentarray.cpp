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

#include "alignmentarray.h"

#include <QDebug>
#include <QMutexLocker>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <numeric>

// FFTW plan functions are not threadsafe
static QMutex s_fftwPlanningMutex;

AlignmentArray::AlignmentArray()
    : m_forwardBuf(nullptr)
    , m_backwardBuf(nullptr)
    , m_autocorrelationMax(std::numeric_limits<double>::min())
    , m_isTransformed(false)
{}

AlignmentArray::AlignmentArray(size_t minimum_size)
    : AlignmentArray()
{
    init(minimum_size);
}

AlignmentArray::~AlignmentArray()
{
    QMutexLocker locker(&s_fftwPlanningMutex);
    if (m_forwardBuf) {
        fftw_free(reinterpret_cast<fftw_complex *>(m_forwardBuf));
        fftw_destroy_plan(m_forwardPlan);
        fftw_free(reinterpret_cast<fftw_complex *>(m_backwardBuf));
        fftw_destroy_plan(m_backwardPlan);
    }
}

void AlignmentArray::init(size_t minimumSize)
{
    QMutexLocker locker(&m_transformMutex);
    m_minimumSize = minimumSize;
    m_actualComplexSize = (minimumSize * 2) - 1;
    if (m_forwardBuf) {
        QMutexLocker locker(&s_fftwPlanningMutex);
        fftw_free(reinterpret_cast<fftw_complex *>(m_forwardBuf));
        m_forwardBuf = nullptr;
        fftw_destroy_plan(m_forwardPlan);
        fftw_free(reinterpret_cast<fftw_complex *>(m_backwardBuf));
        m_backwardBuf = nullptr;
        fftw_destroy_plan(m_backwardPlan);
    }
}

void AlignmentArray::setValues(const std::vector<double> &values)
{
    QMutexLocker locker(&m_transformMutex);
    m_values = values;
    m_isTransformed = false;
}

double AlignmentArray::calculateOffset(AlignmentArray &from, int *offset)
{
    // Create a destination for the correlation values
    s_fftwPlanningMutex.lock();
    fftw_complex *buf = fftw_alloc_complex(m_actualComplexSize);
    std::complex<double> *correlationBuf = reinterpret_cast<std::complex<double> *>(buf);
    fftw_plan correlationPlan
        = fftw_plan_dft_1d(m_actualComplexSize, buf, buf, FFTW_BACKWARD, FFTW_ESTIMATE);
    std::fill(correlationBuf, correlationBuf + m_actualComplexSize, std::complex<double>(0));
    s_fftwPlanningMutex.unlock();

    // Ensure the two sequences are transformed
    transform();
    from.transform();

    // Calculate the cross-correlation signal
    for (size_t i = 0; i < m_actualComplexSize; ++i) {
        correlationBuf[i] = m_forwardBuf[i] * std::conj(from.m_forwardBuf[i]);
    }
    // Convert to time series
    fftw_execute(correlationPlan);

    // Find the maximum correlation offset
    double max = 0;
    for (size_t i = 0; i < m_actualComplexSize; ++i) {
        double norm = std::norm(correlationBuf[i]);
        if (max < norm) {
            *offset = i;
            max = norm;
        }
    }

    if (2 * *offset > (int) m_actualComplexSize) {
        *offset -= ((int) m_actualComplexSize);
    }

    s_fftwPlanningMutex.lock();
    fftw_free(correlationBuf);
    fftw_destroy_plan(correlationPlan);
    s_fftwPlanningMutex.unlock();

    // Normalize the best score by dividing by the max autocorrelation of the two signals
    // (Pearson's correlation coefficient)
    double correlationCoefficient = sqrt(m_autocorrelationMax) * sqrt(from.m_autocorrelationMax);
    return max / correlationCoefficient;
}

double AlignmentArray::calculateOffsetAndSpeed(AlignmentArray &from,
                                               double *speed,
                                               int *offset,
                                               double speedRange)
{
    // The minimum speed step results in one frame of stretch.
    // Do not try to compensate for more than 1 frame of speed difference.
    double minimumSpeedStep = 1.0 / (double) from.m_values.size();
    double speedStep = 0.0005;
    double bestSpeed = 1.0;
    int bestOffset = 0;
    double bestScore = calculateOffset(from, &bestOffset);
    AlignmentArray stretched(m_minimumSize);
    double speedMin = bestSpeed - speedRange;
    double speedMax = bestSpeed + speedRange;

    while (speedStep > (minimumSpeedStep / 10)) {
        for (double s = speedMin; s <= speedMax; s += speedStep) {
            if (s == bestSpeed) {
                continue;
            }
            // Stretch the original values to simulate a speed compensation
            double factor = 1.0 / s;
            size_t stretchedSize = std::floor((double) from.m_values.size() * factor);
            std::vector<double> strechedValues(stretchedSize);
            // Nearest neighbor interpolation
            for (size_t i = 0; i < stretchedSize; i++) {
                size_t srcIndex = std::round(s * i);
                strechedValues[i] = from.m_values[srcIndex];
            }
            stretched.setValues(strechedValues);
            double score = calculateOffset(stretched, offset);
            if (score > bestScore) {
                bestScore = score;
                bestSpeed = s;
                bestOffset = *offset;
            }
        }
        speedStep /= 10;
        speedMin = bestSpeed - (speedStep * 5);
        speedMax = bestSpeed + (speedStep * 5);
    }
    *speed = bestSpeed;
    *offset = bestOffset;
    return bestScore;
}

void AlignmentArray::transform()
{
    QMutexLocker locker(&m_transformMutex);
    if (!m_isTransformed) {
        if (!m_forwardBuf) {
            // Create the plans while the global planning mutex is locked
            s_fftwPlanningMutex.lock();
            fftw_complex *buf = nullptr;
            // Allocate the forward buffer and plan
            buf = fftw_alloc_complex(m_actualComplexSize);
            m_forwardBuf = reinterpret_cast<std::complex<double> *>(buf);
            m_forwardPlan
                = fftw_plan_dft_1d(m_actualComplexSize, buf, buf, FFTW_FORWARD, FFTW_ESTIMATE);
            // Allocate the backward buffer and plan
            buf = fftw_alloc_complex(m_actualComplexSize);
            m_backwardBuf = reinterpret_cast<std::complex<double> *>(buf);
            m_backwardPlan
                = fftw_plan_dft_1d(m_actualComplexSize, buf, buf, FFTW_BACKWARD, FFTW_ESTIMATE);
            s_fftwPlanningMutex.unlock();
        }
        std::fill(m_forwardBuf, m_forwardBuf + m_actualComplexSize, std::complex<double>(0));
        std::fill(m_backwardBuf, m_backwardBuf + m_actualComplexSize, std::complex<double>(0));
        // Calculate the mean and standard deviation to be used to normalize the values.
        double accum = 0.0;
        std::for_each(m_values.begin(), m_values.end(), [&](const double d) { accum += d; });
        double mean = accum / m_values.size();
        accum = 0;
        std::for_each(m_values.begin(), m_values.end(), [&](const double d) {
            accum += (d - mean) * (d - mean);
        });
        double stddev = sqrt(accum / (m_values.size() - 1));
        // Fill the transform array
        // Normalize the input values: Subtract the mean and divide by the standard deviation.
        for (size_t i = 0; i < m_values.size(); i++) {
            m_forwardBuf[i] = (m_values[i] - mean) / stddev;
        }
        // Perform the forward DFT
        fftw_execute(m_forwardPlan);
        // Perform autocorrelation to calculate the maximum correlation value
        for (size_t i = 0; i < m_actualComplexSize; i++) {
            m_backwardBuf[i] = m_forwardBuf[i] * std::conj(m_forwardBuf[i]);
        }
        // Convert back to time series
        fftw_execute(m_backwardPlan);
        // Find the maximum autocorrelation value
        for (size_t i = 0; i < m_actualComplexSize; i++) {
            double norm = std::norm(m_backwardBuf[i]);
            if (norm > m_autocorrelationMax)
                m_autocorrelationMax = norm;
        }
        m_isTransformed = true;
    }
}
