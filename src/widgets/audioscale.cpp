/*
 * Copyright (c) 2015-2026 Meltytech, LLC
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

#include "Logger.h"
#include "iecscale.h"

#include <QFont>
#include <QMouseEvent>
#include <QPainter>

AudioScale::AudioScale(QWidget *parent)
    : QWidget(parent)
{
    const QFont &font = QWidget::font();
    const int fontSize = font.pointSize()
                         - (font.pointSize() > 10 ? 2 : (font.pointSize() > 8 ? 1 : 0));
    setFont(QFont(font.family(), fontSize));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMinimumWidth(fontMetrics().horizontalAdvance("-60"));
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    dbscale << 5 << 0 << -5 << -10 << -15 << -20 << -25 << -30 << -35 << -40 << -50;
}

void AudioScale::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    const int h = IEC_Scale(-dbscale[0]) * height() - 2;
    foreach (int i, dbscale) {
        if (i == dbscale[0])
            continue;
        const QString text = QString::asprintf("%d", i);
        if (height() > width()) {
            const double xf = IEC_Scale(i) * h;
            const int x = width() - fontMetrics().horizontalAdvance(text);
            p.drawText(x, height() - xf - 1, text);
        } else {
            double xf = IEC_Scale(i) * (double) width();
            p.drawText(xf * 40.0 / 42.0 - 10, height() - 2, text);
        }
    }
    p.end();
}

void AudioScale::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && height() > width()) {
        const int h = IEC_Scale(-dbscale[0]) * height() - 2;
        for (int i : dbscale) {
            if (i == dbscale[0])
                continue;
            const QString text = QString::asprintf("%d", i);
            const int textWidth = fontMetrics().horizontalAdvance(text);
            const int x = width() - textWidth;
            const int y = qRound(height() - IEC_Scale(i) * h - 1);
            const QRect labelRect(x, y - fontMetrics().ascent(), textWidth, fontMetrics().height());
            if (labelRect.contains(event->pos())) {
                emit volumeSelected(IEC_Scale(i));
                event->accept();
                return;
            }
        }
    }
    QWidget::mousePressEvent(event);
}
