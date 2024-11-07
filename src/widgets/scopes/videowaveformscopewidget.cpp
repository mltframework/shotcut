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

#include "videowaveformscopewidget.h"
#include <Logger.h>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

static const qreal IRE0 = 16;
static const qreal IRE100 = 235;
static const QColor TEXT_COLOR = {255, 255, 255, 127};


VideoWaveformScopeWidget::VideoWaveformScopeWidget()
    : ScopeWidget("VideoWaveform")
    , m_frame()
    , m_renderImg()
    , m_mutex()
    , m_displayImg()
{
    LOG_DEBUG() << "begin";
    setMouseTracking(true);
    LOG_DEBUG() << "end";
}


void VideoWaveformScopeWidget::refreshScope(const QSize &size, bool full)
{
    Q_UNUSED(size)
    Q_UNUSED(full)

    while (m_queue.count() > 0) {
        m_frame = m_queue.pop();
    }

    int width = m_frame.get_image_width();
    int height = m_frame.get_image_height();

    if (m_frame.is_valid() && width && height) {
        if (m_renderImg.width() != width) {
            m_renderImg = QImage(width, 256, QImage::Format_RGBX8888);
        }

        QColor bgColor( 0, 0, 0, 0xff );
        m_renderImg.fill(bgColor);

        const uint8_t *src = m_frame.get_image(mlt_image_yuv420p);
        uint8_t *dst = m_renderImg.scanLine(0);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint8_t dy = 255 - src[0];
                size_t dIndex = (dy * width + x) * 4;
                if (dst[dIndex] < 0xff) {
                    dst[dIndex] += 0x0f;
                    dst[dIndex + 1] += 0x0f;
                    dst[dIndex + 2] += 0x0f;
                }
                src ++;
            }
        }

        QImage scaledImage = m_renderImg.scaled(size, Qt::IgnoreAspectRatio,
                                                Qt::SmoothTransformation).convertToFormat(QImage::Format_RGB32);

        m_mutex.lock();
        m_displayImg.swap(scaledImage);
        m_mutex.unlock();
    }
}

void VideoWaveformScopeWidget::paintEvent(QPaintEvent *)
{
    if (!isVisible())
        return;

    // Create the painter
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont font = QWidget::font();
    int fontSize = font.pointSize() - (font.pointSize() > 10 ? 2 : (font.pointSize() > 8 ? 1 : 0));
    font.setPointSize(fontSize);
    QFontMetrics fm(font);
    QPen pen;
    pen.setColor(TEXT_COLOR);
    pen.setWidth(qRound(devicePixelRatioF()));
    p.setPen(pen);
    p.setFont(font);

    // draw the waveform data
    m_mutex.lock();
    if (!m_displayImg.isNull()) {
        p.drawImage(rect(), m_displayImg, m_displayImg.rect());
    } else {
        p.fillRect(rect(), QBrush(Qt::black, Qt::SolidPattern));
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

    if (frameWidth != 0) {
        int pixel = frameWidth * event->pos().x() / width();
        text =  tr("Pixel: %1\nIRE: %2").arg(pixel).arg(ire);
    } else {
        text =  tr("IRE: %1").arg(ire);
    }
    QToolTip::showText(event->globalPosition().toPoint(), text);
}

QString VideoWaveformScopeWidget::getTitle()
{
    return tr("Video Waveform");
}
