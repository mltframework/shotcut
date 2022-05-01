/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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
#include "iecscale.h"
#include <QFont>
#include <QPainter>
#include <Logger.h>

AudioScale::AudioScale(QWidget *parent) :
    QWidget(parent)
{
    const QFont &font = QWidget::font();
    const int fontSize = font.pointSize() - (font.pointSize() > 10 ? 2 : (font.pointSize() > 8 ? 1 :
                                                                          0));
    setFont(QFont(font.family(), fontSize));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMinimumWidth(fontMetrics().horizontalAdvance("-60"));
    setFocusPolicy(Qt::NoFocus);
    dbscale << 5 << 0 << -5 << -10 << -15 << -20 << -25 << -30 << -35 << -40 << -50;
}

void AudioScale::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    const int h = IEC_Scale(-dbscale[0]) * height() - 2;
    foreach (int i, dbscale) {
        if (height() > width()) {
            if (i != dbscale[0]) {
                double xf = IEC_Scale(i) * h;
                QString s = QString::asprintf("%d", i);
                p.drawText(width() - fontMetrics().horizontalAdvance(s), height() - xf - 1, s);
            }
        } else {
            double xf = IEC_Scale(i) * (double) width();
            p.drawText(xf * 40.0 / 42.0 - 10, height() - 2, QString::asprintf("%d", i));
        }
    }
    p.end();
}
