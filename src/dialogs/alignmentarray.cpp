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
#include <cstring>
#include <cmath>
#include <iostream>
#include <numeric>

// FFTW plan functions are not threadsafe
static QMutex s_fftwPlanningMutex;

AlignmentArray::AlignmentArray()
    : m_autocorrelationMax(std::numeric_limits<double>::min())
    , m_isTransformed(false)
{
}

AlignmentArray::AlignmentArray(size_t minimum_size)
    : AlignmentArray()
{
    init(minimum_size);
}

AlignmentArray::~AlignmentArray()
{
    QMutexLocker locker(&s_fftwPlanningMutex);
    fftw_free(reinterpret_cast<fftw_complex *>(m_forwardBuf));
    fftw_destroy_plan(m_forwardPlan);
}

void AlignmentArray::init(size_t minimumSize)
{
    m_minimumSize = minimumSize;
    m_actualComplexSize = (minimumSize * 2) - 1;
}

void AlignmentArray::setValues(const std::vector<double> &values)
{
    m_values = values;
}

double AlignmentArray::calculateOffset(AlignmentArray &from, int *offset)
{
    // Create a destination for the correlation values
    s_fftwPlanningMutex.lock();
    fftw_complex *buf = fftw_alloc_complex(m_actualComplexSize);
    std::complex<double> *correlationBuf = reinterpret_cast<std::complex<double>*>(buf);
    fftw_plan correlationPlan = fftw_plan_dft_1d(m_actualComplexSize, buf, buf, FFTW_BACKWARD,
                                                 FFTW_ESTIMATE);
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

    if ( 2 * *offset > (int)m_actualComplexSize ) {
        *offset -= ((int)m_actualComplexSize);
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

double AlignmentArray::calculateOffsetAndDrift(AlignmentArray &from, int precision,
                                               double drift_range, double *drift, int *offset)
{
    static const int FINAL_PRECISION = 9;
    double drift_step = std::pow(10.0, -precision);
    double max_score = *drift;
    double best_drift = 1.0;

    for (double d = *drift - drift_range; d <= *drift + drift_range; d += drift_step) {
        // Copy the "from" sequence with a shift
        AlignmentArray drifted(m_actualComplexSize);
        double factor = 1.0 / d;
        int shift = 0;
        for (size_t i = 0; i < m_minimumSize; ++i) {
            int newShift = std::round(factor * i) - i;
            while (newShift > shift) {
                drifted.m_forwardBuf[i + shift] = from.m_forwardBuf[i];
                ++shift;
            }
            shift = newShift;
            drifted.m_forwardBuf[i + shift] = from.m_forwardBuf[i];
        }
        double score = calculateOffset(drifted, offset);
        if (score > max_score) {
            max_score = score;
            best_drift = d;
        }
    }
    *drift = best_drift;

    if (precision < FINAL_PRECISION) {
        max_score = calculateOffsetAndDrift(from, precision + 1, drift_range / 10, drift, offset);
    }
    return max_score;
}

void AlignmentArray::transform()
{
    QMutexLocker locker(&m_transformMutex);
    if (!m_isTransformed) {
        // Create the plans while the global planning mutex is locked
        s_fftwPlanningMutex.lock();
        fftw_complex *buf = nullptr;
        // Allocate the forward buffer and plan
        buf = fftw_alloc_complex(m_actualComplexSize);
        m_forwardBuf = reinterpret_cast<std::complex<double>*>(buf);
        m_forwardPlan = fftw_plan_dft_1d(m_actualComplexSize, buf, buf, FFTW_FORWARD, FFTW_ESTIMATE);
        std::fill(m_forwardBuf, m_forwardBuf + m_actualComplexSize, std::complex<double>(0));
        // Allocate the backward buffer and plan
        buf = fftw_alloc_complex(m_actualComplexSize);
        std::complex<double> *backwardBuf = reinterpret_cast<std::complex<double>*>(buf);
        fftw_plan backwardPlan = fftw_plan_dft_1d(m_actualComplexSize, buf, buf, FFTW_BACKWARD,
                                                  FFTW_ESTIMATE);
        std::fill(backwardBuf, backwardBuf + m_actualComplexSize, std::complex<double>(0));
        s_fftwPlanningMutex.unlock();

        // Calculate a normalization factor for the initial values.
        // This uses a simplified standard deviation calculation that assumes the mean is 0.
        double accum = 0.0;
        std::for_each (m_values.begin(), m_values.end(), [&](const double d) {
            accum += d * d;
        });
        double factor = sqrt(accum / (m_values.size() - 1));
        // Fill the transform array applying the normalization factor
        for ( size_t i = 0; i < m_values.size(); i++ ) {
            m_forwardBuf[i] = m_values[i] / factor;
        }
        // Perform the forward DFT
        fftw_execute(m_forwardPlan);

        // Perform autocorrelation to calculate the maximum correlation value
        for (size_t i = 0; i < m_actualComplexSize; i++) {
            backwardBuf[i] = m_forwardBuf[i] * std::conj(m_forwardBuf[i]);
        }
        // Convert back to time series
        fftw_execute(backwardPlan);
        // Find the maximum autocorrelation value
        for (size_t i = 0; i < m_actualComplexSize; i++) {
            double norm = std::norm(backwardBuf[i]);
            if (norm > m_autocorrelationMax)
                m_autocorrelationMax = norm;
        }

        s_fftwPlanningMutex.lock();
        fftw_free(backwardBuf);
        fftw_destroy_plan(backwardPlan);
        s_fftwPlanningMutex.unlock();

        m_isTransformed = true;
    }
}
