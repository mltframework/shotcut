/*
 * Copyright (c) 2015 Meltytech, LLC
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

#ifndef IECSCALE_H
#define IECSCALE_H

//----------------------------------------------------------------------------
// IEC standard dB scaling -- as borrowed from meterbridge (c) Steve Harris

static inline double IEC_Scale(double dB)
{
    double fScale = 1.0f;

    if (dB < -70.0f)
        fScale = 0.0f;
    else if (dB < -60.0f)
        fScale = (dB + 70.0f) * 0.0025f;
    else if (dB < -50.0f)
        fScale = (dB + 60.0f) * 0.005f + 0.025f;
    else if (dB < -40.0)
        fScale = (dB + 50.0f) * 0.0075f + 0.075f;
    else if (dB < -30.0f)
        fScale = (dB + 40.0f) * 0.015f + 0.15f;
    else if (dB < -20.0f)
        fScale = (dB + 30.0f) * 0.02f + 0.3f;
    else if (dB < -0.001f || dB > 0.001f) /* if (dB < 0.0f) */
        fScale = (dB + 20.0f) * 0.025f + 0.5f;

    return fScale;
}

static inline double IEC_ScaleMax(double dB, double max)
{
    return IEC_Scale(dB) / IEC_Scale(max);
}

#endif // IECSCALE_H
