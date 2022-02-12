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

#include "audiometerwidget.h"
#include "iecscale.h"
#include <QPainter>
#include <QColor>
#include <QtAlgorithms>
#include <QToolTip>

static const int TEXT_PAD = 2;

AudioMeterWidget::AudioMeterWidget(QWidget *parent): QWidget(parent)
{
    const QFont& font = QWidget::font();
    const int fontSize = font.pointSize() - (font.pointSize() > 10? 2 : (font.pointSize() > 8? 1 : 0));
    QWidget::setFont(QFont(font.family(), fontSize));
    QWidget::setMouseTracking(true);
}

void AudioMeterWidget::setDbLabels(const QVector<int>& labels)
{
    m_dbLabels = labels;
    if (m_dbLabels.size()) {
        std::sort(m_dbLabels.begin(), m_dbLabels.end());
        m_maxDb = m_dbLabels[m_dbLabels.size() - 1];
    }
    calcGraphRect();
}

void AudioMeterWidget::setChannelLabels(const QStringList& labels)
{
    m_chanLabels = labels;
    calcGraphRect();
}

void AudioMeterWidget::setChannelLabelUnits(const QString& units)
{
    m_chanLabelUnits = units;
}

void AudioMeterWidget::setOrientation(Qt::Orientation orientation)
{
    m_orient = orientation;
    calcGraphRect();
}

void AudioMeterWidget::showAudio(const QVector<double>& dbLevels)
{
    m_levels = dbLevels;
    if (m_peaks.size() != m_levels.size()) {
        m_peaks = m_levels;
        calcGraphRect();
    } else {
        for (int i = 0; i < m_levels.size(); i++)
        {
            m_peaks[i] = m_peaks[i] - 0.2;
            if (m_levels[i] >= m_peaks[i]) {
                m_peaks[i] = m_levels[i];
            }
        }
    }
    update();
    updateToolTip();
}

void AudioMeterWidget::calcGraphRect()
{
    int chanLabelCount = m_chanLabels.size();
    int dbLabelCount = m_dbLabels.size();
    int textHeight = fontMetrics().height() + TEXT_PAD;
    int chanCount = m_levels.size() ? m_levels.size() : chanLabelCount ? chanLabelCount : 2;

    if (m_orient == Qt::Horizontal) {
        int dbLabelHeight = dbLabelCount ? textHeight : 0;
        // Find the widest channel label
        int chanLabelWidth = 0;
        for (int i = 0; i < chanLabelCount; i++) {
            int width = fontMetrics().width(m_chanLabels[i]) + TEXT_PAD;
            chanLabelWidth = width > chanLabelWidth ? width : chanLabelWidth;
        }
        int chanHeight = (height() - dbLabelHeight) / chanCount;
        m_graphRect.setTop(0);
        m_graphRect.setRight(width());
        m_graphRect.setBottom(chanHeight * chanCount);
        m_graphRect.setLeft(chanLabelWidth);
        m_barSize.setWidth(m_graphRect.width());
        m_barSize.setHeight(chanHeight);

        m_gradient.setStart(m_graphRect.left(), 0);
        m_gradient.setFinalStop(m_graphRect.right(), 0);
    } else { // Vertical
        int chanLabelHeight = chanLabelCount ? textHeight : 0;
        // Find the widest db label
        int dbLabelWidth = 0;
        for (int i = 0; i < dbLabelCount; i++) {
            QString label = QString::asprintf("%d", m_dbLabels[i]);
            int size = fontMetrics().width(label) + TEXT_PAD;
            dbLabelWidth = size > dbLabelWidth ? size : dbLabelWidth;
        }
        int chanWidth = (width() - dbLabelWidth) / chanCount;
        m_graphRect.setTop(0);
        m_graphRect.setRight(dbLabelWidth + chanWidth * chanCount);
        m_graphRect.setBottom(height() - chanLabelHeight);
        m_graphRect.setLeft(dbLabelWidth);
        m_barSize.setWidth(chanWidth);
        m_barSize.setHeight(m_graphRect.height());

        m_gradient.setStart(0, m_graphRect.bottom());
        m_gradient.setFinalStop(0, m_graphRect.top());
    }

    m_gradient.setColorAt(IEC_ScaleMax(-90.0, m_maxDb), Qt::darkGreen);
    m_gradient.setColorAt(IEC_ScaleMax(-12.0, m_maxDb), Qt::green);
    m_gradient.setColorAt(IEC_ScaleMax(-6.0, m_maxDb), Qt::yellow);
    m_gradient.setColorAt(IEC_ScaleMax(0.0, m_maxDb), Qt::red);
    if (m_maxDb > 0.0 ) {
        m_gradient.setColorAt(IEC_ScaleMax(m_maxDb, m_maxDb), Qt::darkRed);
    }
}

void AudioMeterWidget::drawDbLabels(QPainter& p)
{
    int dbLabelCount = m_dbLabels.size();
    int textHeight = fontMetrics().height();
    int x = 0;
    int y = 0;

    if (dbLabelCount == 0) return;

    p.setPen(palette().text().color().rgb());

    if (m_orient == Qt::Horizontal) {
        // dB scale is horizontal along the bottom
        int prevX = 0;
        y = m_graphRect.bottom() + textHeight + TEXT_PAD;
        for (int i = 0; i < dbLabelCount; i++) {
            int value = m_dbLabels[i];
            QString label = QString::asprintf("%d", value);
            int labelWidth = fontMetrics().width(label);
            x = m_graphRect.left() + IEC_ScaleMax(value, m_maxDb) * m_graphRect.width() - labelWidth / 2;
            if (x + labelWidth > width()) {
                x = width() - labelWidth;
            }
            if (x - prevX >= TEXT_PAD) {
                p.drawText(x, y, label);
                prevX = x + labelWidth;
            }
        }
    } else {
        // dB scale is vertical along the left side
        int prevY = height();
        for (int i = 0; i < dbLabelCount; i++) {
            int value = m_dbLabels[i];
            QString label = QString::asprintf("%d", value);
            x = m_graphRect.left() - fontMetrics().width(label) - TEXT_PAD;
            y = m_graphRect.bottom() - qRound(IEC_ScaleMax(value, m_maxDb) * (double)m_graphRect.height() - (double)textHeight / 2.0);
            if (y - textHeight < 0) {
                y = textHeight;
            }
            if (prevY - y >= TEXT_PAD) {
                p.drawText(x, y, label);
                prevY = y - textHeight;
            }
        }
    }
}

void AudioMeterWidget::drawChanLabels(QPainter& p)
{
    int chanLabelCount = m_chanLabels.size();
    int textHeight = fontMetrics().height();
    int stride = 1;
    int x = 0;
    int y = 0;

    if (chanLabelCount == 0) return;

    p.setPen(palette().text().color().rgb());

    if (m_orient == Qt::Horizontal) {
        // Channel labels are vertical along the left side.

        while( textHeight * chanLabelCount / stride > m_graphRect.width() ) {
            stride++;
        }

        int prevY = m_graphRect.top();
        for (int i = 0; i < chanLabelCount; i += stride) {
            const QString& label = m_chanLabels[i];
            y = m_graphRect.bottom() - (chanLabelCount - 1 - i) * m_barSize.height() - m_barSize.height() / 2 + textHeight / 2;
            x = m_graphRect.left() - fontMetrics().width(label) - TEXT_PAD;
            if ( y - prevY >= TEXT_PAD) {
                p.drawText(x, y, label);
                prevY = y - textHeight;
            }
        }
    } else {
        // Channel labels are horizontal along the bottom.

        // Find the widest channel label
        int chanLabelWidth = 0;
        for (int i = 0; i < chanLabelCount; i++) {
            int width = fontMetrics().width(m_chanLabels[i]) + TEXT_PAD;
            chanLabelWidth = width > chanLabelWidth ? width : chanLabelWidth;
        }

        while( chanLabelWidth * chanLabelCount / stride > m_graphRect.width() ) {
            stride++;
        }

        int prevX = 0;
        y = m_graphRect.bottom() + textHeight + TEXT_PAD;
        for (int i = 0; i < chanLabelCount; i += stride) {
            QString label = m_chanLabels[i];
            x = m_graphRect.left() + i * m_barSize.width() + m_barSize.width() / 2 - fontMetrics().width(label) / 2;
            if (x > prevX) {
                p.drawText(x, y, label);
                prevX = x + fontMetrics().width(label);
            }
        }
    }
}

void AudioMeterWidget::drawBars(QPainter& p)
{
    int chanCount = m_levels.size();
    QRectF bar;

    if (m_orient == Qt::Horizontal) {
        for (int i = 0; i < chanCount; i++) {
            double level = IEC_ScaleMax(m_levels[i], m_maxDb);
            bar.setLeft(m_graphRect.left());
            bar.setRight(bar.left() + m_barSize.width() * level);
            bar.setBottom(m_graphRect.bottom() - (chanCount - 1 - i) * m_barSize.height() - 1);
            bar.setTop(bar.bottom() - m_barSize.height() + 1);
            p.drawRoundedRect(bar, 3, 3);
        }
    } else {
        for (int i = 0; i < chanCount; i++) {
            double level = IEC_ScaleMax(m_levels[i], m_maxDb);
            bar.setLeft(m_graphRect.left() + i * m_barSize.width() + 1);
            bar.setRight(bar.left() + m_barSize.width() - 1);
            bar.setBottom(m_graphRect.bottom());
            bar.setTop(bar.bottom() - qRound((double)m_barSize.height() * level));
            p.drawRoundedRect(bar, 3, 3);
        }
    }
}

void AudioMeterWidget::drawPeaks(QPainter& p)
{
    int chanCount = m_peaks.size();
    QRectF bar;

    if (m_orient == Qt::Horizontal) {
        for (int i = 0; i < chanCount; i++) {
            if (m_peaks[i] == m_levels[i])
                continue;
            double level = IEC_ScaleMax(m_peaks[i], m_maxDb);
            bar.setLeft(m_graphRect.left() + m_barSize.width() * level - 3);
            if (bar.left() < m_graphRect.left())
                continue;
            bar.setRight(bar.left() + 3);
            bar.setBottom(m_graphRect.bottom() - (chanCount - 1 - i) * m_barSize.height() - 1);
            bar.setTop(bar.bottom() - m_barSize.height() + 1);
            p.drawRoundedRect(bar, 3, 3);
        }
    } else {
        for (int i = 0; i < chanCount; i++) {
            if (m_peaks[i] == m_levels[i])
                continue;
            double level = IEC_ScaleMax(m_peaks[i], m_maxDb);
            bar.setLeft(m_graphRect.left() + i * m_barSize.width() + 1);
            bar.setRight(bar.left() + m_barSize.width() - 2);
            bar.setBottom(m_graphRect.bottom() - m_barSize.height() * level + 3);
            if (bar.bottom() > m_graphRect.bottom())
                continue;
            bar.setTop(bar.bottom() - 3);
            p.drawRoundedRect(bar, 3, 3);
        }
    }
}

void AudioMeterWidget::updateToolTip()
{
    QString text = "";
    int chan = -1;
    QPoint mousePos = mapFromGlobal(QCursor::pos());

    if (this->rect().contains(mousePos)) {
        if (m_orient == Qt::Horizontal) {
            if (mousePos.y() <= m_graphRect.bottom() && mousePos.y() >= m_graphRect.top()) {
                chan = (int)(m_graphRect.bottom() - mousePos.y()) / (int)m_barSize.height();
                chan = m_levels.size() - 1 - chan;
            }
        } else {
            if (mousePos.x() >= m_graphRect.left() && mousePos.x() <= m_graphRect.right()) {
                chan = (int)(mousePos.x() - m_graphRect.left()) / (int)m_barSize.width();
            }
        }
    }

    if (chan >=0 && m_levels.size() > chan) {
        if (m_levels[chan] < -90) {
            text = "-inf dB";
        } else {
            text = QString("%1dBFS").arg(m_levels[chan], 0, 'f', 1);
        }

        if (m_chanLabels.size() > chan) {
            if (!m_chanLabelUnits.isEmpty()) {
                text = QString("%1%2: %3").arg(m_chanLabels[chan], m_chanLabelUnits, text);
            } else {
                text = QString("%1: %2").arg(m_chanLabels[chan], text);
            }
        }
    }
    QToolTip::showText(QCursor::pos(), text);
}

void AudioMeterWidget::paintEvent(QPaintEvent* /*e*/)
{
    if (!isVisible())
        return;

    QPainter p(this);
    p.setRenderHints( QPainter::Antialiasing );

    drawDbLabels(p);
    drawChanLabels(p);

    p.setBrush(m_gradient);
    QPen pen(Qt::transparent, 1);
    p.setPen(pen);

    drawBars(p);
    drawPeaks(p);

    p.end();
}

void AudioMeterWidget::resizeEvent(QResizeEvent*)
{
    calcGraphRect();
}

void AudioMeterWidget::mouseMoveEvent(QMouseEvent*)
{
    updateToolTip();
}
