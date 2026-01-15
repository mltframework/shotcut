/*
 * Copyright (c) 2019-2026 Meltytech, LLC
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

#include "videorgbwaveformscopewidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

static const QColor TEXT_COLOR = {255, 255, 255, 127};

VideoRgbWaveformScopeWidget::VideoRgbWaveformScopeWidget()
    : ScopeWidget("RgbWaveform")
    , m_frame()
    , m_renderImg()
    , m_mutex()
    , m_displayImg()
{
    LOG_DEBUG() << "begin";
    setMouseTracking(true);
    setWhatsThis("https://forum.shotcut.org/t/video-rgb-waveform-scope/15652/1");
    LOG_DEBUG() << "end";
}

void VideoRgbWaveformScopeWidget::refreshScope(const QSize &size, bool full)
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
            m_renderImg = QImage(width, 256, QImage::QImage::Format_RGBX8888);
        }

        QColor bgColor(0, 0, 0, 0xff);
        m_renderImg.fill(bgColor);

        const uint8_t *src = m_frame.get_image(mlt_image_rgb);
        uint8_t *dst = m_renderImg.scanLine(0);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint8_t ry = 255 - src[0];
                size_t rIndex = (ry * width + x) * 4;
                if (dst[rIndex] < 0xff) {
                    dst[rIndex] += 0x0f;
                }

                uint8_t gy = 255 - src[1];
                size_t gIndex = (gy * width + x) * 4 + 1;
                if (dst[gIndex] < 0xff) {
                    dst[gIndex] += 0x0f;
                }

                uint8_t by = 255 - src[2];
                size_t bIndex = (by * width + x) * 4 + 2;
                if (dst[bIndex] < 0xff) {
                    dst[bIndex] += 0x0f;
                }
                src += 3;
            }
        }

        QImage scaledImage = m_renderImg
                                 .scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                 .convertToFormat(QImage::Format_RGB32);

        m_mutex.lock();
        m_displayImg.swap(m_renderImg);
        m_mutex.unlock();
    }
}

void VideoRgbWaveformScopeWidget::paintEvent(QPaintEvent *)
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

    // Draw the graticule
    int textpad = 3;
    int textheight = fm.tightBoundingRect("0").height();
    qreal y = 0;
    // 255
    y = 0;
    p.drawLine(QPointF(0, y), QPointF(width(), y));
    p.drawText(textpad, textheight + textpad, tr("255"));
    // 191
    y = (qreal) height() - 191.0 / 255.0 * (qreal) height();
    p.drawLine(0, y, width(), y);
    p.drawText(textpad, height() / 4 - textpad, tr("191"));
    // 127
    y = (qreal) height() - 127.0 / 255.0 * (qreal) height();
    p.drawLine(0, y, width(), y);
    p.drawText(textpad, height() / 2 - textpad, tr("127"));
    // 64
    y = (qreal) height() - 64.0 / 255.0 * (qreal) height();
    p.drawLine(0, y, width(), y);
    p.drawText(textpad, height() * 3 / 4 - textpad, tr("64"));
    // 0
    y = height();
    p.drawLine(0, y, width(), y);
    p.drawText(textpad, height() - textpad, tr("0"));
}

void VideoRgbWaveformScopeWidget::mouseMoveEvent(QMouseEvent *event)
{
    QString text;

    m_mutex.lock();
    int frameWidth = m_displayImg.width();
    m_mutex.unlock();

    int value = 255 - (255 * event->pos().y() / height());

    if (frameWidth != 0) {
        int pixel = frameWidth * event->pos().x() / width();
        text = tr("Pixel: %1\nValue: %2").arg(pixel).arg(value);
    } else {
        text = tr("Value: %1").arg(value);
    }
    QToolTip::showText(event->globalPosition().toPoint(), text);
}

QString VideoRgbWaveformScopeWidget::getTitle()
{
    return tr("Video RGB Waveform");
}
