/*
 * Copyright (c) 2018-2020 Meltytech, LLC
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

#include "videohistogramscopewidget.h"
#include <Logger.h>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

const qreal IRE0 = 16;
const qreal IRE100 = 235;

VideoHistogramScopeWidget::VideoHistogramScopeWidget()
    : ScopeWidget("VideoHistogram")
    , m_frame()
    , m_mutex(QMutex::NonRecursive)
    , m_yBins()
    , m_rBins()
    , m_gBins()
    , m_bBins()
{
    LOG_DEBUG() << "begin";
    setMouseTracking(true);
    LOG_DEBUG() << "end";
}

void VideoHistogramScopeWidget::refreshScope(const QSize &size, bool full)
{
    Q_UNUSED(size)
    Q_UNUSED(full)

    while (m_queue.count() > 0) {
        m_frame = m_queue.pop();
    }

    QVector<unsigned int> yBins(256, 0);
    QVector<unsigned int> rBins(256, 0);
    QVector<unsigned int> gBins(256, 0);
    QVector<unsigned int> bBins(256, 0);

    if (m_frame.is_valid() && m_frame.get_image_width() && m_frame.get_image_height()) {
        const uint8_t *pYUV = m_frame.get_image(mlt_image_yuv420p);
        const uint8_t *pRGB = m_frame.get_image(mlt_image_rgb);
        size_t count = m_frame.get_image_width() * m_frame.get_image_height();
        unsigned int *pYbin = yBins.data();
        unsigned int *pRbin = rBins.data();
        unsigned int *pGbin = gBins.data();
        unsigned int *pBbin = bBins.data();
        while (count--) {
            pYbin[*pYUV++]++;
            pRbin[*pRGB++]++;
            pGbin[*pRGB++]++;
            pBbin[*pRGB++]++;
        }
    }

    m_mutex.lock();
    m_yBins.swap(yBins);
    m_rBins.swap(rBins);
    m_gBins.swap(gBins);
    m_bBins.swap(bBins);
    m_mutex.unlock();
}

void VideoHistogramScopeWidget::paintEvent(QPaintEvent *)
{
    if (!isVisible())
        return;

    // Create the painter
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont font = QWidget::font();
    int fontSize = font.pointSize() - (font.pointSize() > 10 ? 2 : (font.pointSize() > 8 ? 1 : 0));
    font.setPointSize(fontSize);
    p.setFont(font);

    // draw the waveform data
    int histHeight = rect().height() / 4;
    QRect histRect;

    m_mutex.lock();

    histRect = rect();
    histRect.setHeight(histHeight);
    drawHistogram(p, tr("Luma"), Qt::white, palette().text().color(), m_yBins, histRect);

    histRect = rect();
    histRect.setTop(histHeight);
    histRect.setHeight(histHeight);
    drawHistogram(p, tr("Red"), Qt::red, Qt::red, m_rBins, histRect);

    histRect = rect();
    histRect.setTop(histHeight * 2);
    histRect.setHeight(histHeight);
    drawHistogram(p, tr("Green"), Qt::green, Qt::green, m_gBins, histRect);

    histRect = rect();
    histRect.setTop(histHeight * 3);
    histRect.setHeight(histHeight);
    drawHistogram(p, tr("Blue"), Qt::blue, Qt::blue, m_bBins, histRect);

    m_mutex.unlock();

    p.end();
}

void VideoHistogramScopeWidget::drawHistogram(QPainter &p, QString title, QColor color,
                                              QColor outline, QVector<unsigned int> &bins, QRect rect)
{
    unsigned int binCount = bins.size();
    unsigned int *pBins = bins.data();
    QFontMetrics fm(p.font());
    int textpad = 3;
    qreal histHeight = rect.height() - fm.height() - textpad - textpad;
    QPen pen;

    // Find the highest value
    unsigned int maxLevel = 0;
    unsigned int minValue = 255;
    unsigned int maxValue = 0;
    for (unsigned int i = 0; i < binCount; i++) {
        if (pBins[i] > maxLevel)
            maxLevel = pBins[i];
        if (minValue > i && pBins[i] != 0)
            minValue = i;
        if (maxValue < i && pBins[i] != 0)
            maxValue = i;
    }

    // Draw the title and max/min values
    pen.setColor(palette().text().color().rgb());
    p.setPen(pen);
    QString text;
    if (maxLevel > minValue)
        text = QString("%1\tMin: %2\tMax: %3").arg(title, QString::number(minValue),
                                                   QString::number(maxValue));
    else
        text = title;
    p.drawText(textpad, rect.y() + fm.height() + textpad, text);

    // Nothing to draw.
    if (maxLevel < minValue) return;

    // Set histogram background gradient.
    QLinearGradient gradient = QLinearGradient(rect.left(), rect.top(), rect.right(), rect.top());
    gradient.setColorAt(0, color.darker().darker());
    gradient.setColorAt(0.2, color.darker().darker());
    gradient.setColorAt(0.8, color);
    gradient.setColorAt(1, color);
    QBrush brush(gradient);
    p.setBrush(brush);

    // Set histogram outline.
    pen.setColor(outline);
    pen.setWidth(1);
    p.setPen(pen);

    QPainterPath histPath;
    histPath.moveTo( rect.bottomLeft() );
    qreal lineWidth = (qreal)rect.width() / 256.0;
    for (unsigned int i = 0; i < binCount; i++) {
        qreal xPos = (qreal)rect.width() * i / binCount;
        qreal yPos = (qreal)rect.bottom() - (histHeight * pBins[i] / maxLevel);
        histPath.lineTo(xPos, yPos);
        histPath.lineTo(xPos + lineWidth, yPos);
    }
    histPath.lineTo( rect.bottomRight() );
    histPath.closeSubpath();
    p.drawPath( histPath );
}

void VideoHistogramScopeWidget::mouseMoveEvent(QMouseEvent *event)
{
    QString text;
    int value = 256 * event->pos().x() / width();

    if (event->pos().y() < height() / 4) {
        // Show value and IRE in the luminance histogram.
        qreal ire100x = width() * IRE100 / 256;
        qreal ire0x = width() * IRE0 / 256;
        qreal ireStep = (ire0x - ire100x) / 100.0;
        int ire = (ire0x - event->pos().x()) / ireStep;
        text = QString(tr("Value: %1\nIRE: %2")).arg(QString::number(value), QString::number(ire));
    } else {
        text = QString(tr("Value: %1")).arg(QString::number(value));
    }

    QToolTip::showText(event->globalPos(), text);
}

QString VideoHistogramScopeWidget::getTitle()
{
    return tr("Video Histogram");
}
