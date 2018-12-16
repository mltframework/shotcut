/*
 * Copyright (c) 2015-2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#include "videowaveformscopewidget.h"
#include <Logger.h>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

const qreal IRE0 = 16;
const qreal IRE100 = 235;

VideoWaveformScopeWidget::VideoWaveformScopeWidget()
  : ScopeWidget("VideoZoom")
  , m_frame()
  , m_renderImg()
  , m_mutex(QMutex::NonRecursive)
  , m_displayImg()
{
    LOG_DEBUG() << "begin";
    setMouseTracking(true);
    LOG_DEBUG() << "end";
}


void VideoWaveformScopeWidget::refreshScope(const QSize& size, bool full)
{
    Q_UNUSED(size)
    Q_UNUSED(full)

    while (m_queue.count() > 0) {
        m_frame = m_queue.pop();
    }

    if (m_frame.is_valid() && m_frame.get_image_width() && m_frame.get_image_height()) {
        int columns = m_frame.get_image_width();
        if (m_renderImg.width() != columns) {
            m_renderImg = QImage(columns, 256, QImage::Format_ARGB32_Premultiplied);
        }
        QColor bgColor( 0, 0, 0 ,0 );
        m_renderImg.fill(bgColor);

        const uint8_t* yData = m_frame.get_image();

        for (int x = 0; x < columns; x++) {
            int pixels = m_frame.get_image_height();
            for (int j = 0; j < pixels; j++) {
                int y = 255 - (yData[j * columns + x]);
                QRgb currentVal = m_renderImg.pixel(x,y);
                if (currentVal < 0xffffffff) {
                    currentVal += 0x0f0f0f0f;
                    m_renderImg.setPixel(x, y, currentVal);
                }
            }
        }
    }

    m_mutex.lock();
    m_displayImg.swap(m_renderImg);
    m_mutex.unlock();
}

void VideoWaveformScopeWidget::paintEvent(QPaintEvent*)
{
    if (!isVisible())
        return;

    // Create the painter
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont font = QWidget::font();
    int fontSize = font.pointSize() - (font.pointSize() > 10? 2 : (font.pointSize() > 8? 1 : 0));
    font.setPointSize(fontSize);
    QFontMetrics fm(font);
    p.setPen(palette().text().color().rgb());
    p.setFont(font);

    // Fill the background
    p.fillRect(0, 0, width(), height(), QBrush(Qt::black, Qt::SolidPattern));

    // draw the waveform data
    m_mutex.lock();
    if(!m_displayImg.isNull()) {
        p.drawImage(rect(), m_displayImg, m_displayImg.rect());
    }
    m_mutex.unlock();

    // Add IRE lines
    int textpad = 3;
    // 100
    qreal ire100y = height() - (height() * IRE100 / 255);
    p.drawLine(QPointF(0, ire100y), QPointF(width(), ire100y));
    p.drawText(textpad, ire100y - textpad, tr("100"));
    // 0
    qreal ire0y = height() - (height() * IRE0 / 255);
    p.drawLine(QPointF(0, ire0y), QPointF(width(), ire0y));
    QRect textRect = fm.tightBoundingRect(tr("0"));
    p.drawText(textpad, ire0y + textRect.height() + textpad, tr("0"));

    p.end();
}

void VideoWaveformScopeWidget::mouseMoveEvent(QMouseEvent *event)
{
    QString text;
    qreal ire100y = height() - (height() * IRE100 / 255);
    qreal ire0y = height() - (height() * IRE0 / 255);
    qreal ireStep = (ire0y - ire100y) / 100.0;
    int ire = (ire0y - event->pos().y()) / ireStep;

    m_mutex.lock();
    int frameWidth = m_displayImg.width();
    m_mutex.unlock();

    if(frameWidth != 0)
    {
        int pixel = frameWidth * event->pos().x() / width();
        text =  QString(tr("Pixel: %1\nIRE: %2")).arg(QString::number(pixel), QString::number(ire));
    }
    else
    {
        text =  QString(tr("IRE: %1")).arg(QString::number(ire));
    }
    QToolTip::showText(event->globalPos(), text);
}

QString VideoWaveformScopeWidget::getTitle()
{
   return tr("Video Waveform");
}
