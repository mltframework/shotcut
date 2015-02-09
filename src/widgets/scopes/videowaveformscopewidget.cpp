/*
 * Copyright (c) 2015 Meltytech, LLC
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
#include <QDebug>
#include <QPainter>

VideoWaveformScopeWidget::VideoWaveformScopeWidget()
  : ScopeWidget("VideoZoom")
  , m_frame()
  , m_prevSize(0, 0)
  , m_renderImg()
  , m_refreshTime()
  , m_mutex(QMutex::NonRecursive)
  , m_displayImg()
  , m_size(0, 0)
{
    qDebug() << "begin";
    m_refreshTime.start();
    qDebug() << "end";
}


void VideoWaveformScopeWidget::refreshScope()
{
    m_mutex.lock();
    QSize currentSize = m_size;
    m_mutex.unlock();

    while (m_queue.count() > 0) {
        m_frame = m_queue.pop();
    }

    if (m_prevSize == currentSize && m_refreshTime.elapsed() < 90) {
        // When not resizing, limit refreshes to 90ms.
        return;
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

    m_prevSize = currentSize;
    m_refreshTime.restart();
}

void VideoWaveformScopeWidget::paintEvent(QPaintEvent*)
{
    if (!isVisible())
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(0, 0, width(), height(), QBrush(Qt::black, Qt::SolidPattern));
    m_mutex.lock();
    if(!m_displayImg.isNull()) {
        p.drawImage(rect(), m_displayImg, m_displayImg.rect());
    }
    m_mutex.unlock();
    p.end();
}

void VideoWaveformScopeWidget::resizeEvent(QResizeEvent*)
{
    m_mutex.lock();
    m_size = size();
    m_mutex.unlock();
    requestRefresh();
}

QString VideoWaveformScopeWidget::getTitle()
{
   return tr("Video Waveform");
}
