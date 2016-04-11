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
#include <QPainter>

VideoWaveformScopeWidget::VideoWaveformScopeWidget()
  : ScopeWidget("VideoZoom")
  , m_frame()
  , m_renderImg()
  , m_refreshTime()
  , m_mutex(QMutex::NonRecursive)
  , m_displayImg()
{
    LOG_DEBUG() << "begin";
    m_refreshTime.start();
    LOG_DEBUG() << "end";
}


void VideoWaveformScopeWidget::refreshScope(const QSize& size, bool full)
{
    Q_UNUSED(size)
    while (m_queue.count() > 0) {
        m_frame = m_queue.pop();
    }

    if (!full && m_refreshTime.elapsed() < 90) {
        // Limit refreshes to 90ms unless there is a good reason.
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

QString VideoWaveformScopeWidget::getTitle()
{
   return tr("Video Waveform");
}
