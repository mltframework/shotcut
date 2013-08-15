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
#include "mltcontroller.h"
#include <QtWidgets>

static const int margin = 14;
static const int selectionSize = 14;
#ifndef CLAMP
#define CLAMP(x, min, max) (((x) < (min))? (min) : ((x) > (max))? (max) : (x))
#endif

ScrubBar::ScrubBar(QWidget *parent)
    : QWidget(parent)
    , m_head(0)
    , m_scale(-1)
    , m_fps(25)
    , m_max(1)
    , m_in(-1)
    , m_out(-1)
    , m_activeControl(CONTROL_NONE)
{
    setMouseTracking(true);
    const int fontSize = font().pointSize() - (font().pointSize() > 10? 2 : (font().pointSize() > 8? 1 : 0));
    setFont(QFont(font().family(), fontSize));
    m_timecodeWidth = fontMetrics().width("00:00:00:00");
    setMinimumHeight(fontMetrics().height() + selectionSize);
}

void ScrubBar::setScale(int maximum)
{
    m_max = maximum;
    m_scale = (double) (width() - 2 * margin) / (double) maximum;
    if (m_scale == 0) m_scale = -1;
    m_secondsPerTick = qRound(double(m_timecodeWidth * 1.6) / m_scale / m_fps);
    if (m_secondsPerTick > 3600)
        // force to a multiple of one hour
        m_secondsPerTick += 3600 - m_secondsPerTick % 3600;
    else if (m_secondsPerTick > 300)
        // force to a multiple of 5 minutes
        m_secondsPerTick += 300 - m_secondsPerTick % 300;
    else if (m_secondsPerTick > 60)
        // force to a multiple of one minute
        m_secondsPerTick += 60 - m_secondsPerTick % 60;
    else if (m_secondsPerTick > 5)
        // force to a multiple of 10 seconds
        m_secondsPerTick += 10 - m_secondsPerTick % 10;
    else if (m_secondsPerTick > 2)
        // force to a multiple of 5 seconds
        m_secondsPerTick += 5 - m_secondsPerTick % 5;
    m_interval = qRound(double(m_secondsPerTick) * m_fps * m_scale);

    updatePixmap();
}

void ScrubBar::setFramerate(double fps)
{
    m_fps = fps;
}

int ScrubBar::position() const
{
    return m_head;
}

void ScrubBar::setInPoint(int in)
{
    m_in = qMax(in, -1);
    updatePixmap();
    emit inChanged(in);
}

void ScrubBar::setOutPoint(int out)
{
    m_out = qMin(out, m_max);
    updatePixmap();
    emit outChanged(out);
}

void ScrubBar::setMarkers(const QList<int> &list)
{
    m_markers = list;
    updatePixmap();
}

void ScrubBar::mousePressEvent(QMouseEvent * event)
{
    int x = event->x() - margin;
    int in = m_in * m_scale;
    int out = m_out * m_scale;
    int head = m_head * m_scale;
    int pos = CLAMP(x / m_scale, 0, m_max);

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
    if (m_head > -1) {
        if (m_activeControl == CONTROL_NONE) {
            m_activeControl = CONTROL_HEAD;
            m_head = pos;
            const int offset = height() / 2;
            const int x = qMin(x, head);
            const int w = qAbs(x - head);
            update(margin + x - offset, 0, w + 2 * offset, height());
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
    int pos = CLAMP(x / m_scale, 0, m_max);

    if (event->buttons() & Qt::LeftButton) {
        if (m_activeControl == CONTROL_IN)
            setInPoint(pos);
        else if (m_activeControl == CONTROL_OUT)
            setOutPoint(pos);
        else if (m_activeControl == CONTROL_HEAD) {
            const int head = m_head * m_scale;
            const int offset = height() / 2;
            const int x = qMin(x, head);
            const int w = qAbs(x - head);
            update(margin + x - offset, 0, w + 2 * offset, height());
            m_head = pos;
        }
        emit seeked(pos);
    }
}

bool ScrubBar::onSeek(int value)
{
    if (m_activeControl != CONTROL_HEAD)
        m_head = value;
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
    QPen pen(QBrush(palette().text().color()), 2);
    QPainter p(this);
    QRect r = e->rect();
    p.setClipRect(r);
    p.drawPixmap(QPointF(), m_pixmap);

    if (!isEnabled()) return;

    // draw pointer
    QPolygon pa(3);
    const int x = selectionSize / 2 - 1;
    int head = margin + m_cursorPosition;
    pa.setPoints(3, head - x, 0, head + x, 0, head, x);
    p.setBrush(palette().text().color());
    p.setPen(Qt::NoPen);
    p.drawPolygon(pa);
    p.setPen(pen);
    head = margin + m_head * m_scale;
    p.drawLine(head, 0, head, height() - 1);

    // draw in point
    if (m_in > -1) {
        const int in = margin + m_in * m_scale;
        pa.setPoints(3, in - selectionSize / 2, 0, in - selectionSize / 2, selectionSize - 1, in - 1, selectionSize / 2);
        p.setBrush(palette().text().color());
        p.setPen(Qt::NoPen);
        p.drawPolygon(pa);
        p.setPen(pen);
        p.drawLine(in, 0, in, selectionSize - 1);
    }

    // draw out point
    if (m_out > -1) {
        const int out = margin + m_out * m_scale;
        pa.setPoints(3, out + selectionSize / 2, 0, out + selectionSize / 2, selectionSize - 1, out, selectionSize / 2);
        p.setBrush(palette().text().color());
        p.setPen(Qt::NoPen);
        p.drawPolygon(pa);
        p.setPen(pen);
        p.drawLine(out, 0, out, selectionSize - 1);
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
    p.setFont(font());
    int markerHeight = fontMetrics().ascent() + 2;

    if (!isEnabled()) {
        p.fillRect(0, 0, width(), height(), palette().background().color());
        p.end();
        update();
        return;
    }

    // background color
    p.fillRect(margin, 0, width() - 2 * margin, height(), palette().base().color());

    // selected region
    if (m_in > -1 && m_out > m_in) {
        const int in = m_in * m_scale;
        const int out = m_out * m_scale;
        p.fillRect(margin + in, 0, out - in, selectionSize, palette().highlight().color());
    }

    // draw time ticks
    p.setPen(palette().text().color());
    if (m_interval > 2) {
        for (int x = margin; x < width() - margin; x += m_interval) {
            p.drawLine(x, selectionSize, x, height() - 1);
            if (x + m_interval / 4 < width() - margin)
                p.drawLine(x + m_interval / 4,     height() - 3, x + m_interval / 4,     height() - 1);
            if (x + m_interval / 2 < width() - margin)
                p.drawLine(x + m_interval / 2,     height() - 7, x + m_interval / 2,     height() - 1);
            if (x + m_interval * 3 / 4 < width() - margin)
                p.drawLine(x + m_interval * 3 / 4, height() - 3, x + m_interval * 3 / 4, height() - 1);
        }
    }

    // draw timecode
    if (m_interval > m_timecodeWidth) {
        int x = margin;
        for (int i = 0; x < width() - margin - m_timecodeWidth; i++, x += m_interval) {
            MLT.producer()->set("_shotcut_scrubbar", i * m_fps * m_secondsPerTick);
            p.drawText(x + 2, 14 + fontMetrics().ascent() - 2, MLT.producer()->get_time("_shotcut_scrubbar"));
        }
    }

    // draw markers
    if (m_in < 0 && m_out < 0) {
        int i = 1;
        foreach (int pos, m_markers) {
            int x = margin + pos * m_scale;
            QString s = QString::number(i++);
            int markerWidth = fontMetrics().width(s) * 1.5;
            p.fillRect(x, 0, 1, height(), palette().highlight().color());
            p.fillRect(x - markerWidth/2, 0, markerWidth, markerHeight, palette().highlight().color());
            p.drawText(x - markerWidth/3, markerHeight - 2, s);
        }
    }

    p.end();
    update();
}
