/*
 * Copyright (c) 2011 Meltytech, LLC
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

#include "scrubbar.h"
#include <QtGui>

static const int margin = 14;

ScrubBar::ScrubBar(QWidget *parent)
    : QWidget(parent)
    , m_position(0)
    , m_scale(-1)
    , m_fps(25)
    , m_max(1)
    , m_in(-1)
    , m_out(-1)
    , m_activeControl(CONTROL_NONE)
{
    setMouseTracking(true);
    setMinimumHeight(14);
}

void ScrubBar::setScale(int maximum)
{
    m_max = maximum;
    m_scale = (double) (width() - 2 * margin) / (double) maximum;
    if (m_scale == 0) m_scale = -1;
    if (m_scale > 0.5)
        m_interval = 1 * m_fps; // 1 second
    else if (m_scale > 0.09)
        m_interval = 5 * m_fps; // 5 seconds
    else
        m_interval = 60 * m_fps; // 60 seconds
    m_interval *= m_scale;
    m_cursorPosition = m_position * m_scale;
    updatePixmap();
}

void ScrubBar::setFramerate(double fps)
{
    m_fps = fps;
}

int ScrubBar::position() const
{
    return m_position;
}

void ScrubBar::setInPoint(int in)
{
    m_in = qMax(in, 0);
    updatePixmap();
}

void ScrubBar::setOutPoint(int out)
{
    m_out = qMin(out, m_max);
    updatePixmap();
}

void ScrubBar::mousePressEvent(QMouseEvent * event)
{
    int x = event->x() - margin;
    int in = m_in * m_scale;
    int out = m_out * m_scale;
    int pos = x / m_scale;

    if (m_in > -1 && m_out > -1) {
        if (x >= in - 12 && x <= in + 6) {
            m_activeControl = CONTROL_IN;
            setInPoint(pos);
        }
        else if (x >= out - 6 && x <= out + 12) {
            m_activeControl = CONTROL_OUT;
            setOutPoint(pos);
        }
    }
    emit seeked(pos);
}

void ScrubBar::mouseReleaseEvent(QMouseEvent * event)
{
    m_activeControl = CONTROL_NONE;
}

void ScrubBar::mouseMoveEvent(QMouseEvent * event)
{
    int x = event->x() - margin;
    int pos = x / m_scale;

    if (event->buttons() & Qt::LeftButton) {
        if (m_activeControl == CONTROL_IN)
            setInPoint(pos);
        else if (m_activeControl == CONTROL_OUT)
            setOutPoint(pos);
        emit seeked(pos);
    }
}

bool ScrubBar::onSeek(int value)
{
    if (value == m_position)
        return false;
    m_position = value;
    int oldPos = m_cursorPosition;
    m_cursorPosition = value * m_scale;
    const int offset = height() / 2;
    const int x = qMin(oldPos, m_cursorPosition);
    const int w = qAbs(oldPos - m_cursorPosition);
    update(margin + x - offset, 0, w + 2 * offset, height());
    return true;
}

void ScrubBar::paintEvent(QPaintEvent *e)
{

    QPainter p(this);
    QRect r = e->rect();
    p.setClipRect(r);
    p.drawPixmap(QPointF(), m_pixmap);

    // draw pointer
    QPolygon pa(3);
    const int x = height() / 2 - 1;
    pa.setPoints(3, margin + m_cursorPosition - x + 1, 0, margin + m_cursorPosition + x, 0, margin + m_cursorPosition, x - 1);
    p.setBrush(palette().text().color());
    p.setPen(Qt::NoPen);
    p.drawPolygon(pa);
    p.setPen(palette().text().color());
    p.drawLine(margin + m_cursorPosition, 0, margin + m_cursorPosition, height() - 1);

    // draw in point
    if (m_in > -1) {
        const int in = margin + m_in * m_scale;
        pa.setPoints(3, in - 7, 0, in - 7, height() - 1, in - 1, height() / 2);
        p.setBrush(palette().text().color());
        p.setPen(Qt::NoPen);
        p.drawPolygon(pa);
        p.setPen(QPen(QBrush(palette().text().color()), 2));
        p.drawLine(in, 0, in, height() - 1);
    }

    // draw out point
    if (m_out > -1) {
        const int out = margin + m_out * m_scale;
        pa.setPoints(3, out + 7, 0, out + 7, height() - 1, out, height() / 2);
        p.setBrush(palette().text().color());
        p.setPen(Qt::NoPen);
        p.drawPolygon(pa);
        p.setPen(QPen(QBrush(palette().text().color()), 2));
        p.drawLine(out, 0, out, height() - 1);
    }
}

void ScrubBar::resizeEvent(QResizeEvent *)
{
    setScale(m_max);
}

void ScrubBar::updatePixmap()
{
    m_pixmap = QPixmap(width(), height());
    m_pixmap.fill(palette().window().color());
    QPainter p(&m_pixmap);
    int y = height() / 4;

    // background color
    p.fillRect(margin, 0, width() - 2 * margin, height(), palette().base().color());

    // selected region
    if (m_in > -1 && m_out > m_in) {
        const int in = m_in * m_scale;
        const int out = m_out * m_scale;
        p.fillRect(margin + in, 0, out - in, height(), palette().highlight().color());
    }

    // draw time ticks
    p.setPen(palette().light().color());
    if (m_interval > 2) {
        int x = margin;
        for (int i = 0; x < width() - margin; x = margin + ++i * m_interval)
            p.drawLine(x, height() - 1 - y /*- y * !(i % 5)*/, x, height() - 1);
    }

    p.end();
    update();
}
