/*
 * Copyright (c) 2019-2023 Meltytech, LLC
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

#include "videozoomwidget.h"

#include "Logger.h"

#include <QMouseEvent>
#include <QMutexLocker>
#include <QPainter>
#include <QToolTip>

const int MIN_ZOOM = 2;
const int MAX_ZOOM = 20;
const int DEFAULT_ZOOM = 10;

QColor getHighContrastColor(const QColor &color)
{
    if (color.value() > 128) {
        return QColor(Qt::black);
    }
    return QColor(Qt::white);
}

VideoZoomWidget::VideoZoomWidget()
    : m_locked(false)
    , m_zoom(DEFAULT_ZOOM)
    , m_imageOffset(0, 0)
    , m_mouseGrabPixel(0, 0)
    , m_selectedPixel(-1, -1)
    , m_mutex()
    , m_frame()
{
    LOG_DEBUG() << "begin";
    setMouseTracking(true);
    LOG_DEBUG() << "end";
}

void VideoZoomWidget::putFrame(SharedFrame frame)
{
    if (!frame.is_valid())
        return;

    // Make sure the images are pre-cached for the UI thread
    frame.get_image(mlt_image_yuv420p);
    frame.get_image(mlt_image_rgb);

    m_mutex.lock();
    m_frame = frame;
    m_mutex.unlock();

    update();
}

QPoint VideoZoomWidget::getSelectedPixel()
{
    return m_selectedPixel;
}

void VideoZoomWidget::setSelectedPixel(QPoint pixel)
{
    QMutexLocker locker(&m_mutex);
    if (!m_frame.is_valid())
        return;
    if (pixel.x() < 0)
        return;
    if (pixel.x() >= m_frame.get_image_width())
        return;
    if (pixel.y() < 0)
        return;
    if (pixel.y() >= m_frame.get_image_height())
        return;
    m_selectedPixel = pixel;
    update();
    locker.unlock();
    emit pixelSelected(m_selectedPixel);
}

QRect VideoZoomWidget::getPixelRect()
{
    return QRect(m_imageOffset, QSize(width() / m_zoom, height() / m_zoom));
}

int VideoZoomWidget::getZoom()
{
    return m_zoom;
}

VideoZoomWidget::PixelValues VideoZoomWidget::getPixelValues(const QPoint &pixel)
{
    QMutexLocker locker(&m_mutex);
    return pixelToValues(pixel);
}

void VideoZoomWidget::setOffset(QPoint offset)
{
    QMutexLocker locker(&m_mutex);
    if (!m_frame.is_valid())
        return;
    if (offset.x() < 0)
        return;
    if (offset.x() >= m_frame.get_image_width())
        return;
    if (offset.y() < 0)
        return;
    if (offset.y() >= m_frame.get_image_height())
        return;
    m_imageOffset = offset;
    update();
}

void VideoZoomWidget::lock(bool locked)
{
    m_locked = locked;
}

QSize VideoZoomWidget::sizeHint() const
{
    return QSize(400, 400);
}

void VideoZoomWidget::paintEvent(QPaintEvent *)
{
    if (!isVisible())
        return;

    QMutexLocker locker(&m_mutex);
    if (!m_frame.is_valid())
        return;

    // Create the painter
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const uint8_t *pImg = m_frame.get_image(mlt_image_rgb);
    int iWidth = m_frame.get_image_width();
    int iHeight = m_frame.get_image_height();
    int wWidth = width() - (width() % m_zoom);
    int wHeight = height() - (height() % m_zoom);
    int ix = m_imageOffset.x();
    int iy = m_imageOffset.y();

    // draw the pixels
    for (int y = 0; y < wHeight && iy < iHeight; y += m_zoom) {
        const uint8_t *pPixel = pImg + ((iy * iWidth) + ix) * 3;
        for (int x = 0; x < wWidth; x += m_zoom) {
            p.fillRect(x, y, m_zoom, m_zoom, QColor(pPixel[0], pPixel[1], pPixel[2], 255));
            pPixel += 3;
        }
        iy++;
    }

    // Outline the selected pixel
    if (m_selectedPixel.x() >= 0 && m_selectedPixel.y() >= 0 && m_selectedPixel.x() < iWidth
        && m_selectedPixel.y() < iHeight) {
        const uint8_t *pPixel = pImg + ((m_selectedPixel.y() * iWidth) + m_selectedPixel.x()) * 3;
        int posX = (m_selectedPixel.x() - m_imageOffset.x()) * m_zoom;
        int posY = (m_selectedPixel.y() - m_imageOffset.y()) * m_zoom;
        QColor pixelcolor(pPixel[0], pPixel[1], pPixel[2]);
        p.setPen(getHighContrastColor(pixelcolor));
        p.drawRect(posX, posY, m_zoom, m_zoom);
    }
}

void VideoZoomWidget::mouseMoveEvent(QMouseEvent *event)
{
    QMutexLocker locker(&m_mutex);
    if (!m_frame.is_valid())
        return;
    int iWidth = m_frame.get_image_width();
    int iHeight = m_frame.get_image_height();
    QPoint currMousePixel = posToPixel(event->pos());
    if (currMousePixel.x() < 0)
        return;
    if (currMousePixel.x() >= iWidth)
        return;
    if (currMousePixel.y() < 0)
        return;
    if (currMousePixel.y() >= iHeight)
        return;
    locker.unlock();

    if (event->buttons() & Qt::LeftButton) {
        if (currMousePixel != m_mouseGrabPixel) {
            int maxOffsetX = iWidth - (width() / m_zoom);
            int maxOffsetY = iHeight - (height() / m_zoom);
            // Calculate the new image offset
            QPoint newImageOffset;
            newImageOffset.setX((int) m_mouseGrabPixel.x() - ((int) event->pos().x() / m_zoom));
            newImageOffset.setX(qBound(0, newImageOffset.x(), maxOffsetX));
            newImageOffset.setY((int) m_mouseGrabPixel.y() - ((int) event->pos().y() / m_zoom));
            newImageOffset.setY(qBound(0, newImageOffset.y(), maxOffsetY));

            // Apply the offset if it has changed
            if (newImageOffset != m_imageOffset) {
                m_imageOffset = newImageOffset;
            }
        }
    } else if (!m_locked) {
        m_selectedPixel = currMousePixel;
        emit pixelSelected(m_selectedPixel);
    }

    /*
        // Create a tool tip to display pixel information
        PixelValues values = pixelToValues(currMousePixel);
        QString text =  tr("Zoom: %1x\nPixel: %2,%3\nRGB: %4 %5 %6\nYUV: %7 %8 %9").arg(
                            QString::number(m_zoom)).arg(
                            QString::number(currMousePixel.x() + 1)).arg(
                            QString::number(currMousePixel.y() + 1)).arg(
                            QString::number(values.r)).arg(
                            QString::number(values.g)).arg(
                            QString::number(values.b)).arg(
                            QString::number(values.y)).arg(
                            QString::number(values.u)).arg(
                            QString::number(values.v));
        QToolTip::showText(event->globalPos(), text);
    */
    update();
}

void VideoZoomWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QMutexLocker locker(&m_mutex);
        if (!m_frame.is_valid())
            return;
        QPoint currMousePixel = posToPixel(event->pos());
        m_selectedPixel = currMousePixel;
        m_mouseGrabPixel = currMousePixel;
        locker.unlock();

        emit pixelSelected(m_selectedPixel);
        update();
    }
}

void VideoZoomWidget::wheelEvent(QWheelEvent *event)
{
    QMutexLocker locker(&m_mutex);
    if (!m_frame.is_valid())
        return;

    QPoint steps = event->angleDelta() / 8 / 15;
    int newZoom = qBound(MIN_ZOOM, m_zoom + steps.y(), MAX_ZOOM);

    if (newZoom != m_zoom) {
        // Zoom in on the center pixel.
        int iWidth = m_frame.get_image_width();
        int iHeight = m_frame.get_image_height();
        int maxOffsetX = iWidth - (width() / newZoom);
        int maxOffsetY = iHeight - (height() / newZoom);
        QPoint centerPixel = posToPixel(rect().center());
        m_imageOffset.setX(centerPixel.x() - (width() / newZoom / 2));
        m_imageOffset.setX(qBound(0, m_imageOffset.x(), maxOffsetX));
        m_imageOffset.setY(centerPixel.y() - (height() / newZoom / 2));
        m_imageOffset.setY(qBound(0, m_imageOffset.y(), maxOffsetY));
        m_zoom = newZoom;

        locker.unlock();
        emit zoomChanged(m_zoom);
        update();
    }

    if (locker.isLocked())
        locker.unlock();
    event->accept();
}

QPoint VideoZoomWidget::pixelToPos(const QPoint &pixel)
{
    int x = ((int) pixel.x() - (int) m_imageOffset.x()) * m_zoom;
    int y = ((int) pixel.y() - (int) m_imageOffset.y()) * m_zoom;
    return QPoint(x, y);
}

QPoint VideoZoomWidget::posToPixel(const QPoint &pos)
{
    int x = ((int) pos.x() / m_zoom) + (int) m_imageOffset.x();
    int y = ((int) pos.y() / m_zoom) + (int) m_imageOffset.y();
    return QPoint(x, y);
}

VideoZoomWidget::PixelValues VideoZoomWidget::pixelToValues(const QPoint &pixel)
{
    PixelValues values;
    int iWidth = m_frame.get_image_width();
    int iHeight = m_frame.get_image_height();
    int imageOffset = iWidth * pixel.y() + pixel.x();
    const uint8_t *pRgb = m_frame.get_image(mlt_image_rgb) + imageOffset * 3;
    const uint8_t *pYuv = m_frame.get_image(mlt_image_yuv420p);
    const uint8_t *pY = pYuv + imageOffset;
    const uint8_t *pU = pYuv + (iWidth * iHeight) + (iWidth / 2 * (pixel.y() / 2))
                        + (pixel.x() / 2);
    const uint8_t *pV = pYuv + (iWidth * iHeight * 5 / 4) + (iWidth / 2 * (pixel.y() / 2))
                        + (pixel.x() / 2);
    values.y = *pY;
    values.u = *pU;
    values.v = *pV;
    values.r = pRgb[0];
    values.g = pRgb[1];
    values.b = pRgb[2];
    return values;
}
