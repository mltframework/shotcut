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

#ifndef ALIGNMENTARRAY_H
#define ALIGNMENTARRAY_H

#include <QMutex>

#include <complex.h>
#include <fftw3.h>
#include <vector>

class AlignmentArray
{
public:
    AlignmentArray();
    AlignmentArray(size_t minimum_size);
    virtual ~AlignmentArray();

    void init(size_t minimum_size);
    void setValues(const std::vector<double> &values);
    double calculateOffset(AlignmentArray &from, int *offset);
    double calculateOffsetAndSpeed(AlignmentArray &from, double *speed_about, int *offset);

private:
    void transform();
    std::vector<double> m_values;
    fftw_plan m_forwardPlan;
    std::complex<double> *m_forwardBuf;
    fftw_plan m_backwardPlan;
    std::complex<double> *m_backwardBuf;
    double m_autocorrelationMax;
    size_t m_minimumSize;
    size_t m_actualComplexSize;
    bool m_isTransformed;
    QMutex m_transformMutex;
};

#endif

