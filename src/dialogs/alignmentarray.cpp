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

#include <QMutex>
#include <QMutexLocker>

#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cassert>

// FFTW plan functions are not threadsafe
static QMutex s_fftwPlanningMutex;

AlignmentArray::AlignmentArray()
    : m_isTransformed(false)
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
    fftw_free(reinterpret_cast<fftw_complex*>(m_buffer));
    fftw_destroy_plan(m_plan);
}

void AlignmentArray::init(size_t minimumSize)
{
    m_minimumSize = minimumSize;
    // Pad zeros to the nearest power of 2
    m_actualComplexSize = 1;
    while(minimumSize > m_actualComplexSize)
    {
        m_actualComplexSize *= 2;
    }
    QMutexLocker locker(&s_fftwPlanningMutex);
    fftw_complex* buf = fftw_alloc_complex(m_actualComplexSize);
    m_buffer = reinterpret_cast<std::complex<double>*>(buf);
    m_plan = fftw_plan_dft_1d(m_actualComplexSize, buf, buf, FFTW_FORWARD, FFTW_ESTIMATE);
    std::fill( m_buffer, m_buffer + m_actualComplexSize, std::complex<double>(0) );
}

void AlignmentArray::setValue(size_t index, double value)
{
    if (index < m_actualComplexSize) {
        m_buffer[index] = value;
    }
}

double AlignmentArray::calculateOffset(AlignmentArray &from, int* offset)
{
    // Create a destination for the correlation values
    s_fftwPlanningMutex.lock();
    std::complex<double>* correlationBuf = reinterpret_cast<std::complex<double>*>(fftw_alloc_complex(m_actualComplexSize));
    fftw_plan correlationPlan = fftw_plan_dft_1d(m_actualComplexSize, reinterpret_cast<fftw_complex*>(correlationBuf), reinterpret_cast<fftw_complex*>(correlationBuf), FFTW_BACKWARD, FFTW_ESTIMATE);
    s_fftwPlanningMutex.unlock();
    // Ensure the two sequences are transformed
    transform();
    from.transform();

    // Calculate the correlation
    for(size_t i = 0; i < m_actualComplexSize; ++i)
    {
        correlationBuf[i] = m_buffer[i] * std::conj(from.m_buffer[i]);
    }
    fftw_execute(correlationPlan);

    // Find the maximum correlation
    double max = 0;
    for (size_t i = 0; i < m_actualComplexSize; ++i)
    {
        double norm = std::norm(correlationBuf[i]);
        if (max < norm) {
            *offset = i;
            max = norm;
        }
    }

    if ( 2 * *offset > (int)m_actualComplexSize )
    {
        *offset -= ((int)m_actualComplexSize);
    }

    s_fftwPlanningMutex.lock();
    fftw_free(correlationBuf);
    fftw_destroy_plan(correlationPlan);
    s_fftwPlanningMutex.unlock();

    return max;
}

double AlignmentArray::calculateOffsetAndDrift(AlignmentArray& from, int precision, double drift_range, double* drift, int* offset)
{
    static const int FINAL_PRECISION = 9;
    double drift_step = std::pow(10.0, -precision);
    double max_score = *drift;
    double best_drift = 1.0;

    for(double d = *drift - drift_range; d <= *drift + drift_range; d += drift_step)
    {
        // Copy the "from" sequence with a shift
        AlignmentArray drifted(m_actualComplexSize);
        double factor = 1.0 / d;
        int shift = 0;
        for(size_t i = 0; i < m_minimumSize; ++i)
        {
            int newShift = std::round(factor * i) - i;
            while(newShift > shift)
            {
                drifted.m_buffer[i + shift] = from.m_buffer[i];
                ++shift;
            }
            shift = newShift;
            drifted.m_buffer[i + shift] = from.m_buffer[i];
        }
        double score = calculateOffset(drifted, offset);
        if(score > max_score)
        {
            max_score = score;
            best_drift = d;
        }
    }
    *drift = best_drift;

    if(precision < FINAL_PRECISION)
    {
        max_score = calculateOffsetAndDrift(from, precision + 1, drift_range / 10, drift, offset);
    }
    return max_score;
}

void AlignmentArray::transform()
{
    s_fftwPlanningMutex.lock();
    if (!m_isTransformed)
    {
        m_isTransformed = true;
        s_fftwPlanningMutex.unlock();
        fftw_execute(m_plan);
    }
    s_fftwPlanningMutex.unlock();
}
