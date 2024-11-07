/*
 * Copyright (c) 2019-2022 Meltytech, LLC
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

#include "videovectorscopewidget.h"

#include "mltcontroller.h"
#include "qmltypes/qmlprofile.h"
#include <Logger.h>

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <QToolTip>

static const QColor LINE_COLOR = {255, 255, 255, 127};

VideoVectorScopeWidget::VideoVectorScopeWidget()
    : ScopeWidget("VideoVector")
    , m_frame()
    , m_renderImg()
    , m_mutex()
    , m_displayImg()
    , m_profileChanged(false)
{
    LOG_DEBUG() << "begin";
    setMouseTracking(true);
    profileChanged();
    connect(&QmlProfile::singleton(), SIGNAL(profileChanged()), this, SLOT(profileChanged()));
    LOG_DEBUG() << "end";
}

VideoVectorScopeWidget::~VideoVectorScopeWidget()
{
    disconnect(&QmlProfile::singleton(), SIGNAL(profileChanged()), this, SLOT(profileChanged()));
}

QString VideoVectorScopeWidget::getTitle()
{
    return tr("Video Vector");
}

void VideoVectorScopeWidget::refreshScope(const QSize &size, bool full)
{
    Q_UNUSED(full)

    qreal side = qMin(size.width(), size.height());
    QSize squareSize = QSize(side, side);

    if (m_graticuleImg.size() != size || m_profileChanged) {

        m_graticuleImg = QImage(squareSize, QImage::Format_RGB32);
        m_graticuleImg.fill(0);
        QPainter p(&m_graticuleImg);
        p.setRenderHint(QPainter::Antialiasing, true);

        // Convert the coordinate system to match the U/V coordinate system
        // 256x256 going up from the bottom
        p.translate(0, side);
        p.scale(side / 256.0, -1.0 * side / 256.0);

        m_mutex.lock();

        drawGraticuleLines(p, devicePixelRatioF());

        drawGraticuleMark(p, m_points[BLUE_100], Qt::blue, devicePixelRatioF() * 2, 8);
        drawGraticuleMark(p, m_points[CYAN_100], Qt::cyan, devicePixelRatioF() * 2, 8);
        drawGraticuleMark(p, m_points[GREEN_100], Qt::green, devicePixelRatioF() * 2, 8);
        drawGraticuleMark(p, m_points[YELLOW_100], Qt::yellow, devicePixelRatioF() * 2, 8);
        drawGraticuleMark(p, m_points[RED_100], Qt::red, devicePixelRatioF() * 2, 8);
        drawGraticuleMark(p, m_points[MAGENTA_100], Qt::magenta, devicePixelRatioF() * 2, 8);
        drawGraticuleMark(p, m_points[BLUE_75], Qt::blue, devicePixelRatioF(), 5);
        drawGraticuleMark(p, m_points[CYAN_75], Qt::cyan, devicePixelRatioF(), 5);
        drawGraticuleMark(p, m_points[GREEN_75], Qt::green, devicePixelRatioF(), 5);
        drawGraticuleMark(p, m_points[YELLOW_75], Qt::yellow, devicePixelRatioF(), 5);
        drawGraticuleMark(p, m_points[RED_75], Qt::red, devicePixelRatioF(), 5);
        drawGraticuleMark(p, m_points[MAGENTA_75], Qt::magenta, devicePixelRatioF(), 5);

        drawSkinToneLine(p, devicePixelRatioF());

        m_profileChanged = false;
        p.end();

        m_mutex.unlock();
    }

    while (m_queue.count() > 0) {
        m_frame = m_queue.pop();
    }

    int width = m_frame.get_image_width();
    int height = m_frame.get_image_height();

    if (m_frame.is_valid() && width && height) {
        if (m_renderImg.width() != 256) {
            m_renderImg = QImage(256, 256, QImage::Format_RGBX8888);
        }
        m_renderImg.fill(0);

        const uint8_t *src = m_frame.get_image(mlt_image_yuv420p);
        const uint8_t *uSrc = src + (width * height);
        const uint8_t *vSrc = uSrc + (width * height / 4);
        uint8_t *dst = m_renderImg.scanLine(0);
        int cHeight = height / 2;
        int cWidth = width / 2;

        for (int y = 0; y < cHeight; y++) {
            for (int x = 0; x < cWidth; x++) {
                uint8_t dx = *uSrc;
                uint8_t dy = 255 - *vSrc;
                size_t dIndex = (dy * 256 + dx) * 4;
                if (dst[dIndex] < 0xff) {
                    dst[dIndex] += 0x0f;
                    dst[dIndex + 1] += 0x0f;
                    dst[dIndex + 2] += 0x0f;
                }
                uSrc++;
                vSrc++;
            }
        }

        QImage newDisplayImage = m_graticuleImg.copy();
        QPainter p(&newDisplayImage);
        // Use "plus" composition so that light points will stand out on top of a graticule line.
        p.setCompositionMode(QPainter::CompositionMode_Plus);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawImage(newDisplayImage.rect(), m_renderImg, m_renderImg.rect());
        p.end();

        m_mutex.lock();
        m_displayImg.swap(newDisplayImage);
        m_mutex.unlock();
    } else {
        m_mutex.lock();
        m_displayImg = m_graticuleImg.copy();
        m_mutex.unlock();
    }
}

void VideoVectorScopeWidget::drawGraticuleLines(QPainter &p, qreal lineWidth)
{
    QRadialGradient radialGradient(128.0, 128.0, 128.0);
    radialGradient.setColorAt(0.0, Qt::transparent);
    radialGradient.setColorAt(0.05, Qt::transparent);
    radialGradient.setColorAt(0.06, LINE_COLOR.darker());
    radialGradient.setColorAt(0.10, LINE_COLOR.darker());
    radialGradient.setColorAt(0.11, Qt::transparent);
    radialGradient.setColorAt(0.3, Qt::transparent);
    radialGradient.setColorAt(1.0, LINE_COLOR);
    QBrush graticuleBrush(radialGradient);
    p.setBrush(graticuleBrush);
    p.setPen(QPen(graticuleBrush, lineWidth));
    p.drawLine(m_points[BLUE_100], m_points[YELLOW_100]);
    p.drawLine(m_points[CYAN_100], m_points[RED_100]);
    p.drawLine(m_points[GREEN_100], m_points[MAGENTA_100]);
}

void VideoVectorScopeWidget::drawSkinToneLine(QPainter &p, qreal lineWidth)
{
    // Draw a skin tone line 33 degrees counter clockwise from the red vector
    qreal angle = qRadiansToDegrees(qAtan((qreal)(m_points[RED_100].x() - 128) / (qreal)(
                                              m_points[RED_100].y() - 128)));
    angle += 270;
    angle -= 33;

    QRadialGradient radialGradient(128.0, 128.0, 128.0);
    radialGradient.setColorAt(0.0, Qt::transparent);
    radialGradient.setColorAt(0.2, Qt::transparent);
    radialGradient.setColorAt(1.0, LINE_COLOR);
    QBrush graticuleBrush(radialGradient);
    p.setBrush(graticuleBrush);
    p.setPen(QPen(graticuleBrush, lineWidth, Qt::DotLine));
    QLineF skinToneLine;
    skinToneLine.setP1(QPoint(128, 128));
    skinToneLine.setLength(120);
    skinToneLine.setAngle(angle);
    p.drawLine(skinToneLine);
}

void VideoVectorScopeWidget::drawGraticuleMark(QPainter &p, const QPoint &point, QColor color,
                                               qreal lineWidth, qreal LineLength)
{
    color = color.darker(100);
    p.setBrush(color);
    p.setPen(QPen(color, lineWidth, Qt::SolidLine, Qt::RoundCap));
    QLineF angleline;
    qreal angle = qAtan((qreal)(point.x() - 128) / (qreal)(point.y() - 128));
    angle = qRadiansToDegrees(angle);
    angleline.setP1(point);
    angleline.setLength(LineLength / 2);
    angleline.setAngle(angle);
    p.drawLine(angleline);
    angleline.setAngle(angle + 180);
    p.drawLine(angleline);
}

void VideoVectorScopeWidget::paintEvent(QPaintEvent *)
{
    if (!isVisible())
        return;

    QRect squareRect = getCenteredSquare();

    // Create the painter
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // draw the vector image
    m_mutex.lock();
    if (!m_displayImg.isNull()) {
        p.drawImage(squareRect, m_displayImg, m_displayImg.rect());
    } else {
        p.fillRect(squareRect, QBrush(Qt::black, Qt::SolidPattern));
    }
    m_mutex.unlock();
}

void VideoVectorScopeWidget::mouseMoveEvent(QMouseEvent *event)
{
    QRectF squareRect = getCenteredSquare();
    if (!squareRect.contains(event->pos())) {
        QToolTip::hideText();
        return;
    }
    qreal realX = (qreal)event->pos().x() - ((qreal)width() - squareRect.width()) / 2;
    qreal realY = (qreal)event->pos().y() - ((qreal)height() - squareRect.height()) / 2;
    qreal u = realX * 255.0 / squareRect.width();
    qreal v = (squareRect.height() - realY) * 255.0 / squareRect.height();
    QString text =  tr("U: %1\nV: %2").arg(qRound(u)).arg(qRound(v));
    QToolTip::showText(event->globalPosition().toPoint(), text);
}

QRect VideoVectorScopeWidget::getCenteredSquare()
{
    // Calculate the size. Vectorscope is always a square.
    QRect squareRect;
    if (width() > height()) {
        int x = (width() - height()) / 2;
        squareRect = QRect(x, 0, height(), height());
    } else {
        int y = (height() - width()) / 2;
        squareRect = QRect(0, y, width(), width());
    }
    return squareRect;
}

void VideoVectorScopeWidget::profileChanged()
{
    LOG_DEBUG() << MLT.profile().colorspace();
    m_mutex.lock();
    switch (MLT.profile().colorspace()) {
    case 601:
        m_points[BLUE_75]     = QPoint(212, 114);
        m_points[CYAN_75]     = QPoint(156, 44);
        m_points[GREEN_75]    = QPoint(72, 58);
        m_points[YELLOW_75]   = QPoint(44, 142);
        m_points[RED_75]      = QPoint(100, 212);
        m_points[MAGENTA_75]  = QPoint(184, 198);
        m_points[BLUE_100]    = QPoint(240, 110);
        m_points[CYAN_100]    = QPoint(166, 16);
        m_points[GREEN_100]   = QPoint(54, 34);
        m_points[YELLOW_100]  = QPoint(16, 146);
        m_points[RED_100]     = QPoint(90, 240);
        m_points[MAGENTA_100] = QPoint(202, 222);
        break;
    default:
    case 709:
        m_points[BLUE_75]     = QPoint(212, 120);
        m_points[CYAN_75]     = QPoint(147, 44);
        m_points[GREEN_75]    = QPoint(63, 52);
        m_points[YELLOW_75]   = QPoint(44, 136);
        m_points[RED_75]      = QPoint(109, 212);
        m_points[MAGENTA_75]  = QPoint(193, 204);
        m_points[BLUE_100]    = QPoint(240, 118);
        m_points[CYAN_100]    = QPoint(154, 16);
        m_points[GREEN_100]   = QPoint(42, 26);
        m_points[YELLOW_100]  = QPoint(16, 138);
        m_points[RED_100]     = QPoint(102, 240);
        m_points[MAGENTA_100] = QPoint(214, 230);
        break;
    }
    m_profileChanged = true;
    m_mutex.unlock();
    requestRefresh();
}
