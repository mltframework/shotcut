/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "audioscale.h"
#include <QFont>
#include <QPainter>
#include <QDebug>

AudioScale::AudioScale(QWidget *parent) :
    QWidget(parent)
{
    const QFont& font = QWidget::font();
    const int fontSize = font.pointSize() - (font.pointSize() > 10? 2 : (font.pointSize() > 8? 1 : 0));
    setFont(QFont(font.family(), fontSize));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMinimumWidth(fontMetrics().width("-60"));
    setFocusPolicy(Qt::NoFocus);
    dbscale << 5 << 0 << -5 << -10 << -15 << -20 << -25 << -30 << -35 << -40 << -50 << -70;
}

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
    else if (dB < -0.001f || dB > 0.001f)  /* if (dB < 0.0f) */
        fScale = (dB + 20.0f) * 0.025f + 0.5f;

    return fScale;
}

void AudioScale::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const int h = IEC_Scale(-dbscale[0]) * height() - 2;
    foreach (int i, dbscale) {
        if (height() > width()) {
            if (i != dbscale[0]) {
                double xf = IEC_Scale(i) * h;
                QString s = QString().sprintf("%d", i);
                p.drawText(width() - fontMetrics().width(s), height() - xf - 1, s);
            }
        } else {
            double xf = IEC_Scale(i) * (double) width();
            p.drawText(xf * 40.0/42.0 - 10, height() - 2, QString().sprintf("%d", i));
        }
    }
    p.end();
}
