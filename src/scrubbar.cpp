/*
 * Copyright (c) 2011-2024 Meltytech, LLC
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
#include "settings.h"

#include <QToolTip>
#include <QtWidgets>

static const int selectionSize = 14; /// the height of the top bar
#ifndef CLAMP
#define CLAMP(x, min, max) (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))
#endif

ScrubBar::ScrubBar(QWidget *parent)
    : QWidget(parent)
    , m_head(-1)
    , m_scale(-1)
    , m_fps(25)
    , m_max(1)
    , m_in(-1)
    , m_out(-1)
    , m_margin(14) /// left and right margins
    , m_activeControl(CONTROL_NONE)
    , m_timecodeWidth(0)
    , m_loopStart(-1)
    , m_loopEnd(-1)
{
    setMouseTracking(true);
    setMinimumHeight(fontMetrics().height() + selectionSize);
}

void ScrubBar::setScale(int maximum)
{
    if (!m_timecodeWidth) {
        const int fontSize = font().pointSize()
                             - (font().pointSize() > 10 ? 2 : (font().pointSize() > 8 ? 1 : 0));
        setFont(QFont(font().family(), fontSize * devicePixelRatioF()));
        m_timecodeWidth = fontMetrics().horizontalAdvance("00:00:00:00") / devicePixelRatioF();
    }
    m_max = maximum;
    /// m_scale is the pixels per frame ratio
    m_scale = (double) (width() - 2 * m_margin) / (double) maximum;
    if (m_scale == 0)
        m_scale = -1;
    m_secondsPerTick = qMax(qRound(double(m_timecodeWidth * 1.8) / m_scale / m_fps), 1);
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
    m_head = -1;
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

void ScrubBar::setLoopRange(int start, int end)
{
    m_loopStart = start;
    m_loopEnd = end;
    updatePixmap();
}

void ScrubBar::mousePressEvent(QMouseEvent *event)
{
    int x = event->position().x() - m_margin;
    int in = m_in * m_scale;
    int out = m_out * m_scale;
    int head = m_head * m_scale;
    int pos = CLAMP(x / m_scale, 0, m_max);

    if (m_in > -1 && m_out > -1) {
        if (x >= in - 12 && x <= in + 6) {
            m_activeControl = CONTROL_IN;
            setInPoint(pos);
        } else if (x >= out - 6 && x <= out + 12) {
            m_activeControl = CONTROL_OUT;
            setOutPoint(pos);
        }
    }
    if (m_head > -1) {
        if (m_activeControl == CONTROL_NONE) {
            m_activeControl = CONTROL_HEAD;
            m_head = pos;
            const int offset = height() / 2;
            const int x = head;
            const int w = qAbs(x - head);
            update(m_margin + x - offset, 0, w + 2 * offset, height());
        }
    }
    if (m_activeControl >= CONTROL_IN && !Settings.playerPauseAfterSeek())
        emit paused(pos);
    emit seeked(pos);
}

void ScrubBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_activeControl = CONTROL_NONE;
}

void ScrubBar::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->position().x() - m_margin;
    int pos = CLAMP(x / m_scale, 0, m_max);

    if (event->buttons() & Qt::LeftButton) {
        if (m_activeControl == CONTROL_IN)
            setInPoint(pos);
        else if (m_activeControl == CONTROL_OUT)
            setOutPoint(pos);
        else if (m_activeControl == CONTROL_HEAD) {
            const int head = m_head * m_scale;
            const int offset = height() / 2;
            const int x = head;
            const int w = qAbs(x - head);
            update(m_margin + x - offset, 0, w + 2 * offset, height());
            m_head = pos;
        }
        if (m_activeControl >= CONTROL_IN && !Settings.playerPauseAfterSeek())
            emit paused(pos);
        emit seeked(pos);
    } else if (event->buttons() == Qt::NoButton && MLT.producer()) {
        QString text = QString::fromLatin1(
            MLT.producer()->frames_to_time(pos, Settings.timeFormat()));
        QToolTip::showText(event->globalPosition().toPoint(), text);
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
    update(m_margin + x - offset, 0, w + 2 * offset, height());
    return true;
}

void ScrubBar::paintEvent(QPaintEvent *e)
{
    QPen pen(QBrush(palette().text().color()), 2);
    QPainter p(this);
    QRect r = e->rect();
    p.setClipRect(r);
    p.drawPixmap(0, 0, width(), height(), m_pixmap);

    if (!isEnabled())
        return;

    // draw pointer
    QPolygon pa(3);
    const int x = selectionSize / 2 - 1;
    int head = m_margin + m_cursorPosition;
    pa.setPoints(3, head - x - 1, 0, head + x, 0, head, x);
    p.setBrush(palette().text().color());
    p.setPen(Qt::NoPen);
    p.drawPolygon(pa);
    p.setPen(pen);
    if (m_head >= 0) {
        head = m_margin + m_head * m_scale;
        p.drawLine(head, 0, head, height() - 1);
    }

    // draw in point
    if (m_in > -1) {
        const int in = m_margin + m_in * m_scale;
        pa.setPoints(3,
                     in - selectionSize / 2,
                     0,
                     in - selectionSize / 2,
                     selectionSize - 1,
                     in - 1,
                     selectionSize / 2);
        p.setBrush(palette().text().color());
        p.setPen(Qt::NoPen);
        p.drawPolygon(pa);
        p.setPen(pen);
        p.drawLine(in, 0, in, selectionSize - 1);
    }

    // draw out point
    if (m_out > -1) {
        const int out = m_margin + m_out * m_scale;
        pa.setPoints(3,
                     out + selectionSize / 2,
                     0,
                     out + selectionSize / 2,
                     selectionSize - 1,
                     out,
                     selectionSize / 2);
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
    const auto ratio = devicePixelRatioF();
    const int l_width = width() * ratio;
    const int l_height = height() * ratio;
    const int l_margin = m_margin * ratio;
    const int l_selectionSize = selectionSize * ratio;
    const int l_interval = m_interval * ratio;
    const int l_timecodeWidth = m_timecodeWidth * ratio;
    m_pixmap = QPixmap(l_width, l_height);
    m_pixmap.fill(palette().window().color());
    QPainter p(&m_pixmap);
    p.setFont(font());
    const int markerHeight = fontMetrics().ascent() + 2 * ratio;
    QPen pen;

    if (!isEnabled()) {
        p.fillRect(0, 0, l_width, l_height, palette().window().color());
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
        p.fillRect(l_margin + in, 0, out - in, l_selectionSize, Qt::red);
        p.fillRect(l_margin + in + (2 + ratio),
                   ratio, // 2 for the in point line
                   out - in - 2 * (2 + ratio) - qFloor(0.5 * ratio),
                   l_selectionSize - ratio * 2,
                   palette().highlight().color());
    }

    // draw time ticks
    pen.setColor(palette().text().color());
    pen.setWidth(qRound(ratio));
    p.setPen(pen);
    if (l_interval > 2) {
        for (int x = l_margin; x < l_width - l_margin; x += l_interval) {
            p.drawLine(x, l_selectionSize, x, l_height - 1);
            if (x + l_interval / 4 < l_width - l_margin)
                p.drawLine(x + l_interval / 4,
                           l_height - 3 * ratio,
                           x + l_interval / 4,
                           l_height - 1);
            if (x + l_interval / 2 < l_width - l_margin)
                p.drawLine(x + l_interval / 2,
                           l_height - 7 * ratio,
                           x + l_interval / 2,
                           l_height - 1);
            if (x + l_interval * 3 / 4 < l_width - l_margin)
                p.drawLine(x + l_interval * 3 / 4,
                           l_height - 3 * ratio,
                           x + l_interval * 3 / 4,
                           l_height - 1);
        }
    }

    // draw timecode
    const auto timeFormat = Settings.timeFormat();
    if (l_interval > l_timecodeWidth && MLT.producer()) {
        int x = l_margin;
        for (int i = 0; x < l_width - l_margin - l_timecodeWidth; i++, x += l_interval) {
            int y = l_selectionSize + fontMetrics().ascent() - 2 * ratio;
            int frames = qRound(i * m_fps * m_secondsPerTick);
            p.drawText(x + 2 * ratio,
                       y,
                       QString(MLT.producer()->frames_to_time(frames, timeFormat)).left(8));
        }
    }

    // draw markers
    if (m_in < 0 && m_out < 0) {
        int i = 1;
        foreach (int pos, m_markers) {
            int x = l_margin + pos * m_scale * ratio;
            QString s = QString::number(i++);
            int markerWidth = fontMetrics().horizontalAdvance(s) * 1.5;
            p.fillRect(x, 0, 1, l_height, palette().highlight().color());
            p.fillRect(x - markerWidth / 2,
                       0,
                       markerWidth,
                       markerHeight,
                       palette().highlight().color());
            p.drawText(x - markerWidth / 3, markerHeight - 2 * ratio, s);
        }
    }

    // draw loop range
    if (m_loopStart > -1 && m_loopEnd > -1) {
        const int start = m_loopStart * m_scale * ratio;
        const int end = m_loopEnd * m_scale * ratio;
        QColor loopColor = palette().highlight().color();
        loopColor.setAlphaF(0.5);
        p.fillRect(l_margin + start, l_height - 7 * ratio, end - start, l_height * ratio, loopColor);
    }

    p.end();
    update();
}
