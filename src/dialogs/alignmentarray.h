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

#include <complex.h>
#include <fftw3.h>

class AlignmentArray
{
public:
    AlignmentArray();
    AlignmentArray(size_t minimum_size);
    virtual ~AlignmentArray();

    void init(size_t minimum_size);
    void setValue(size_t index, double value);
    double calculateOffset(AlignmentArray &from, int* offset);
    double calculateOffsetAndDrift(AlignmentArray& from, int precision, double drift_range, double* drift_about, int* offset);

private:
    void transform();
    fftw_plan m_plan;
    std::complex<double>* m_buffer;
    size_t m_minimumSize;
    size_t m_actualComplexSize;
    bool m_isTransformed = false;
};

#endif

