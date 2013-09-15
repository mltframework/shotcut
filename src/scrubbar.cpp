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

static const int margin = 14;        /// left and right margins
static const int selectionSize = 14; /// the height of the top bar
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
    setFont(QFont(font().family(), fontSize * devicePixelRatio()));
    m_timecodeWidth = fontMetrics().width("00:00:00:00") / devicePixelRatio();
    setMinimumHeight(fontMetrics().height() / devicePixelRatio() + selectionSize);
}

void ScrubBar::setScale(int maximum)
{
    m_max = maximum;
    /// m_scale is the pixels per frame ratio
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
    /// m_interval is the number of pixels per major tick to be labeled with time
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
    p.drawPixmap(0, 0, width(), height(), m_pixmap);

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

bool ScrubBar::event(QEvent *event)
{
    QWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        updatePixmap();
    return false;
}

void ScrubBar::updatePixmap()
{
    const int ratio = devicePixelRatio();
    const int l_width = width() * ratio;
    const int l_height = height() * ratio;
    const int l_margin = margin * ratio;
    const int l_selectionSize = selectionSize * ratio;
    const int l_interval = m_interval * ratio;
    const int l_timecodeWidth = m_timecodeWidth * ratio;
    m_pixmap = QPixmap(l_width, l_height);
    m_pixmap.fill(palette().window().color());
    QPainter p(&m_pixmap);
    p.setFont(font());
    const int markerHeight = fontMetrics().ascent() + 2 * ratio;

    if (!isEnabled()) {
        p.fillRect(0, 0, l_width, l_height, palette().background().color());
        p.end();
        update();
        return;
    }

    // background color
    p.fillRect(l_margin, 0, l_width - 2 * l_margin, l_height, palette().base().color());

    // selected region
    if (m_in > -1 && m_out > m_in) {
        const int in = m_in * m_scale * ratio;
        const int out = m_out * m_scale * ratio;
        p.fillRect(l_margin + in, 0, out - in, l_selectionSize, palette().highlight().color());
    }

    // draw time ticks
    QPen pen = QPen(palette().text().color());
    pen.setWidth(ratio);
    p.setPen(pen);
    if (l_interval > 2) {
        for (int x = l_margin; x < l_width - l_margin; x += l_interval) {
            p.drawLine(x, l_selectionSize, x, l_height - 1);
            if (x + l_interval / 4 < l_width - l_margin)
                p.drawLine(x + l_interval / 4,     l_height - 3 * ratio, x + l_interval / 4,     l_height - 1);
            if (x + l_interval / 2 < l_width - l_margin)
                p.drawLine(x + l_interval / 2,     l_height - 7 * ratio, x + l_interval / 2,     l_height - 1);
            if (x + l_interval * 3 / 4 < l_width - l_margin)
                p.drawLine(x + l_interval * 3 / 4, l_height - 3 * ratio, x + l_interval * 3 / 4, l_height - 1);
        }
    }

    // draw timecode
    if (l_interval > l_timecodeWidth && MLT.producer()) {
        int x = l_margin;
        for (int i = 0; x < l_width - l_margin - l_timecodeWidth; i++, x += l_interval) {
            int y = l_selectionSize + fontMetrics().ascent() - 2 * ratio;
            MLT.producer()->set("_shotcut_scrubbar", qRound(i * m_fps * m_secondsPerTick));
            p.drawText(x + 2 * ratio, y, MLT.producer()->get_time("_shotcut_scrubbar"));
        }
    }

    // draw markers
    if (m_in < 0 && m_out < 0) {
        int i = 1;
        foreach (int pos, m_markers) {
            int x = l_margin + pos * m_scale * ratio;
            QString s = QString::number(i++);
            int markerWidth = fontMetrics().width(s) * 1.5;
            p.fillRect(x, 0, 1, l_height, palette().highlight().color());
            p.fillRect(x - markerWidth/2, 0, markerWidth, markerHeight, palette().highlight().color());
            p.drawText(x - markerWidth/3, markerHeight - 2 * ratio, s);
        }
    }

    p.end();
    update();
}
